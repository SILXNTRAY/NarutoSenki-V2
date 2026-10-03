--
-- HudLayer (mid-game HUD)
--
-- The layer instance is still created by C++ (LoadLayer::onLoadFinish), because
-- GameLayer / CharacterBase / HPBar hold a HudLayer* and call into it.  What
-- moved to Lua:
--
--   * HP bar, HP mark, XP (EXP) bar and their labels: created, updated and
--     animated here (rotation, percentage, text).
--   * Position / scale / rotation of every control whose behaviour stays in
--     C++ (attack button, Skill 1-5, Item 1-4, minimap layer).  Edit the
--     layout tables below to move things; no C++ rebuild is needed.
--
-- Entry points called from C++ (see Classes/Constants/UiFlowKeys.hpp,
-- namespace HudFlowKeys):
--
--   HudLayer_InitStatusBars(hud, hp, exp, lv)
--   HudLayer_SetHP(hud, hpPercent, hp)
--   HudLayer_SetEXP(hud, exp, lv)
--   HudLayer_LayoutControls(hud, isMobile)
--
-- The bars are handed back to C++ with hud:setStatusBars(...), so C++ code that
-- still touches them (e.g. status_hpbar->setOpacity()) keeps working. C++-owned
-- controls are reached through getters (hud:getSkill1Button(), ...).
--

-- Must match DESKTOP_UI_SCALE in Classes/HudLayer.h (ActionButton masks use it).
HudLayer.DESKTOP_UI_SCALE = 0.8

local FONT_DEFAULT = 'Fonts/1.fnt'

local EXP_PER_LEVEL = 500
local EXP_MAX = 2500

-- HP bar: full -> empty sweeps the bar through this many degrees (0 .. -180).
local HP_SWEEP_DEGREES = 180
local HP_ROTATE_TIME = 0.2

--
-- Status bars (fully Lua)
--

-- Static layout of the status bars. y values are measured from the TOP of the
-- screen (converted with display.height), matching the original C++ code.
local STATUS = {
    hpbar = {x = 53, fromTop = 54, z = 40},
    hpMark = {x = 54, fromTop = 105, z = 45},
    expbar = {x = 54, fromTop = 54, z = 50},
    hpLabel = {x = 0, fromTop = 54, z = 5000, scale = 0.35},
    expLabel = {x = 94, fromTop = 54, z = 5000, scale = 0.35}
}

local function expPercent(exp, lv)
    local percent = (exp - (lv - 1) * EXP_PER_LEVEL) / EXP_PER_LEVEL * 100
    if percent > 100 then percent = 100 end
    if percent < 0 then percent = 0 end
    return percent
end

function HudLayer:initStatusBars(hp, exp, lv)
    local h = display.height

    -- HP bar (rotates as HP drops)
    local hpbar = display.newSprite('#status_hpbar.png')
    hpbar:setPosition(STATUS.hpbar.x, h - STATUS.hpbar.fromTop)
    self:addChild(hpbar, STATUS.hpbar.z)

    local hpMark = display.newSprite('#status_hpMark.png')
    hpMark:setAnchorPoint(0, 0)
    hpMark:setPosition(STATUS.hpMark.x, h - STATUS.hpMark.fromTop)
    self:addChild(hpMark, STATUS.hpMark.z)

    -- XP bar (radial progress timer, fills in reverse)
    local expbar = display.newProgressTimer(
                       display.newSprite('#status_ckrbar.png'),
                       display.PROGRESS_TIMER_RADIAL)
    expbar:setPercentage(0)
    expbar:setReverseDirection(true)
    expbar:setPosition(STATUS.expbar.x, h - STATUS.expbar.fromTop)
    self:addChild(expbar, STATUS.expbar.z)

    -- Labels
    local hpLabel = CCLabelBMFont:create(tostring(math.floor(hp)), FONT_DEFAULT)
    hpLabel:setScale(STATUS.hpLabel.scale)
    hpLabel:setAnchorPoint(0, 0)
    hpLabel:setPosition(STATUS.hpLabel.x, h - STATUS.hpLabel.fromTop)
    self:addChild(hpLabel, STATUS.hpLabel.z)

    local expLabel = CCLabelBMFont:create(
                         string.format('%d%%', math.floor(expPercent(exp, lv))),
                         FONT_DEFAULT)
    expLabel:setScale(STATUS.expLabel.scale)
    expLabel:setAnchorPoint(0.5, 0)
    expLabel:setPosition(STATUS.expLabel.x, h - STATUS.expLabel.fromTop)
    self:addChild(expLabel, STATUS.expLabel.z)

    -- Hand the nodes back to C++ (it still touches them, e.g. setOpacity) and
    -- keep Lua-side references for the update functions.
    self:setStatusBars(hpbar, hpMark, expbar, hpLabel, expLabel)
    self._hpbar, self._expbar = hpbar, expbar
    self._hpLabel, self._expLabel = hpLabel, expLabel

    -- Make the bars reflect a non-default starting state too.
    self:setEXP(exp, lv)
end

-- percent: current HP / max HP in [0, 1]; hp: absolute HP for the label.
function HudLayer:setHP(percent, hp)
    local bar = self._hpbar
    if not bar or not self._hpLabel then return end

    local angle = -((1 - percent) * HP_SWEEP_DEGREES)
    bar:runAction(CCRotateTo:create(HP_ROTATE_TIME, angle))
    self._hpLabel:setString(tostring(math.floor(hp)))
