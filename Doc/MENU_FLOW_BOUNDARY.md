# Menu/UI Source-of-Truth Boundary

This project keeps menu presentation split between Lua and C++. To reduce drift:

- Lua is the source-of-truth for scene transitions (`enterSelectLayer`, `onGameOver`, `backToStartMenu`).
- C++ is responsible for input/widget state and game-mode selection payload.

## Callback Map

- `GameModeLayer` -> `enterSelectLayer(mode, enableCustomSelect)`
- `GameLayer` -> `onGameOver()`
- `SelectLayer` -> `backToStartMenu()`
- `CreditsLayer` -> `CreditsLayer_BackToStartMenu()`

All callback keys are centralized in `Classes/Constants/UiFlowKeys.hpp`.

## Rule

When adding a new menu callback:
1. Add key in `UiFlowKeys.hpp`.
2. Implement Lua global function in `lua/ui/StartMenu.lua` (or related scene file).
3. Call using bridge helper from C++, avoid hardcoded string literals.

## Mid-game HUD (`HudLayer`)

`HudLayer` is created by C++ (`LoadLayer::onLoadFinish`) but its presentation is driven by `lua/ui/HudLayer.lua`:

- **Lua-owned:** HP bar, HP mark, XP bar and their labels (creation, rotation, percentage, text).
  Lua hands them back via `HudLayer::setStatusBars(...)`, which fills the `status_hpbar` / `status_hpMark` / `status_expbar` / `hpLabel` / `expLabel` members, so existing C++ code that touches them (e.g. `setOpacity`) keeps working.
- **C++ logic, Lua layout:** attack button, Skill 1-5, Item 1-4 and the minimap layer keep their behaviour in C++; Lua sets their position, scale and rotation (layout tables in `HudLayer.lua`).

Callbacks (keys in `HudFlowKeys`, first argument is the `HudLayer`): `HudLayer_InitStatusBars`, `HudLayer_SetHP`, `HudLayer_SetEXP`, `HudLayer_LayoutControls`.
`HudLayer`/`ActionButton` are exposed to Lua in `tools/tolua++/game/NarutoSenki.pkg`; regenerate `LuaCocos2d.cpp` with `tools/tolua++/build.*` after changing it
(note: the committed `LuaCocos2d.cpp` carries a hand-added macOS GL-constants block after the `using namespace` lines that the generator does not emit; re-apply it).
