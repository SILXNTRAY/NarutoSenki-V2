--
-- GameModeLayer (game mode select screen)
--
-- The layer instance is created by C++ (GameModeLayer::create).  Everything
-- that *does* something stays there: the mode data (titles, descriptions,
-- locked state), touch handling and selection of the mode buttons, the label
-- text, the return button action, and the scene changes (back to the main
-- menu, on to the select screen).  What lives in Lua:
--
--   * The decoration: background, menu bars and title.
--   * Position of every control C++ creates: the mode buttons, the mode
--     label and the return menu.  Edit the LAYOUT table below to move things;
--     no C++ rebuild is needed.
--   * The chain mask drawn over locked mode buttons.
--
-- Entry points called from C++ (see Classes/Constants/UiFlowKeys.hpp,
-- namespace GameModeFlowKeys):
--
--   GameModeLayer_Init(layer)
--   GameModeLayer_DecorateLock(button, useMask2)
--
-- C++-owned controls are reached through getters (layer:getModeButton(i), ...).
--
ns.GameModeLayer = GameModeLayer

--
-- Layout (y values are measured from the bottom unless noted)
--

local LAYOUT = {
    title = {x = 2, fromTop = 2},
    label = {x = 10, y = 2},
    returnMenu = {fromRight = 38, y = 65},
    -- mode buttons: three columns of grids, see placeButton()
    grid = {
        width = 460, -- the block of buttons is centred using this width
        iconWidth = 100,
        centerOffsetY = 30, -- block centre sits this far above the screen centre
        -- first column: 3 wide buttons, stacked
        column1 = {count = 3, stepY = 55 + 7.5, topDy = 55 + 7.5},
        -- middle: 3 buttons in a row
        row = {count = 3, firstX = 10, stepX = 80 + 5, firstIndex = 2},
        -- last column: remaining buttons, stacked
        column2 = {x = 20 + (80 + 5) * 4, topDy = 47, stepY = 86 + 8}
    }
}

local Z = {background = -100, bars = 2, title = 3, label = 5, returnMenu = 5, lock = 1000}

local LOCK_MASK = 'GameMode/chain_mask.png'
local LOCK_MASK2 = 'GameMode/chain_mask2.png'

--
-- Decoration
--

function GameModeLayer:initDecor()
    local h = display.height

    -- background
    local bgSprite = CCSprite:create('red_bg.png')
    bgSprite:fullScreen()
    bgSprite:setAnchorPoint(0, 0)
    bgSprite:setPosition(0, 0)
    self:addChild(bgSprite, Z.background)

    -- menu bars (stretched to the full width)
    local barBottom = CCSprite:create('menu_bar2.png')
    barBottom:setAnchorPoint(0, 0)
    barBottom:fullScreen()
    self:addChild(barBottom, Z.bars)

    local barTop = CCSprite:create('menu_bar3.png')
    barTop:setAnchorPoint(0, 0)
    barTop:setPosition(0, h - barTop:getContentSize().height)
    barTop:fullScreen()
    self:addChild(barTop, Z.bars)

    -- title
    local title = display.newSprite('#startmenu_title.png')
    title:setAnchorPoint(0, 0)
    title:setPosition(LAYOUT.title.x,
                      h - title:getContentSize().height - LAYOUT.title.fromTop)
    self:addChild(title, Z.title)
end

--
-- Controls whose behaviour stays in C++
--

-- Button i (0-based, same index as the GameMode enum) -> x, y.
local function buttonPosition(i)
    local g = LAYOUT.grid
    local offsetX = math.floor((display.width - g.width) / 2 + g.iconWidth / 2)
    local posY = display.height / 2 + g.centerOffsetY

    if i < g.column1.count then
        return offsetX, (posY + g.column1.topDy) - i * g.column1.stepY
    elseif i < g.column1.count + g.row.count then
        return offsetX + g.row.firstX + (i - g.row.firstIndex) * g.row.stepX, posY
    else
        local first = g.column1.count + g.row.count
        return offsetX + g.column2.x,
               (posY + g.column2.topDy) - (i - first) * g.column2.stepY
    end
end

function GameModeLayer:layoutControls()
    for i = 0, self:getModeButtonCount() - 1 do
        local x, y = buttonPosition(i)
        local button = self:getModeButton(i)
        button:setPosition(x, y)
    end

    local label = self:getModeLabel()
    label:setAnchorPoint(0, 0)
    label:setPosition(LAYOUT.label.x, LAYOUT.label.y)

    local returnMenu = self:getReturnMenu()
    returnMenu:setAnchorPoint(1, 0.5)
    returnMenu:setPosition(display.width - LAYOUT.returnMenu.fromRight,
                           LAYOUT.returnMenu.y)
end

-- Draws the chain over a locked mode button and hands it back to C++, which
-- keeps ownership of the lock state (ModeMenuButton::unlock removes it).
local function decorateLock(button, useMask2)
    local mask = CCSprite:create(useMask2 and LOCK_MASK2 or LOCK_MASK)
    mask:setPosition(button:getPositionX(), button:getPositionY())
    button:getParent():addChild(mask, Z.lock)
    button:setLockMask(mask)
end

--
-- Global entry points (called from C++ through LuaBridge)
--

function GameModeLayer_Init(layer)
    layer:initDecor()
    layer:layoutControls()
end

function GameModeLayer_DecorateLock(button, useMask2)
    decorateLock(button, useMask2)
end
