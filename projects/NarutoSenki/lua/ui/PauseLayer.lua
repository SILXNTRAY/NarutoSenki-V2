--
-- PauseLayer (pause screen)
--
-- The layer instance is created by C++ (GameLayer::onPause).  Everything that
-- *does* something stays there: pausing / resuming the audio, the BGM / voice /
-- preload toggles and their saved state, resuming the game, surrendering, the
-- selected state of the toggle buttons, and the touch callbacks of every button.
-- What lives in Lua:
--
--   * The dimmer drawn over the frozen game screenshot, the menu bars, the
--     title and the surrender hint text.
--   * The position of EVERY button C++ creates (resume, back to menu, BGM,
--     voice, preload, and the confirm dialog's yes / no), each on its own.
--   * The "back to menu" confirm dialog: background, title and text.
--
-- Like HudLayer, every element has its own entry in one of the layout
-- functions below.  An entry is { x, y, anchor, scale, rotation, opacity, flipX,
-- flipY, visible, z } (see ui/Placement.lua); x / y are measured from the
-- bottom-left corner of the screen.  Edit them, no C++ rebuild is needed.
--
-- Entry points called from C++ (see Classes/Constants/UiFlowKeys.hpp,
-- namespace PauseFlowKeys):
--
--   PauseLayer_InitDecor(layer)
--   PauseLayer_LayoutControls(layer)
--   PauseLayer_BuildConfirm(layer)
--
-- C++-owned controls are reached through getters (layer:getResumeButton(), ...).
--
ns.PauseLayer = PauseLayer

local Placement = ns.Placement

local function size(node) return node:getContentSize() end

--
-- Layout.  `c` carries the screen size (w, h) and the nodes involved, so
-- positions can be derived from their sizes.
--

-- Static decoration (all created in Lua).
local function decorLayout(c)
    local w, h = c.w, c.h

    local L = {}
    L.dimmer = {opacity = 150, z = 1}
    L.barBottom = {x = 0, y = 0, anchor = {0, 0}, stretch = true, z = 2}
    L.barTop = {
        x = 0,
        y = h - size(c.barTop).height,
        anchor = {0, 0},
        stretch = true,
        z = 2
    }
    L.title = {x = 2, y = h - size(c.title).height - 2, anchor = {0, 0}, z = 3}
    -- "surrender" hint, between the pause menu and the sound toggles
    L.hint = {x = w / 2, y = h / 2 - 23, z = 4}
    return L
end

-- Buttons created by C++.  Each button is placed on its own, by its centre.
local function controlLayout(c)
    local w, h = c.w, c.h

    local L = {}

    -- resume / back to menu, stacked with 26 px between them
    local stackY, stackGap = h / 2 + 30, 26
    local resumeH, backH = size(c.resumeButton).height, size(c.backButton).height
    L.resumeButton = {x = w / 2, y = stackY + (backH + stackGap) / 2}
    L.backButton = {x = w / 2, y = stackY - (resumeH + stackGap) / 2}

    -- BGM / voice, side by side with 25 px between them, under the stack
    local soundY, soundGap = stackY - 80, 25
    local bgmW, voiceW = size(c.bgmButton).width, size(c.voiceButton).width
    local soundLeft = w / 2 - (bgmW + voiceW + soundGap) / 2
    L.bgmButton = {x = soundLeft + bgmW / 2, y = soundY}
    L.voiceButton = {x = soundLeft + bgmW + soundGap + voiceW / 2, y = soundY}

    -- preload toggle, under the sound buttons
    L.preloadButton = {x = w / 2, y = h / 2 - 84}
    return L
end

-- "Back to menu" confirm dialog.
local function confirmLayout(c)
    local w, h = c.w, c.h

    local L = {}
    L.background = {x = w / 2, y = h / 2, z = 1}
    L.title = {x = w / 2, y = h / 2 + 38, z = 2}
    L.text = {x = w / 2, y = h / 2 + 8, z = 2}

    -- yes / no side by side with 24 px between them
    local rowY, gap = h / 2 - 30, 24
    local yesW, noW = size(c.yesButton).width, size(c.noButton).width
    local left = w / 2 - (yesW + noW + gap) / 2
    L.yesButton = {x = left + yesW / 2, y = rowY}
    L.noButton = {x = left + yesW + gap + noW / 2, y = rowY}
    return L
end

--
-- Decoration
--

function PauseLayer:initDecor()
    local c = {w = display.width, h = display.height}

    -- the frozen screenshot C++ added is anchored at the corner
    self:getSnapshotBg():setAnchorPoint(0, 0)

    c.barBottom = CCSprite:create('menu_bar2.png')
    c.barTop = CCSprite:create('menu_bar3.png')
    c.title = display.newSprite('#pause_title.png')
    c.hint = display.newSprite('#surrender_tips.png')

    local L = decorLayout(c)

    -- the dimmer is a colour layer: its alpha is the opacity of the spec
    c.dimmer = CCLayerColor:create(ccc4(0, 0, 0, L.dimmer.opacity), c.w, c.h)
    self:addChild(c.dimmer, L.dimmer.z)

    Placement.add(self, c.barBottom, L.barBottom)
    Placement.add(self, c.barTop, L.barTop)
    Placement.add(self, c.title, L.title)
    Placement.add(self, c.hint, L.hint)
end

--
-- Controls whose behaviour stays in C++
--

function PauseLayer:layoutControls()
    local c = {
        w = display.width,
        h = display.height,
        resumeButton = self:getResumeButton(),
        backButton = self:getBackButton(),
        bgmButton = self:getBgmButton(),
        voiceButton = self:getVoiceButton(),
        preloadButton = self:getPreloadButton()
    }

    -- buttons are placed on screen directly, so their menus sit at the origin
    self:getPauseMenu():setPosition(0, 0)
    self:getSoundMenu():setPosition(0, 0)
    self:getPreMenu():setPosition(0, 0)

    local L = controlLayout(c)
    for _, name in ipairs({
        'resumeButton', 'backButton', 'bgmButton', 'voiceButton', 'preloadButton'
    }) do Placement.apply(c[name], L[name]) end
end

--
-- Confirm dialog ("back to menu").  C++ has already added the exit layer and
-- the yes / no menu to it; this adds the look around it and places the buttons.
--

function PauseLayer:buildConfirm()
    local c = {
        w = display.width,
        h = display.height,
        yesButton = self:getYesButton(),
        noButton = self:getNoButton()
    }
    local exitLayer = self:getExitLayer()

    c.background = display.newSprite('#confirm_bg.png')
    c.title = display.newSprite('#confirm_title.png')
    c.text = display.newSprite('#surrender_text.png')

    local L = confirmLayout(c)

    -- the buttons are placed on screen directly, so their menu sits at the origin
    self:getConfirmMenu():setPosition(0, 0)
    Placement.apply(c.yesButton, L.yesButton)
    Placement.apply(c.noButton, L.noButton)

    Placement.add(exitLayer, c.background, L.background)
    Placement.add(exitLayer, c.title, L.title)
    Placement.add(exitLayer, c.text, L.text)
end

--
-- Global entry points (called from C++ through LuaBridge)
--

function PauseLayer_InitDecor(layer) layer:initDecor() end

function PauseLayer_LayoutControls(layer) layer:layoutControls() end

function PauseLayer_BuildConfirm(layer) layer:buildConfirm() end
