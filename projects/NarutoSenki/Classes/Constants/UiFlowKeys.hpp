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
