#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <dynamictargeting>

#define PLUGIN_VERSION "1.0.0"

/* FakeClientCommand() formats the command into a 256 byte buffer. */
#define MAX_COMMAND_LINE_LENGTH 256
#define MAX_COMMAND_NAME_LENGTH 64

public Plugin myinfo =
{
	name = "Dynamic Targeting",
	author = "BotoX, srcdslab",
	description = "Lets admins pick the intended player when a command target is ambiguous",
	version = PLUGIN_VERSION,
	url = "https://github.com/srcdslab/sm-ext-DynamicTargeting"
};

ConVar g_cvEnable;

/* Last command issued, as seen by the global command listener. The ambiguous
 * lookup happens while that command's callback runs, in the same tick. */
int g_iLastCmdClient;
int g_iLastCmdTick = -1;
ReplySource g_LastCmdReplySource;
char g_sLastCmdName[MAX_COMMAND_NAME_LENGTH];
char g_sLastCmdArgs[MAX_COMMAND_LINE_LENGTH];

/* Command to re-issue from each admin's menu, split around the ambiguous
 * argument. Only the latest menu of an admin is ever displayed. */
ReplySource g_PendingReplySource[MAXPLAYERS + 1];
char g_sPendingCmdStart[MAXPLAYERS + 1][MAX_COMMAND_LINE_LENGTH];
char g_sPendingCmdEnd[MAXPLAYERS + 1][MAX_COMMAND_LINE_LENGTH];

/* Used to sort the menu by name. */
char g_sSortNames[MAXPLAYERS + 1][MAX_NAME_LENGTH];

public void OnPluginStart()
{
	LoadTranslations("common.phrases");

	g_cvEnable = CreateConVar("sm_dynamictargeting_enable", "1", "Show a menu to pick the intended player when a command target is ambiguous", 0, true, 0.0, true, 1.0);
	AutoExecConfig(true);

	AddCommandListener(Listener_AnyCommand);
}

public void OnClientDisconnect(int client)
{
	g_sPendingCmdStart[client][0] = '\0';
	g_sPendingCmdEnd[client][0] = '\0';
}

public Action Listener_AnyCommand(int client, const char[] command, int argc)
{
	g_iLastCmdClient = client;
	g_iLastCmdTick = GetGameTickCount();
	g_LastCmdReplySource = GetCmdReplySource();
	strcopy(g_sLastCmdName, sizeof(g_sLastCmdName), command);

	/* A truncated argument string can't be re-issued faithfully. */
	if (GetCmdArgString(g_sLastCmdArgs, sizeof(g_sLastCmdArgs)) >= sizeof(g_sLastCmdArgs) - 1)
		g_sLastCmdName[0] = '\0';

	return Plugin_Continue;
}

public void DynamicTargeting_OnAmbiguousTarget(int admin, const char[] pattern, int filterFlags, const int[] clients, int numClients)
{
	if (!g_cvEnable.BoolValue)
		return;

	/* The command is re-issued on the admin's behalf, so only act on a lookup
	 * made for a command that the admin issued themselves. */
	if (!g_sLastCmdName[0] || admin != g_iLastCmdClient || GetGameTickCount() != g_iLastCmdTick)
		return;

	/* Re-issuing chat would repeat the message in chat. */
	if (StrEqual(g_sLastCmdName, "say") || StrEqual(g_sLastCmdName, "say_team"))
		return;

	/* The pattern must be one of the arguments, to swap it for a #userid. */
	int length;
	int start = FindArgument(g_sLastCmdArgs, pattern, length);
	if (start == -1)
		return;

	char sCmdStart[MAX_COMMAND_LINE_LENGTH];
	char sArgsStart[MAX_COMMAND_LINE_LENGTH];
	strcopy(sArgsStart, start + 1, g_sLastCmdArgs);
	FormatEx(sCmdStart, sizeof(sCmdStart), "%s %s", g_sLastCmdName, sArgsStart);

	char sCmdEnd[MAX_COMMAND_LINE_LENGTH];
	strcopy(sCmdEnd, sizeof(sCmdEnd), g_sLastCmdArgs[start + length]);

	/* "#" and up to 11 digits for the userid, plus the terminator. */
	if (strlen(sCmdStart) + strlen(sCmdEnd) + 13 > MAX_COMMAND_LINE_LENGTH)
		return;

	g_PendingReplySource[admin] = g_LastCmdReplySource;
	strcopy(g_sPendingCmdStart[admin], sizeof(g_sPendingCmdStart[]), sCmdStart);
	strcopy(g_sPendingCmdEnd[admin], sizeof(g_sPendingCmdEnd[]), sCmdEnd);

	DisplayAmbiguousMenu(admin, pattern, clients, numClients);
}

