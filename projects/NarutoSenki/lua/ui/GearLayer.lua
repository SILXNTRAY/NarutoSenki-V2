--
-- GearLayer (in-game gear shop)
--
-- The layer instance is still created by C++ (GameLayer::onGear), because the
-- game state lives there.  Everything that *does* something stays in C++:
-- buying and selling gears, which gear is selected, touch handling, the scroll
-- list and its clipping, the sounds and leaving the shop.  What lives in Lua:
--
--   * The shop panel and the dimmer drawn over the frozen game screenshot.
--   * Position / anchor of every control C++ creates: coin label, scroll bar,
--     detail card, big icon, buy / close menus, the clipped list and the row
--     of gears the player already owns.
--   * The look of a gear button: gear icon, "sold out" overlay and where the
--     button sits (shop grid or owned row).
--   * Which sprite frame the detail card / big icon shows for a gear.
--   * The scroll limits handed to the C++ scroll list.
--
-- Edit the layout tables below to move things; no C++ rebuild is needed.
--
-- Entry points called from C++ (see Classes/Constants/UiFlowKeys.hpp,
-- namespace GearFlowKeys):
--
--   GearLayer_LayoutControls(layer)
--   GearLayer_LayoutCurrentGears(layer)
--   GearLayer_ShowGearDetail(layer, gearType, updateBigIcon)
--   GearLayer_DecorateButton(button, gearType, buttonType, isBuyed, slot)
--
-- C++-owned controls are reached through getters (layer:getCoinLabel(), ...).
--
ns.GearLayer = GearLayer

-- Must match enum class GearButtonType in Classes/GearLayer.h.
local BUTTON = {BUY = 0, SELL = 1}

--
-- Layout.  x values are relative to the centre of the shop panel, y values are
-- measured from the bottom of the screen unless noted.
--

local PANEL = {
    -- centre of the panel, relative to the screen centre
    offsetY = -12
}

local LAYOUT = {
    dimmerOpacity = 150,
    coinLabel = {dx = 2, y = 58},
    screwBar = {dx = 25, y = 126},
    -- the detail card and the big icon share the same column (measured from
    -- the right edge of the panel)
    detail = {fromRight = 54, y = 210},
    bigIcon = {fromRight = 54, y = 90},
    buyMenu = {dx = 78, y = 65},
    -- the scrolling list is clipped to the panel's left part
    clipper = {fromLeft = 4, y = 85},
    closeMenu = {fromRight = 12, fromTop = 20},
    -- row of gears the player already owns
    currentGears = {fromLeft = 8, y = 64}
}

-- Scroll limits for the C++ list (see ScrewLayer::setScrollLimits): the bar
-- stays between BAR_MIN and BAR_MAX, the list never scrolls below LIST_MIN.
-- LIST_MIN is also where the list starts.  The bar *starts* at screwBar.y,
-- above BAR_MAX, and snaps into range on the first touch.
local SCROLL = {BAR_MIN = 90, BAR_MAX = 122, LIST_MIN = 76}

-- Shop grid: gears are laid out three per row, filling downwards.
local GRID = {columns = 3, x = 6, columnWidth = 46, rowHeight = 60}

-- Row of owned gears.
local OWNED = {x = 13, step = 34, iconScale = 0.75}

-- Position of a gear icon on its price plate.
local BUY_ICON = {x = 20, y = 30}

local Z = {dimmer = 1, panel = 10}

-- Panel metrics, captured when the controls are laid out (only one shop is
-- open at a time).  Used again when the owned-gear row is placed.
local panel

--
-- Controls whose behaviour stays in C++
--