end

function HudLayer:setEXP(exp, lv)
    local bar = self._expbar
    if not bar or not self._expLabel then return end

    local percent = expPercent(exp, lv)
    -- The radial timer only shows the second half of the sweep: 50% .. 100%.
    bar:setPercentage((1 + percent / 100) * 50)

    if exp >= EXP_MAX then
        bar:setPercentage(100)
        self._expLabel:setString('Max')
    else
        self._expLabel:setString(string.format('%d%%', math.floor(percent)))
    end
end

--
-- Layout for controls whose logic stays in C++
--
-- Every entry returns {x, y, scale, rotation}; scale / rotation are optional.
-- `c` is a context with the screen size (w, h) and the C++ controls, so
-- positions can be derived from other controls' size/position exactly as the
-- C++ code did.
--

local function size(node) return node:getContentSize() end

local function mobileLayout(c)
    local w = c.w
    local nAttack, s1, s2, s3, s4, s5 = c.nAttackButton, c.skill1Button,
                                        c.skill2Button, c.skill3Button,
                                        c.skill4Button, c.skill5Button

    local L = {}
    L.nAttackButton = {x = w - 60, y = 8}
    L.skill1Button = {x = w - size(s1).width - 64, y = 2}
    L.skill2Button = {x = w - 95, y = 50}
    L.skill3Button = {x = w - 44, y = 8 + size(nAttack).height + 8}
    L.skill4Button = {x = L.skill1Button.x - size(s4).width - 8, y = 2}
    L.skill5Button = {x = L.skill4Button.x - size(s5).width - 8, y = 2}

    -- Items
    L.item1Button = {x = 8, y = L.skill3Button.y + size(s3).height}
    L.item2Button = {x = w - 44, y = L.skill3Button.y + size(s3).height + 8}
    L.item3Button = {x = L.skill5Button.x - size(s5).width - 8, y = 2}
    L.item4Button = {x = L.skill2Button.x - size(s2).width - 8, y = L.skill2Button.y}
    return L
end

local function desktopLayout(c)
    local w = c.w
    local nAttack, s1 = c.nAttackButton, c.skill1Button
    local width = size(s1).width
    local scale = HudLayer.DESKTOP_UI_SCALE

    local L = {}
    -- Attack button is parked off-screen on desktop (keyboard controls).
    L.nAttackButton = {x = w + 100, y = -100}

    L.skill1Button = {x = w / 2 - (width + 8) * 2, y = 2, scale = scale}
    L.skill2Button = {x = w / 2 - width - 8, y = 2, scale = scale}
    L.skill3Button = {x = w / 2, y = 2, scale = scale}
    L.skill4Button = {x = w / 2 + width + 8, y = 2, scale = scale}
    L.skill5Button = {x = w / 2 + (width + 8) * 2, y = 2, scale = scale}

    -- Ramen item
    L.item1Button = {
        x = 8,
        y = 8 + size(nAttack).height + size(s1).height,
        scale = scale
    }
    -- First item (Speed up & Stealth)
    L.item3Button = {x = w - width - 64, y = 2, scale = scale}
    -- Second item (Kawarimi)
    L.item4Button = {x = w - 95, y = 50, scale = scale}
    -- Third item (Trap)
    L.item2Button = {x = w - 44, y = 8 + size(nAttack).height + 8, scale = scale}
    return L
end

-- Minimap / tower & player icons container. Icons inside are positioned by C++.
local function miniLayerLayout(c) return {x = c.w - 112, y = c.h - 38} end

-- layout key -> C++ accessor (members are exposed to Lua through getters)
local GETTERS = {
    nAttackButton = 'getNAttackButton',
    skill1Button = 'getSkill1Button',
    skill2Button = 'getSkill2Button',
    skill3Button = 'getSkill3Button',
    skill4Button = 'getSkill4Button',
    skill5Button = 'getSkill5Button',
    item1Button = 'getItem1Button',
    item2Button = 'getItem2Button',
    item3Button = 'getItem3Button',
    item4Button = 'getItem4Button'
}
local CONTROLS = {
    'nAttackButton', 'skill1Button', 'skill2Button', 'skill3Button',
    'skill4Button', 'skill5Button', 'item1Button', 'item2Button',
    'item3Button', 'item4Button'
}

local function applyTransform(node, spec)
    if not node or not spec then return end
    node:setPosition(spec.x, spec.y)
    if spec.scale then node:setScale(spec.scale) end
    if spec.rotation then node:setRotation(spec.rotation) end
end

function HudLayer:layoutControls(isMobile)
    local c = {w = display.width, h = display.height}
    for _, name in ipairs(CONTROLS) do c[name] = self[GETTERS[name]](self) end

    local layout = isMobile and mobileLayout(c) or desktopLayout(c)
    for _, name in ipairs(CONTROLS) do applyTransform(c[name], layout[name]) end

    applyTransform(self:getMiniLayer(), miniLayerLayout(c))
end

--
-- Global entry points (called from C++ through LuaBridge)
--

function HudLayer_InitStatusBars(hud, hp, exp, lv)
    hud:initStatusBars(hp, exp, lv)
end

function HudLayer_SetHP(hud, percent, hp) hud:setHP(percent, hp) end

function HudLayer_SetEXP(hud, exp, lv) hud:setEXP(exp, lv) end

function HudLayer_LayoutControls(hud, isMobile) hud:layoutControls(isMobile) end
