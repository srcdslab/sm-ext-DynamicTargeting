# sm-ext-DynamicTargeting

When a command target is ambiguous (a partial name matches more than one player), shows the admin a menu to pick the intended player, then re-runs the command on that player.

Works with every plugin that uses `ProcessTargetString` / `FindTarget`, without modifying or recompiling them, and without patching SourceMod core.

## Example

Two players are connected: `gene` and `Ene ~special characters~`. An admin types `sm_slay Ene`:

1. The command fails as usual with *More than one client matched the given pattern.*
2. The admin also gets a menu listing `Ene ~special characters~ (#12)` and `gene (#7)`.
3. Picking one runs `sm_slay #12` (or `#7`) as that admin.

## How it works

It has two parts:

- **Extension** (`dynamictargeting.ext`): registers an `ICommandTargetProcessor` through SourceMod's public `IPlayerManager` interface. SourceMod calls these for every plain name pattern before its own partial name matching, so the extension sees every lookup made by any plugin. When a lookup is about to fail with `COMMAND_TARGET_AMBIGUOUS`, it fires `DynamicTargeting_OnAmbiguousTarget` with the matching players the admin can target. It never resolves the target itself, so the lookup still fails as it did before.
- **Plugin** (`DynamicTargeting.smx`): uses a global command listener to know which command is being run, and handles the forward: it shows the menu, then re-runs that command with the ambiguous argument replaced by the chosen player's `#userid`.

The command is re-run with `FakeClientCommand` as the admin, so the usual admin access checks apply. Replies go where the original command replied (chat or console).

## When the menu is shown

The menu is only shown when all of these are true:

- The lookup was made for an in-game, human admin, while running a command that this admin typed (in console or through a chat trigger such as `!slay`), in the same server tick.
- The ambiguous pattern is one of the command's arguments, so it can be swapped for a `#userid`.
- The command is not `say` / `say_team`, and the rebuilt command fits in 255 characters.

Patterns starting with `@` are left to SourceMod and to other target processors.

## Configuration

| ConVar | Default | Description |
| --- | --- | --- |
| `sm_dynamictargeting_enable` | `1` | Show a menu to pick the intended player when a command target is ambiguous |

## API

```sourcepawn
#include <dynamictargeting>

forward void DynamicTargeting_OnAmbiguousTarget(int admin, const char[] pattern, int filterFlags, const int[] clients, int numClients);
```

## Credits

Based on BotoX's DynamicTargeting, which needed a patched SourceMod core ([alliedmodders/sourcemod#1069](https://github.com/alliedmodders/sourcemod/pull/1069)).
