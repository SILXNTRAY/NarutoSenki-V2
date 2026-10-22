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
// (HudLayer, isMobile, playerIsKonoha, scaleX) - places the action buttons, the top bar (map + pause
// button, kills / deaths, team score, gold, clock, status bar background).
static constexpr const char *kLayoutControls = "HudLayer_LayoutControls";
// (HudLayer) - places the three gear buttons as one group, right after C++ creates them.
static constexpr const char *kLayoutGear = "HudLayer_LayoutGear";
// (HudLayer) - places the character avatar button (opens the gear shop) when C++ creates it.
static constexpr const char *kLayoutAvatar = "HudLayer_LayoutAvatar";
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

// Lua globals implemented in lua/ui/GameModeLayer.lua (game mode select screen).
// First argument is the object named in the comment.
namespace GameModeFlowKeys
{
// (GameModeLayer) - builds background, bars, title and lays out every control C++ created (mode buttons, label, return menu).
static constexpr const char *kInit = "GameModeLayer_Init";
// (ModeMenuButton, useMask2) - builds the chain mask over a locked button and hands it back with setLockMask.
static constexpr const char *kDecorateLock = "GameModeLayer_DecorateLock";
} // namespace GameModeFlowKeys

// Lua globals implemented in lua/ui/PauseLayer.lua (pause screen).
// First argument is the PauseLayer.
namespace PauseFlowKeys
{
// (PauseLayer) - builds the dimmer, menu bars, title and the "surrender" hint over the snapshot C++ already added.
static constexpr const char *kInitDecor = "PauseLayer_InitDecor";
// (PauseLayer) - aligns and positions the menus C++ created (pause, sound, preload).
static constexpr const char *kLayoutControls = "PauseLayer_LayoutControls";
// (PauseLayer) - builds the "back to menu" confirm dialog around the exit layer / confirm menu C++ created.
static constexpr const char *kBuildConfirm = "PauseLayer_BuildConfirm";
} // namespace PauseFlowKeys

// Lua globals implemented in lua/ui/GameOver.lua (result screen).
// First argument is the GameOver layer. All values are computed by C++ (scores,
// rewards, records, grade); Lua only draws them.
namespace GameOverFlowKeys
{
// (GameOver) - builds dimmer, menu bars, title and places / scales the result panel C++ created.
static constexpr const char *kInit = "GameOver_Init";
// (GameOver, heroName, clockText) - the player's half-body portrait, the list background and the clock.
static constexpr const char *kShowFrame = "GameOver_ShowFrame";
// (GameOver, heroName, isKonoha, index, kills, deaths, flogs, gearsCsv) - one scoreboard row; gearsCsv is e.g. "0,4,7".
static constexpr const char *kAddHeroRow = "GameOver_AddHeroRow";
// (GameOver, rewardText, extraText) - the coin reward plate.
static constexpr const char *kShowReward = "GameOver_ShowReward";
// (GameOver, frameName) - the grade picture (S / A / Defeat ...).
static constexpr const char *kShowGrade = "GameOver_ShowGrade";
// (GameOver, versionText) - version label; also places the close menu C++ created.
static constexpr const char *kShowFooter = "GameOver_ShowFooter";
// (GameOver) - builds the "back to menu" confirm dialog around the exit layer / confirm menu C++ created.
static constexpr const char *kBuildConfirm = "GameOver_BuildConfirm";
} // namespace GameOverFlowKeys

// Lua globals implemented in lua/ui/LoadLayer.lua (loading screen before a match).
// First argument is the LoadLayer. Asset loading, the timers and starting the
// match stay in C++; Lua only draws.
namespace LoadFlowKeys
{
// (LoadLayer) - menu bars, title and the two moving clouds.
static constexpr const char *kInitDecor = "LoadLayer_InitDecor";
// (LoadLayer, imageName) - full-screen background image (blue / red).
static constexpr const char *kSetBackground = "LoadLayer_SetBackground";
// (LoadLayer, heroName, slot, frameCount) - one walking hero at the bottom right.
// The hero's Walk frames must already be loaded; frameCount 1 means "standing still".
static constexpr const char *kAddAvatar = "LoadLayer_AddAvatar";
// (LoadLayer, tipNumber) - the tip picture (tip{N}.png) in the middle of the screen.
static constexpr const char *kShowTip = "LoadLayer_ShowTip";
// (LoadLayer) - the blinking "loading" text.
static constexpr const char *kShowLoadingText = "LoadLayer_ShowLoadingText";
} // namespace LoadFlowKeys
