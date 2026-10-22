--
-- GameOver (result screen)
--
-- The layer instance is created by C++ (GameLayer game over flow).  Everything
-- that *does* something stays there: the score / grade / reward calculation,
-- the anti-cheat checks, saving coins, win counts and best times, handing out
-- rewards, the game mode hooks, the sounds, the touch callbacks and leaving
-- the screen.  C++ passes the finished numbers to Lua.  What lives in Lua:
--
--   * The dimmer over the frozen game screenshot, the menu bars and title.
--   * The result panel: where it sits and its starting scale (C++ plays the
--     zoom-in and starts the listing when it ends).
--   * The portrait, list background and clock.
--   * Every scoreboard row: avatar, kills / deaths / flogs and the gear icons.
--   * The coin reward plate, the grade picture and the version label.
--   * The position of the close button and of the confirm dialog's yes / no.
--
-- Like HudLayer, every element has its own entry in one of the layout
-- functions below.  An entry is { x, y, anchor, scale, rotation, opacity, flipX,
-- flipY, visible, z } (see ui/Placement.lua); x / y are measured from the
-- bottom-left corner of the screen.  Most elements hang off the result panel,
-- so the context `c.panel` gives its edges (left, right, bottom, top, cx, cy).
-- Edit them, no C++ rebuild is needed.
--
-- Entry points called from C++ (see Classes/Constants/UiFlowKeys.hpp,
-- namespace GameOverFlowKeys):
--
--   GameOver_Init(layer)
--   GameOver_ShowFrame(layer, heroName, clockText)
--   GameOver_AddHeroRow(layer, heroName, isKonoha, index, kills, deaths, flogs, gearsCsv)
--   GameOver_ShowReward(layer, rewardText, extraText)
--   GameOver_ShowGrade(layer, frameName)
--   GameOver_ShowFooter(layer, versionText)
--   GameOver_BuildConfirm(layer)
--
-- C++-owned controls are reached through getters (layer:getResultBg(),
-- layer:getCloseButton(), ...).
--
ns.GameOver = GameOver

local Placement = ns.Placement

local FONT_DEFAULT = 'Fonts/1.fnt'
local FONT_YELLOW = 'Fonts/yellow.fnt'

-- Heroes whose portrait faces the wrong way and has to be mirrored.
local FLIPPED_PORTRAITS = {
    Konan = true,
    Karin = true,
    Suigetsu = true,
    Hidan = true,
    Tobirama = true,
    Tsunade = true,
    Kankuro = true,
    SageJiraiya = true,
    Minato = true,
    Tobi = true,
    Lee = true,
    RockLee = true,
    Hinata = true,
    Asuma = true,
    Chiyo = true,
    Kisame = true
}

local function size(node) return node:getContentSize() end

-- Edges of the result panel C++ created (read when needed: it is placed first).
local function panelContext(layer)
    local bg = layer:getResultBg()
    local s = size(bg)
    local cx, cy = bg:getPositionX(), bg:getPositionY()
    return {
        w = display.width,
        h = display.height,
        panel = {
            left = cx - s.width / 2,
            right = cx + s.width / 2,
            bottom = cy - s.height / 2,
            top = cy + s.height / 2,
            cx = cx,
            cy = cy,
            width = s.width,
            height = s.height
        }
    }
end

--
-- Layout
--

-- Static decoration and the panel itself.
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
    -- the panel zooms in from half size (C++ runs the zoom)
    L.panel = {x = w / 2, y = h / 2 - 6, scale = 0.5}
    return L
end

-- Portrait, list background and clock.
local function frameLayout(c)
    local p = c.panel

    local L = {}
    L.portrait = {
        x = p.right - size(c.portrait).width,
        y = p.bottom + 9,
        anchor = {0, 0},
        flipX = c.flipPortrait,
        z = 5
    }
    L.list = {x = p.left + 2, y = p.bottom + 26, anchor = {0, 0}, z = 5}

    local plateW = size(c.timePlate).width
    L.timePlate = {
        x = p.right - 11 - plateW,
        y = p.bottom + 46,
        anchor = {0, 0},
        z = 6
    }
    L.clock = {
        x = L.timePlate.x + plateW / 2,
        y = L.timePlate.y + 3,
        anchor = {0.5, 0},
        scale = 0.48,
        z = 7
    }
    return L
