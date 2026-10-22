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
--     C++ (attack button, Skill 1-5, Item 1-4).  Edit the layout tables below
--     to move things; no C++ rebuild is needed.
--   * The top bar: map + pause button, kills / deaths, team score, gold, clock
--     and status bar background.  Related elements are GROUPS that move as one
--     (see "Top bar groups" below).
--   * The three gear buttons, as one group with three layouts (default,
--     vertical, horizontal).
--   * The character avatar button in the top-left corner (opens the gear shop).
--   * Where the gear sign and the freeze overlays sit on their buttons (ns.HudMarks).
--
-- Entry points called from C++ (see Classes/Constants/UiFlowKeys.hpp,
-- namespace HudFlowKeys):
--
--   HudLayer_InitStatusBars(hud, hp, exp, lv)
--   HudLayer_SetHP(hud, hpPercent, hp)
--   HudLayer_SetEXP(hud, exp, lv)
--   HudLayer_LayoutControls(hud, isMobile, playerIsKonoha, scaleX)
--   HudLayer_LayoutGear(hud)
--   HudLayer_LayoutAvatar(hud)
--
-- The bars are handed back to C++ with hud:setStatusBars(...), so C++ code that
-- still touches them (e.g. status_hpbar->setOpacity()) keeps working. C++-owned
-- controls are reached through getters (hud:getSkill1Button(), ...).
--

-- Must match DESKTOP_UI_SCALE in Classes/HudLayer.h (ActionButton masks use it).
HudLayer.DESKTOP_UI_SCALE = 0.8

-- Where the small overlays sit on an action button.  Each entry is an offset (x, y) in button
-- units from the button's BOTTOM-LEFT corner.  C++ (ActionButton) reads this table when it
-- creates the overlay; the numbers below are what the game did before this table existed:
--   gearSign     the gear badge on the three gear buttons while gears are enabled.
--                (x, y) is the badge's CENTRE.
--   skillFreeze  skill_freeze.png: the cooldown overlay on skill and item buttons and the lock
--                on the ougis buttons.  (x, y) is the overlay's bottom-left corner.
--   gearFreeze   gear_freeze.png: the cooldown overlay on the gear buttons.  Bottom-left corner.
-- The offset is multiplied by the button's scale, so it follows desktop / group scaling.
ns.HudMarks = {
    gearSign = {x = 17, y = 17},
    skillFreeze = {x = 0, y = 0},
    gearFreeze = {x = 0, y = 0}
}

local FONT_DEFAULT = 'Fonts/1.fnt'

local EXP_PER_LEVEL = 500
local EXP_MAX = 2500

-- How the HP and XP bars react to HP / XP:
--   'rotate'  the original look: the HP bar swings through HP_SWEEP_DEGREES and the
--             XP bar is a radial progress timer.
--   'scale'   nothing rotates: each bar's WIDTH (scale X) follows its percentage and
--             the height never changes. Positions and anchors come from STATUS_SCALE.
local BAR_MODE = 'rotate'

-- HP bar (rotate mode): full -> empty sweeps the bar through this many degrees (0 .. -180).
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

