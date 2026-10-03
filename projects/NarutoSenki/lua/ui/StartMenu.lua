--
-- StartMenu (main menu: Training / Network / Credits / Exit)
--
-- The layer instance is created by C++ (StartMenu::create), and everything that
-- *does* something stays there: touch handling, the carousel state (which
-- button sits in which slot), button actions, opening other scenes, sounds and
-- the menu text frame.  What lives in Lua:
--
--   * The static decoration: ground, clouds (floating animation), menu bars,
--     title, version label and the fading avatar.
--   * Position / scale / visibility of every control C++ creates: the four
--     carousel buttons (per slot), the menu text, the news / login buttons and
--     the notice bar.  Edit the layout tables below to move things; no C++
--     rebuild is needed.
--   * The animations: the notice marquee and the carousel button transitions.
--
-- Entry points called from C++ (see Classes/Constants/UiFlowKeys.hpp,
-- namespace StartMenuFlowKeys):
--
--   StartMenu_InitDecor(menu, versionCode)
--   StartMenu_LayoutControls(menu)
--   StartMenu_MoveButton(button, fromSlot, toSlot)
--
-- C++-owned controls are reached through getters (menu:getMenuButton(i), ...).
--
ns.StartMenu = StartMenu

local FONT_DEFAULT = 'Fonts/1.fnt'

--
-- Carousel layout
--

-- Must match enum class MenuSlot in Classes/StartMenu.h.
local SLOT = {UPPER = 0, TOP = 1, LOWER = 2, HIDDEN = 3}

local BUTTON_X = 105
local BIG_SCALE = 1
local SMALL_SCALE = 0.5

-- Where a button sits in each slot.  The hidden slot is the same spot as the
-- top one, but behind it and faded out.
local SLOT_LAYOUT = {
    [SLOT.UPPER] = {y = 150, scale = SMALL_SCALE, visible = true},
    [SLOT.TOP] = {y = 92, scale = BIG_SCALE, visible = true},
    [SLOT.LOWER] = {y = 48, scale = SMALL_SCALE, visible = true},
    [SLOT.HIDDEN] = {y = 92, scale = SMALL_SCALE, visible = false}
}

local MOVE_TIME = 0.5
local FADE_TIME = 0.3

-- z-order a button gets when it moves from one slot to another (indexed
-- [from][to]).  C++ scrolls forward (Upper -> Top -> Lower -> Hidden -> Upper)
-- or backward (the reverse).
local MOVE_Z = {
    [SLOT.UPPER] = {[SLOT.TOP] = 3, [SLOT.HIDDEN] = 1},
    [SLOT.TOP] = {[SLOT.LOWER] = 1, [SLOT.UPPER] = 2},
    [SLOT.LOWER] = {[SLOT.HIDDEN] = 1, [SLOT.TOP] = 3},
    [SLOT.HIDDEN] = {[SLOT.UPPER] = 1, [SLOT.LOWER] = 2}
}

--
-- Other controls (y values are measured from the bottom unless noted)
--

local LAYOUT = {
    menuText = {x = 10, y = 2},
    -- news / login buttons sit this far below the top of the screen
    newsButton = {x = 15, fromTop = 50},
    loginButton = {fromRight = 15, fromTop = 50},
    noticeBg = {x = 15, y = 228},
    noticeClipper = {x = 35, y = 228}
}

-- Notice marquee: the label scrolls left and re-enters from the right.
local NOTICE_SPEED = 0.6 -- pixels per frame
local NOTICE_RESTART_X = 190

--
-- Decoration
--

local function newFloatAction(dx)
    local move = CCMoveBy:create(1, CCPoint(dx, 0))
    return CCRepeatForever:create(transition.sequence({move, move:reverse()}))
end