end

-- One scoreboard row.  Konoha heroes fill the upper block (index 0, 1, 2),
-- the others the lower one (index 3, 4, 5).  `r` is { avatar, isKonoha, index }.
local function rowLayout(c, r)
    local p = c.panel
    local step = 25

    local x = p.left + 2
    local y
    if r.isKonoha then
        y = p.bottom + 152 - r.index * step
    else
        y = p.bottom + 76 - (r.index - 3) * step
    end
    local numberY = y + size(r.avatar).height / 2

    local L = {}
    L.avatar = {x = x, y = y, anchor = {0, 0}, z = 7}
    L.kills = {x = x + 44, y = numberY, scale = 0.3, z = 7}
    L.deaths = {x = x + 68, y = numberY, scale = 0.3, z = 7}
    L.flogs = {x = x + 94, y = numberY, scale = 0.3, z = 7}
    -- gear icons follow the flog number, one every 19 px
    L.gear = function(slot)
        return {
            x = x + 94 + 22 + slot * 19,
            y = numberY - 1,
            scale = 0.5,
            z = 7
        }
    end
    return L
end

-- Coin reward plate and its labels.
local function rewardLayout(c)
    local p = c.panel

    local plateX = p.right - size(c.coinPlate).width - 11
    local plateY = p.bottom + 12

    local L = {}
    L.coinPlate = {x = plateX, y = plateY, anchor = {0, 0}, z = 6}
    L.extra = {x = plateX + 70, y = plateY + 22, anchor = {0.5, 0}, z = 7}
    L.extraLabel = {
        x = plateX + 68,
        y = plateY + 3,
        anchor = {0.5, 0},
        scale = 0.5,
        z = 7
    }
    L.rewardLabel = {
        x = plateX + 28,
        y = plateY + 3,
        anchor = {0.5, 0},
        scale = 0.55,
        z = 7
    }
    return L
end

-- Grade picture (S / A / Defeat ...).
local function gradeLayout(c)
    local p = c.panel
    return {
        grade = {
            x = p.right - size(c.grade).width - 12,
            y = p.bottom + 88,
            anchor = {0, 0},
            z = 7
        }
    }
end

-- Version label and the close button (C++ owns the button).
local function footerLayout(c)
    local p = c.panel

    local L = {}
    L.version = {x = c.w / 2 + 94, y = p.bottom + 6, scale = 0.3, z = 5}
    -- measured from the screen centre, like the original
    L.closeButton = {
        x = c.w / 2 + p.width / 2 - 12,
        y = c.h / 2 + p.height / 2 - 18
    }
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
-- Decoration and panel
--

function GameOver:initDecor()
    local c = {w = display.width, h = display.height}

    -- the frozen screenshot C++ added is anchored at the corner
    self:getSnapshotBg():setAnchorPoint(0, 0)

    c.barBottom = CCSprite:create('menu_bar2.png')
    c.barTop = CCSprite:create('menu_bar3.png')
    c.title = display.newSprite('#result_title.png')

    local L = decorLayout(c)

    c.dimmer = CCLayerColor:create(ccc4(0, 0, 0, L.dimmer.opacity), c.w, c.h)
    self:addChild(c.dimmer, L.dimmer.z)

    Placement.add(self, c.barBottom, L.barBottom)
    Placement.add(self, c.barTop, L.barTop)
    Placement.add(self, c.title, L.title)
    Placement.apply(self:getResultBg(), L.panel)
end

--
-- Portrait, list background and clock
--

function GameOver:showFrame(heroName, clockText)
    local c = panelContext(self)

    c.portrait = display.newSprite(string.format('#%s_half.png', heroName))
    c.flipPortrait = FLIPPED_PORTRAITS[heroName] and true or false
    c.list = display.newSprite('#list_bg.png')
    c.timePlate = display.newSprite('#time_bg.png')
    c.clock = CCLabelBMFont:create(clockText, FONT_DEFAULT)

    local L = frameLayout(c)

    Placement.add(self, c.portrait, L.portrait)
    Placement.add(self, c.list, L.list)
    Placement.add(self, c.timePlate, L.timePlate)
    Placement.add(self, c.clock, L.clock)
