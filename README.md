# sm-ext-DynamicTargeting

When a command target is ambiguous (a partial name matches more than one player), shows the admin a menu to pick the intended player, then re-runs the command on that player.

Works with every plugin that uses `ProcessTargetString` / `FindTarget`, without modifying or recompiling them, and without patching SourceMod core.