function StartMenu:initDecor(versionCode)
    local w, h = display.width, display.height

    -- ground
    local goldLeft = display.newSprite('#gold_left.png')
    goldLeft:setAnchorPoint(0, 0)
    goldLeft:setPosition(0, 20)
    self:addChild(goldLeft, 1)

    local goldRight = display.newSprite('#gold_right.png')
    goldRight:setAnchorPoint(0, 1)
    goldRight:setPosition(w - goldRight:getContentSize().width - 20, h - 20)
    self:addChild(goldRight, 1)

    -- clouds
    local cloudLeft = display.newSprite('#cloud.png')
    cloudLeft:setPosition(0, 15)
    cloudLeft:setFlipX(true)
    cloudLeft:setFlipY(true)
    cloudLeft:setAnchorPoint(0, 0)
    self:addChild(cloudLeft, 1)
    cloudLeft:runAction(newFloatAction(-15))

    local cloudRight = display.newSprite('#cloud.png')
    cloudRight:setPosition(w - cloudRight:getContentSize().width,
                           h - (cloudRight:getContentSize().height + 15))
    cloudRight:setAnchorPoint(0, 0)
    self:addChild(cloudRight, 1)
    cloudRight:runAction(newFloatAction(15))

    -- menu bars (stretched to the full width)
    local barBottom = CCSprite:create('menu_bar2.png')
    barBottom:setAnchorPoint(0, 0)
    barBottom:setScaleX(w / barBottom:getContentSize().width)
    self:addChild(barBottom, 2)

    local barTop = CCSprite:create('menu_bar3.png')
    barTop:setAnchorPoint(0, 0)
    barTop:setPosition(0, h - barTop:getContentSize().height)
    barTop:setScaleX(w / barTop:getContentSize().width)
    self:addChild(barTop, 2)

    -- title
    local title = display.newSprite('#startmenu_title.png')
    title:setAnchorPoint(0, 0)
    title:setPosition(2, h - title:getContentSize().height - 2)
    self:addChild(title, 3)

    -- version
    local versionLabel = CCLabelBMFont:create(versionCode, FONT_DEFAULT)
    versionLabel:setScale(0.3)
    versionLabel:setPosition(w - 25, 10)
    self:addChild(versionLabel, 5)

    -- avatar: plays each frame set, then fades in, holds and fades out
    local avator = display.newSprite('#avator1.png')
    avator:setAnchorPoint(0, 0)
    avator:setOpacity(0)
    avator:setPosition(w - avator:getContentSize().width, 19)
    self:addChild(avator, 1)

    local frames, actions = {}, {}
    for i = 1, 4 do
        -- every round plays one more frame than the previous one
        frames[#frames + 1] = display.newSpriteFrame(
                                  string.format('avator%d.png', i))
        actions[#actions + 1] = CCAnimate:create(
                                    display.newAnimation(frames, 0.1))
        actions[#actions + 1] = CCFadeIn:create(0.8)
        actions[#actions + 1] = CCDelayTime:create(1.0)
        actions[#actions + 1] = CCFadeOut:create(0.5)
    end
    avator:runAction(CCRepeatForever:create(transition.sequence(actions)))
end

--
-- Controls whose behaviour stays in C++
--

-- Instant placement of a carousel button in a slot (used for the initial layout).
local function placeButton(button, slot)
    local spec = SLOT_LAYOUT[slot]
    button:setPosition(BUTTON_X, spec.y)
    button:setScale(spec.scale)
    button:setVisible(spec.visible)
end

function StartMenu:layoutControls()
    local w, h = display.width, display.height

    for i = 0, self:getMenuButtonCount() - 1 do
        local button = self:getMenuButton(i)
        placeButton(button, button:getSlotIndex())
    end

    local text = self:getMenuText()
    text:setAnchorPoint(0, 0)
    text:setPosition(LAYOUT.menuText.x, LAYOUT.menuText.y)

    -- the items live in a CCMenu created by C++; the menu is what moves
    local news = self:getNewsButton()
    news:setAnchorPoint(0, 0.5)
    news:getParent():setPosition(LAYOUT.newsButton.x, h - LAYOUT.newsButton.fromTop)

    local login = self:getLoginButton()
    login:setAnchorPoint(1, 0.5)
    login:getParent():setPosition(w - LAYOUT.loginButton.fromRight,
                                  h - LAYOUT.loginButton.fromTop)

    -- notice bar
    local noticeBg = self:getNoticeBg()
    noticeBg:setAnchorPoint(0, 0)
    noticeBg:setPosition(LAYOUT.noticeBg.x, LAYOUT.noticeBg.y)
    self:getNoticeClipper():setPosition(LAYOUT.noticeClipper.x,
                                        LAYOUT.noticeClipper.y)
    self:getNoticeLabel():setAnchorPoint(0, 0)
    self:startNoticeMarquee()
end

function StartMenu:startNoticeMarquee()
    local label = self:getNoticeLabel()
    if not label then return end

    self:scheduleUpdateWithPriorityLua(function()
        local x = label:getPositionX()
        if x >= -label:getContentSize().width then
            label:setPositionX(x - NOTICE_SPEED)
        else
            label:setPositionX(NOTICE_RESTART_X)
        end
    end, 0)
end

-- Animates one carousel button between slots.  C++ has already updated the
-- logical state (button slot, which button is active); this is visuals only.
local function moveButton(button, from, to)
    local spec = SLOT_LAYOUT[to]
    button:getParent():reorderChild(button, MOVE_Z[from][to])

    local move = CCMoveTo:create(MOVE_TIME, CCPoint(BUTTON_X, spec.y))
    local action = move

    if to == SLOT.TOP or from == SLOT.TOP then
        -- the active button grows, the one it replaces shrinks
        action = CCSpawn:createWithTwoActions(
                     move, CCScaleTo:create(MOVE_TIME, spec.scale))
    elseif to == SLOT.HIDDEN then
        action = CCSpawn:createWithTwoActions(move, CCFadeOut:create(FADE_TIME))
    elseif from == SLOT.HIDDEN then
        button:setVisible(true)
        action = CCSpawn:createWithTwoActions(move, CCFadeIn:create(FADE_TIME))
    end

    if to == SLOT.TOP then
        -- announce the newly active entry once it has arrived
        action = transition.sequence({
            action, CCCallFunc:create(function() button:playSound() end)
        })
    end

    button:runAction(action)
end

--
-- Global entry points (called from C++ through LuaBridge)
--

function StartMenu_InitDecor(menu, versionCode) menu:initDecor(versionCode) end

function StartMenu_LayoutControls(menu) menu:layoutControls() end

function StartMenu_MoveButton(button, from, to) moveButton(button, from, to) end

--
-- Scene flow
--

function StartMenu:init()
    log('Initial StartMenu...')

    audio.setSoundsVolume(0.5)

    tools.addSprites('Menu.plist')
    tools.addSprites('Result.plist')
    tools.addSprites('NamePlate.plist')
end

function enterSelectLayer(gameMode, enableCustomSelect)
    tools.addSprites('Select.plist')
    tools.addSprites('UI.plist')
    tools.addSprites('Report.plist')
    tools.addSprites('Ougis.plist')
    tools.addSprites('Ougis2.plist')
    tools.addSprites('Map.plist')
    tools.addSprites('Gears.plist')

    _G.mode = gameMode
    _G.enableCustomSelect = enableCustomSelect
    local selectScene = CCScene:create()
    local selectLayer = SelectLayer:create()

    hook.registerInitHandlerOnly(selectLayer)

    selectScene:addChild(selectLayer)
    director.replaceSceneWithFade(selectScene, 1.25)
end

function onGameOver()
    local menuScene = CCScene:create()
    local menuLayer = StartMenu:create()

    hook.registerInitHandlerOnly(menuLayer)
    menuScene:addChild(menuLayer)
    director.replaceSceneWithFade(menuScene, 1.25)
end
