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
