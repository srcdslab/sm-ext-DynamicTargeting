#if defined _dynamictargeting_included
	#endinput
#endif
#define _dynamictargeting_included

/**
 * Called when a command target lookup (ProcessTargetString, FindTarget, ...)
 * is about to fail with COMMAND_TARGET_AMBIGUOUS, because the partial name
 * pattern matches more than one client.
 *
 * This is called from within the lookup, so usually from within the command
 * callback that made it. The lookup still fails as usual afterwards.
 *
 * Only called for lookups made on behalf of an in-game, non-fake admin, and
 * only if at least one of the matching clients can be targeted by that admin.
 *
 * @param admin         Admin performing the lookup.
 * @param pattern       Pattern that was looked up.
 * @param filterFlags   COMMAND_FILTER flags of the lookup.
 * @param clients       Matching clients the admin can target, by client index.
 * @param numClients    Number of clients in the array.
 */
forward void DynamicTargeting_OnAmbiguousTarget(int admin, const char[] pattern, int filterFlags, const int[] clients, int numClients);

/**
 * Do not edit below this line!
 */
public Extension __ext_dynamictargeting =
{
	name = "DynamicTargeting",
	file = "dynamictargeting.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
