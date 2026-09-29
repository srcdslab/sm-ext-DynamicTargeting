/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Dynamic Targeting Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include <ctype.h>
#include <vector>

#include "extension.h"

DynamicTargeting g_DynamicTargeting;
SMEXT_LINK(&g_DynamicTargeting);

IForward *g_pOnAmbiguousTarget = NULL;

/**
 * Case-insensitive substring search, identical to the one core uses to match
 * partial names, so that both always agree on what is ambiguous.
 */
static const char *stristr(const char *str, const char *substr)
{
	if (!*substr)
	{
		return str;
	}

	const char *needle = substr;
	const char *prevloc = str;
	const char *haystack = str;

	while (*haystack)
	{
		if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle))
		{
			haystack++;
			if (!*++needle)
			{
				return prevloc;
			}
		}
		else
		{
			haystack = ++prevloc;
			needle = substr;
		}
	}

	return NULL;
}

bool DynamicTargeting::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	g_pOnAmbiguousTarget = forwards->CreateForward("DynamicTargeting_OnAmbiguousTarget", ET_Ignore, 5, NULL,
		Param_Cell,		// admin
		Param_String,	// pattern
		Param_Cell,		// filter_flags
		Param_Array,	// clients
		Param_Cell);	// num_clients

	playerhelpers->RegisterCommandTargetProcessor(this);
	sharesys->RegisterLibrary(myself, "DynamicTargeting");

	return true;
}

void DynamicTargeting::SDK_OnUnload()
{
	playerhelpers->UnregisterCommandTargetProcessor(this);
	forwards->ReleaseForward(g_pOnAmbiguousTarget);
}

bool DynamicTargeting::ProcessCommandTarget(cmd_target_info_t *info)
{
	if (g_pOnAmbiguousTarget->GetFunctionCount() == 0)
	{
		return false;
	}

	/* Only admins in game can be offered a choice. Leave empty patterns and
	 * multi-target patterns ("@..."), which another processor may still claim,
	 * to core.
	 */
	if (info->admin <= 0 || info->pattern[0] == '\0' || info->pattern[0] == '@')
	{
		return false;
	}

	IGamePlayer *pAdmin = playerhelpers->GetGamePlayer(info->admin);
	if (!pAdmin || !pAdmin->IsInGame() || pAdmin->IsFakeClient())
	{
		return false;
	}

	/* Mirror core's partial name matching: the lookup is ambiguous as soon as
	 * two names match, whether or not the admin can target them. Only the
	 * clients the admin can target are reported.
	 */
	int matches = 0;
	std::vector<cell_t> clients;
	int max_clients = playerhelpers->GetMaxClients();
	for (int i = 1; i <= max_clients; i++)
	{
		IGamePlayer *pTarget = playerhelpers->GetGamePlayer(i);
		if (!pTarget)
		{
			continue;
		}

		const char *name = pTarget->GetName();
		if (!name || stristr(name, info->pattern) == NULL)
		{
			continue;
		}

		matches++;

		if (playerhelpers->FilterCommandTarget(pAdmin, pTarget, info->flags) == COMMAND_TARGET_VALID)
		{
			clients.push_back(i);
		}
	}

	if (matches < 2 || clients.empty())
	{
		return false;
	}

	g_pOnAmbiguousTarget->PushCell(info->admin);
	g_pOnAmbiguousTarget->PushString(info->pattern);
	g_pOnAmbiguousTarget->PushCell(info->flags);
	g_pOnAmbiguousTarget->PushArray(clients.data(), (unsigned int)clients.size());
	g_pOnAmbiguousTarget->PushCell((cell_t)clients.size());
	g_pOnAmbiguousTarget->Execute(NULL);

	/* Let core report COMMAND_TARGET_AMBIGUOUS as usual. */
	return false;
}