function GearLayer:layoutControls()
    local w, h = display.width, display.height

    -- frozen screenshot of the game, darkened
    self:getSnapshotBg():setAnchorPoint(0, 0)
    self:addChild(CCLayerColor:create(ccc4(0, 0, 0, LAYOUT.dimmerOpacity), w, h),
                  Z.dimmer)

    -- shop panel
    local bg = display.newSprite('#gears_bg.png')
    local size = bg:getContentSize()
    local cx, cy = w / 2, h / 2 + PANEL.offsetY
    bg:setPosition(cx, cy)
    self:addChild(bg, Z.panel)

    local left, right = cx - size.width / 2, cx + size.width / 2
    panel = {cx = cx, cy = cy, left = left, right = right}

    local coin = self:getCoinLabel()
    coin:setAnchorPoint(0, 0)
    coin:setPosition(cx + LAYOUT.coinLabel.dx, LAYOUT.coinLabel.y)

    local screwBar = self:getScrewBar()
    screwBar:setAnchorPoint(0.5, 0)
    screwBar:setPosition(cx + LAYOUT.screwBar.dx, LAYOUT.screwBar.y)

    local detail = self:getGearDetail()
    detail:setAnchorPoint(0.5, 1)
    detail:setPosition(right - LAYOUT.detail.fromRight, LAYOUT.detail.y)

    -- only created on desktop builds
    local bigIcon = self:getGearBigIcon()
    if bigIcon then
        bigIcon:setAnchorPoint(0.5, 0)
        bigIcon:setPosition(right - LAYOUT.bigIcon.fromRight, LAYOUT.bigIcon.y)
    end

    -- the menus are what moves, their items stay at the menu origin
    self:getBuyMenu():setPosition(cx + LAYOUT.buyMenu.dx, LAYOUT.buyMenu.y)
    -- (measured from the screen centre, not the panel centre, like the original)
    self:getCloseMenu():setPosition(right - LAYOUT.closeMenu.fromRight,
                                    h / 2 + size.height / 2 -
                                        LAYOUT.closeMenu.fromTop)

    -- scroll list: clipped to the panel, positioned inside the clipper
    self:getClipper():setPosition(left + LAYOUT.clipper.fromLeft,
                                  LAYOUT.clipper.y)

    local list = self:getScrewLayer()
    list:setScrollLimits(SCROLL.BAR_MIN, SCROLL.BAR_MAX, SCROLL.LIST_MIN)
    list:setAnchorPoint(0, 0)
    list:setPositionY(SCROLL.LIST_MIN)
end

-- The row of gears the player already owns sits at the bottom left of the panel.
function GearLayer:layoutCurrentGears()
    if not panel then return end

    local row = self:getCurrentGearLayer()
    if not row then return end

    row:setAnchorPoint(0, 0)
    row:setPosition(panel.left + LAYOUT.currentGears.fromLeft,
                    LAYOUT.currentGears.y)
end

-- Switches the detail card (and the big icon) to a gear.
function GearLayer:updateDetail(gearType, updateBigIcon)
    self:getGearDetail():setDisplayFrame(
        display.newSpriteFrame(string.format('gearDetail_%02d.png', gearType)))

    local bigIcon = self:getGearBigIcon()
    if updateBigIcon and bigIcon then
        bigIcon:setDisplayFrame(
            display.newSpriteFrame(string.format('gear_%02d.png', gearType)))
    end
end

--
-- Gear buttons
--

-- Builds what is drawn on a gear button and places it.  C++ has already set
-- the button's logical state (gear, type, bought); this is visuals only.
local function decorateButton(button, gearType, buttonType, isBuyed, slot)
    local icon = display.newSprite(string.format('#gear_%02d.png', gearType))

    if buttonType == BUTTON.BUY then
        -- shop grid: fills rows of GRID.columns, going down
        local row = math.floor(slot / GRID.columns)
        local column = slot - GRID.columns * row
        button:setAnchorPoint(0, 0)
        button:setPosition(GRID.x + column * GRID.columnWidth,
                           -row * GRID.rowHeight)

        icon:setPosition(BUY_ICON.x, BUY_ICON.y)
    else
        -- owned row: small icons side by side
        button:setPositionX(OWNED.x + slot * OWNED.step)

        icon:setScale(OWNED.iconScale)
    end
    button:addChild(icon)

    if isBuyed then
        -- "sold out" overlay, centred on the button
        local size = button:getContentSize()
        local soldOut = display.newSprite('#gear_so.png')
        soldOut:setPosition(size.width / 2, size.height / 2)
        button:addChild(soldOut)
    end
end

--
-- Global entry points (called from C++ through LuaBridge)
--

function GearLayer_LayoutControls(layer) layer:layoutControls() end

function GearLayer_LayoutCurrentGears(layer) layer:layoutCurrentGears() end

function GearLayer_ShowGearDetail(layer, gearType, updateBigIcon)
    layer:updateDetail(gearType, updateBigIcon)
end

function GearLayer_DecorateButton(button, gearType, buttonType, isBuyed, slot)
    decorateButton(button, gearType, buttonType, isBuyed, slot)
end