void DisplayAmbiguousMenu(int admin, const char[] pattern, const int[] clients, int numClients)
{
	int[] sorted = new int[numClients];
	for (int i = 0; i < numClients; i++)
	{
		sorted[i] = clients[i];
		GetClientName(clients[i], g_sSortNames[clients[i]], sizeof(g_sSortNames[]));
	}

	SortCustom1D(sorted, numClients, SortByName);

	Menu menu = new Menu(MenuHandler_Ambiguous, MenuAction_Select|MenuAction_End|MenuAction_DrawItem);
	menu.SetTitle("\"%s\": %T", pattern, "More than one client matched", admin);

	char sUserId[12];
	char sDisplay[MAX_NAME_LENGTH + 16];
	for (int i = 0; i < numClients; i++)
	{
		int userid = GetClientUserId(sorted[i]);
		IntToString(userid, sUserId, sizeof(sUserId));
		FormatEx(sDisplay, sizeof(sDisplay), "%s (#%d)", g_sSortNames[sorted[i]], userid);
		menu.AddItem(sUserId, sDisplay);
	}

	menu.Display(admin, MENU_TIME_FOREVER);
}

int SortByName(int elem1, int elem2, const int[] array, Handle hndl)
{
	return strcmp(g_sSortNames[elem1], g_sSortNames[elem2], false);
}

public int MenuHandler_Ambiguous(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			delete menu;
		}
		case MenuAction_DrawItem:
		{
			int style;
			char sUserId[12];
			menu.GetItem(param2, sUserId, sizeof(sUserId), style);

			/* The player left since the menu was built. */
			if (!GetClientOfUserId(StringToInt(sUserId)))
				return ITEMDRAW_DISABLED;

			return style;
		}
		case MenuAction_Select:
		{
			int client = param1;
			if (!g_sPendingCmdStart[client][0])
				return 0;

			char sUserId[12];
			menu.GetItem(param2, sUserId, sizeof(sUserId));

			/* Run it as the admin, so the usual access checks apply, replying
			 * where the original command replied (chat or console). */
			ReplySource oldSource = SetCmdReplySource(g_PendingReplySource[client]);
			FakeClientCommand(client, "%s#%s%s", g_sPendingCmdStart[client], sUserId, g_sPendingCmdEnd[client]);
			SetCmdReplySource(oldSource);
		}
	}

	return 0;
}

/**
 * Finds the first argument of a command argument string that is equal to
 * pattern, splitting arguments on whitespace and double quotes like
 * BreakString() does.
 *
 * @param args      Command argument string.
 * @param pattern   Argument to look for.
 * @param length    Set to the length of the argument in args, quotes included.
 * @return          Offset of the argument in args, quotes included, or -1.
 */
int FindArgument(const char[] args, const char[] pattern, int &length)
{
	char arg[MAX_COMMAND_LINE_LENGTH];
	int pos = 0;

	while (args[pos] != '\0')
	{
		while (args[pos] != '\0' && IsCharSpace(args[pos]))
			pos++;

		if (args[pos] == '\0')
			break;

		int start = pos;
		int len = 0;

		if (args[pos] == '"')
		{
			pos++;
			while (args[pos] != '\0' && args[pos] != '"')
				arg[len++] = args[pos++];

			if (args[pos] == '"')
				pos++;
		}
		else
		{
			while (args[pos] != '\0' && !IsCharSpace(args[pos]))
				arg[len++] = args[pos++];
		}

		arg[len] = '\0';

		if (StrEqual(arg, pattern))
		{
			length = pos - start;
			return start;
		}
	}

	return -1;
}
