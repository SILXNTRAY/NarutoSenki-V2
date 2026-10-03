#pragma once

namespace UiFlowKeys
{
static constexpr const char *kEnterSelectLayer = "enterSelectLayer";
static constexpr const char *kOnGameOver = "onGameOver";
static constexpr const char *kBackToStartMenu = "backToStartMenu";
static constexpr const char *kCreditsBackToStartMenu = "CreditsLayer_BackToStartMenu";
} // namespace UiFlowKeys

// Lua globals implemented in lua/ui/HudLayer.lua (first argument is the HudLayer).
namespace HudFlowKeys
{
static constexpr const char *kInitStatusBars = "HudLayer_InitStatusBars";
static constexpr const char *kSetHP = "HudLayer_SetHP";
static constexpr const char *kSetEXP = "HudLayer_SetEXP";
static constexpr const char *kLayoutControls = "HudLayer_LayoutControls";
} // namespace HudFlowKeys

// Lua globals implemented in lua/ui/StartMenu.lua (main menu).
// First argument is the object named in the comment.
namespace StartMenuFlowKeys
{
// (StartMenu, versionCode) - builds the static decoration: ground, clouds, bars, title, version label, avatar.
static constexpr const char *kInitDecor = "StartMenu_InitDecor";
// (StartMenu) - positions the C++-owned controls (carousel buttons, menu text, news/login, notice) and starts the notice marquee.
static constexpr const char *kLayoutControls = "StartMenu_LayoutControls";
// (MenuButton, fromSlot, toSlot) - animates one carousel button from one slot to the next. Slot values: MenuSlot in StartMenu.h.
static constexpr const char *kMoveButton = "StartMenu_MoveButton";
} // namespace StartMenuFlowKeys

// Lua globals implemented in lua/ui/CreditsLayer.lua (credits screen, fully Lua).
namespace CreditsFlowKeys
{
// (CreditsLayer) - builds the whole screen: background, clouds, bars, title, credit sheets, return button, music.
static constexpr const char *kInit = "CreditsLayer_Init";
} // namespace CreditsFlowKeys

// Lua globals implemented in lua/ui/GearLayer.lua (in-game gear shop).
// First argument is the object named in the comment.
namespace GearFlowKeys
{
// (GearLayer) - builds the dimmer / shop panel and lays out every control C++ created (bars, menus, clipper, scroll list).
static constexpr const char *kLayoutControls = "GearLayer_LayoutControls";
// (GearLayer) - positions the row of gears the player already owns.
static constexpr const char *kLayoutCurrentGears = "GearLayer_LayoutCurrentGears";
// (GearLayer, gearType, updateBigIcon) - shows the detail card (and optionally the big icon) of a gear.
static constexpr const char *kShowDetail = "GearLayer_ShowGearDetail";
// (GearButton, gearType, buttonType, isBuyed, slot) - builds the icon / "sold out" overlay and places the button.
// buttonType values: GearButtonType in GearLayer.h (Buy = 0, Sell = 1).
static constexpr const char *kDecorateButton = "GearLayer_DecorateButton";
} // namespace GearFlowKeys