-- Scale mode: where the two bars sit. `anchor` is the point of the bar that stays put while
-- it shrinks: {0, 0.5} = left edge, {1, 0.5} = right edge, {0.5, 0.5} = it shrinks toward its
-- middle (the default, which for the built-in half-discs is the ring's centre line).
local STATUS_SCALE = {
    hpbar = {x = 53, fromTop = 54, anchor = {0.5, 0.5}},
    expbar = {x = 54, fromTop = 54, anchor = {0.5, 0.5}}
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

    -- Scale mode: same two bars, but placed / anchored from STATUS_SCALE and never rotated
    if BAR_MODE == 'scale' then
        for name, bar in pairs({hpbar = hpbar, expbar = expbar}) do
            local s = STATUS_SCALE[name]
            bar:setAnchorPoint(s.anchor[1], s.anchor[2])
            bar:setPosition(s.x, h - s.fromTop)
        end
        expbar:setPercentage(100) -- the timer stays full: the width is what changes
    end

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

    if BAR_MODE == 'scale' then
        -- only the width changes
        bar:runAction(CCScaleTo:create(HP_ROTATE_TIME, math.max(percent, 0), 1))
    else
        local angle = -((1 - percent) * HP_SWEEP_DEGREES)
        bar:runAction(CCRotateTo:create(HP_ROTATE_TIME, angle))
    end
    self._hpLabel:setString(tostring(math.floor(hp)))
end

function HudLayer:setEXP(exp, lv)
    local bar = self._expbar
    if not bar or not self._expLabel then return end

    local percent = expPercent(exp, lv)
    if BAR_MODE == 'scale' then
        -- only the width changes (a full bar at max)
        bar:setScaleX(exp >= EXP_MAX and 1 or percent / 100)
    else
        -- The radial timer only shows the second half of the sweep: 50% .. 100%.
        bar:setPercentage((1 + percent / 100) * 50)
    end

    if exp >= EXP_MAX then
        if BAR_MODE ~= 'scale' then bar:setPercentage(100) end
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

--
-- Top bar groups
--
-- A group moves and scales as one (see ui/Placement.lua): change its x / y
-- and every member follows; change `scale` to resize the whole group.  A member
-- sits at the group origin + (dx, dy) * scale.  Origins below are measured from
-- the bottom-left corner of the screen, so "w, h" is the top-right corner.
--
--   mapPause    the minimap and the pause button.
--   scoreboard  kills / deaths ("kd") and the team score ("team").  Move
--               `scoreboard` to move both, or `kd` / `team` (their x / y are
--               relative to `scoreboard`) to move just one.
--
-- Not grouped: gold (coinLabel), game clock and the status bar background.
--

local function topBarGroups(c)
    local w, h = c.w, c.h
    local G = {}

    G.mapPause = {
        x = w,
        y = h,
        scale = 1,
        members = {
            pauseButton = {dx = 0, dy = 0, anchor = {1, 1}},
            miniLayer = {dx = -112, dy = -38}
        }
    }

    -- team score: the player's own team is the left number, the other team the right one
    local leftTeam = c.playerIsKonoha and 'konoLabel' or 'akaLabel'
    local rightTeam = c.playerIsKonoha and 'akaLabel' or 'konoLabel'
    local team = {
        x = 0,
        y = 0,
        members = {
            colonLabel = {dx = -37, dy = -44, anchor = {0.5, 1}, scale = 0.5}
        }
    }
    team.members[leftTeam] = {dx = -42, dy = -46, anchor = {1, 1}, scale = 0.35}
    team.members[rightTeam] = {dx = -32, dy = -46, anchor = {0, 1}, scale = 0.35}

    local killIconX = -114
    local deadIconX = -114 + 26
    G.scoreboard = {
        x = w,
        y = h,
        scale = 1,
        groups = {
            kd = {
                x = 0,
                y = 0,
                members = {
                    killIcon = {dx = killIconX, dy = -46, anchor = {0, 1}},
                    killLabel = {
                        dx = killIconX + size(c.killIcon).width / 2 + 5,
                        dy = -56,
                        anchor = {0, 1},
                        scale = 0.26
                    },
                    deadIcon = {dx = deadIconX, dy = -47, anchor = {0, 1}},
                    deadLabel = {
                        dx = deadIconX + size(c.deadIcon).width / 2 + 5,
                        dy = -56,
                        anchor = {0, 1},
                        scale = 0.26
                    }
                }
            },
            team = team
        }
    }
    return G
end

-- Individual elements (each placed on its own).
local function topBarSingles(c)
    return {
        statusBar = {
            x = 0,
            y = c.h - size(c.statusBar).height,
            anchor = {0, 0}
        },
        gameClock = {x = c.scaleX * 25, y = 0, anchor = {0.5, 0}, scale = 0.4},
        coinLabel = {x = 121, y = c.h - 61, anchor = {0, 0}}
    }
end

function HudLayer:layoutTopBar(w, h, playerIsKonoha, scaleX)
    local Placement = ns.Placement

    local c = {
        w = w,
        h = h,
        playerIsKonoha = playerIsKonoha,
        scaleX = scaleX or 1,
        statusBar = self:getStatusBar(),
        pauseButton = self:getPauseButton(),
        miniLayer = self:getMiniLayer(),
        killIcon = self:getKillIcon(),
        killLabel = self:getKillLabel(),
        deadIcon = self:getDeadIcon(),
        deadLabel = self:getDeadLabel(),
        konoLabel = self:getKonoLabel(),
        akaLabel = self:getAkaLabel(),
        colonLabel = self:getColonLabel(),
        coinLabel = self:getCoinLabel(),
        gameClock = self:getGameClock()
    }

    -- the pause button is placed on screen directly, so its menu sits at the origin
    self:getPauseMenu():setPosition(0, 0)

    for _, group in pairs(topBarGroups(c)) do Placement.placeGroup(group, c) end

    local singles = topBarSingles(c)
    for name, spec in pairs(singles) do Placement.apply(c[name], spec) end
end

--
-- Gear buttons
--
-- The three gear buttons are one group, anchored to the top-left corner.
-- GEAR_MODE picks how they are laid out:
--
--   'default'     the original staggered arrow (left, lower middle, right)
--   'vertical'    a straight column, top to bottom
--   'horizontal'  a straight row, left to right
--
-- Move the whole group with GEAR_GROUP.x / .y (offsets from the top-left corner,
-- so y is negative to go down), resize it with .scale, and tune the spacing of
-- the straight layouts with GEAR_GAP (pixels between buttons).
--

local GEAR_MODE = 'default'
local GEAR_GAP = 5
local GEAR_GROUP = {x = 0, y = 0, scale = 1}

local function gearGroup(c)
    local first = -92 -- distance of the first button below the top edge
    local b = size(c.gear1Button)
    local members

    if GEAR_MODE == 'vertical' then
        local step = b.height + GEAR_GAP
        members = {
            gear1Button = {dx = 0, dy = first},
            gear2Button = {dx = 0, dy = first - step},
            gear3Button = {dx = 0, dy = first - 2 * step}
        }
    elseif GEAR_MODE == 'horizontal' then
        local step = b.width + GEAR_GAP
        members = {
            gear1Button = {dx = 0, dy = first},
            gear2Button = {dx = step, dy = first},
            gear3Button = {dx = 2 * step, dy = first}
        }
    else
        members = {
            gear1Button = {dx = 0, dy = first},
            gear2Button = {dx = 35, dy = first - 20},
            gear3Button = {dx = 70, dy = first}
        }
    end

    return {
        x = GEAR_GROUP.x,
        y = c.h + GEAR_GROUP.y,
        scale = GEAR_GROUP.scale,
        members = members
    }
end

function HudLayer:layoutGear()
    local c = {
        w = display.width,
        h = display.height,
        gear1Button = self:getGear1Button(),
        gear2Button = self:getGear2Button(),
        gear3Button = self:getGear3Button()
    }
    ns.Placement.placeGroup(gearGroup(c), c)
end


--
-- Avatar button
--
-- The character avatar in the top-left corner (<Name>_avator.png, the button
-- that opens the gear shop).  By default it hugs the top-left corner.
--

local function avatarLayout(c)
    return {avatar = {x = 0, y = c.h - size(c.avatar).height, anchor = {0, 0}}}
end

function HudLayer:layoutAvatar()
    local c = {w = display.width, h = display.height, avatar = self:getGearMenuSprite()}

    -- the button is placed on screen directly, so its menu sits at the origin
    self:getGearMenu():setPosition(0, 0)
    ns.Placement.apply(c.avatar, avatarLayout(c).avatar)
end

function HudLayer:layoutControls(isMobile, playerIsKonoha, scaleX)
    local c = {w = display.width, h = display.height}
    for _, name in ipairs(CONTROLS) do c[name] = self[GETTERS[name]](self) end

    local layout = isMobile and mobileLayout(c) or desktopLayout(c)
    for _, name in ipairs(CONTROLS) do applyTransform(c[name], layout[name]) end

    self:layoutTopBar(c.w, c.h, playerIsKonoha, scaleX)
end

--
-- Global entry points (called from C++ through LuaBridge)
--

function HudLayer_InitStatusBars(hud, hp, exp, lv)
    hud:initStatusBars(hp, exp, lv)
end

function HudLayer_SetHP(hud, percent, hp) hud:setHP(percent, hp) end

function HudLayer_SetEXP(hud, exp, lv) hud:setEXP(exp, lv) end

function HudLayer_LayoutControls(hud, isMobile, playerIsKonoha, scaleX)
    hud:layoutControls(isMobile, playerIsKonoha, scaleX)
end

function HudLayer_LayoutGear(hud) hud:layoutGear() end

function HudLayer_LayoutAvatar(hud) hud:layoutAvatar() end