end

--
-- Scoreboard rows
--

function GameOver:addHeroRow(heroName, isKonoha, index, kills, deaths, flogs, gearsCsv)
    local c = panelContext(self)

    local r = {
        avatar = display.newSprite(string.format('#%s_small.png', heroName)),
        isKonoha = isKonoha,
        index = index
    }
    local L = rowLayout(c, r)

    local killLabel = CCLabelBMFont:create(tostring(kills), FONT_DEFAULT)
    local deathLabel = CCLabelBMFont:create(tostring(deaths), FONT_DEFAULT)
    local flogLabel = CCLabelBMFont:create(tostring(flogs), FONT_DEFAULT)

    -- gears the hero owns
    local slot = 0
    for gear in string.gmatch(gearsCsv, '%d+') do
        local icon = display.newSprite(string.format('#gear_%02d.png', tonumber(gear)))
        Placement.add(self, icon, L.gear(slot))
        slot = slot + 1
    end

    Placement.add(self, r.avatar, L.avatar)
    Placement.add(self, killLabel, L.kills)
    Placement.add(self, deathLabel, L.deaths)
    Placement.add(self, flogLabel, L.flogs)
end

--
-- Reward plate and grade
--

function GameOver:showReward(rewardText, extraText)
    local c = panelContext(self)

    c.coinPlate = display.newSprite('#coin_bg.png')
    c.extra = display.newSprite('#adExtra.png')
    c.extraLabel = CCLabelBMFont:create(extraText, FONT_YELLOW)
    c.rewardLabel = CCLabelBMFont:create(rewardText, FONT_YELLOW)

    local L = rewardLayout(c)

    Placement.add(self, c.coinPlate, L.coinPlate)
    Placement.add(self, c.extra, L.extra)
    Placement.add(self, c.extraLabel, L.extraLabel)
    Placement.add(self, c.rewardLabel, L.rewardLabel)
end

function GameOver:showGrade(frameName)
    local c = panelContext(self)
    c.grade = display.newSprite('#' .. frameName)

    local L = gradeLayout(c)
    Placement.add(self, c.grade, L.grade)
end

--
-- Footer: version label and the close button
--

function GameOver:showFooter(versionText)
    local c = panelContext(self)
    c.version = CCLabelBMFont:create(versionText, FONT_DEFAULT)

    local L = footerLayout(c)

    Placement.add(self, c.version, L.version)

    -- the button is placed on screen directly, so its menu sits at the origin
    self:getOverMenu():setPosition(0, 0)
    Placement.apply(self:getCloseButton(), L.closeButton)
end

--
-- Confirm dialog ("back to menu").  C++ has already added the exit layer and
-- the yes / no menu to it; this adds the look around it and places the buttons.
--

function GameOver:buildConfirm()
    local c = {
        w = display.width,
        h = display.height,
        yesButton = self:getYesButton(),
        noButton = self:getNoButton()
    }
    local exitLayer = self:getExitLayer()

    c.background = display.newSprite('#confirm_bg.png')
    c.title = display.newSprite('#confirm_title.png')
    c.text = display.newSprite('#btm_text.png')

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

function GameOver_Init(layer) layer:initDecor() end

function GameOver_ShowFrame(layer, heroName, clockText)
    layer:showFrame(heroName, clockText)
end

function GameOver_AddHeroRow(layer, heroName, isKonoha, index, kills, deaths, flogs, gearsCsv)
    layer:addHeroRow(heroName, isKonoha, index, kills, deaths, flogs, gearsCsv)
end

function GameOver_ShowReward(layer, rewardText, extraText)
    layer:showReward(rewardText, extraText)
end

function GameOver_ShowGrade(layer, frameName) layer:showGrade(frameName) end

function GameOver_ShowFooter(layer, versionText) layer:showFooter(versionText) end

function GameOver_BuildConfirm(layer) layer:buildConfirm() end
