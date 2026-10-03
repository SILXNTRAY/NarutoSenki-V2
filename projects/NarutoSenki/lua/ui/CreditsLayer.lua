--
-- CreditsLayer (staff credits screen)
--
-- Fully Lua.  Everything you see is built here: background, floating clouds,
-- menu bars, title, the two credit sheets, the return button and the credits
-- music.  Edit the LAYOUT table below to move things; no C++ rebuild is needed.
--
-- C++ (Classes/CreditsLayer.cpp) is only a thin shell:
--
--   * StartMenu::onCreditsCallBack creates the scene and the layer.
--   * CreditsLayer::init() calls CreditsLayer_Init(layer)   (below).
--   * The hardware / keyboard "back" key is delivered to CreditsLayer::
--     keyBackClicked(), which calls CreditsLayer_BackToStartMenu().
--
-- Entry points called from C++ (see Classes/Constants/UiFlowKeys.hpp):
--
--   CreditsLayer_Init(layer)
--   CreditsLayer_BackToStartMenu()
--
ns.CreditsLayer = CreditsLayer

--
-- Layout (y values are measured from the bottom unless noted)
--

local LAYOUT = {
    -- clouds drift back and forth forever
    cloudLeft = {x = 0, y = 15, drift = -15},
    cloudRight = {fromRight = 0, fromTop = 15, drift = 15},
    -- title sits in the top-left corner
    title = {x = 2, fromTop = 2},
    -- the credit sheets are placed relative to the screen centre
    credit01 = {dx = -20, dy = 80},
    credit02 = {dx = 15, dy = -60},
    returnButton = {fromRight = 38, y = 65}
}

local Z = {
    background = -5,
    clouds = 1,
    bars = 2,
    title = 3,
    returnButton = 5
}

local FLOAT_TIME = 1

--
-- Helpers
--

-- Moves a node `dx` pixels and back again, forever.
local function newFloatAction(dx)
    local move = CCMoveBy:create(FLOAT_TIME, CCPoint(dx, 0))
    return CCRepeatForever:create(transition.sequence({move, move:reverse()}))
end

--
-- Scene building
--

function CreditsLayer:build()
    local w, h = display.width, display.height

    -- background
    local bgSprite = CCSprite:create('blue_bg.png')
    bgSprite:fullScreen()
    bgSprite:setAnchorPoint(0, 0)
    bgSprite:setPosition(0, 0)
    self:addChild(bgSprite, Z.background)

    -- clouds
    local cloudLeft = display.newSprite('#cloud.png')
    cloudLeft:setPosition(LAYOUT.cloudLeft.x, LAYOUT.cloudLeft.y)
    cloudLeft:setFlipX(true)
    cloudLeft:setFlipY(true)
    cloudLeft:setAnchorPoint(0, 0)
    self:addChild(cloudLeft, Z.clouds)
    cloudLeft:runAction(newFloatAction(LAYOUT.cloudLeft.drift))

    local cloudRight = display.newSprite('#cloud.png')
    local cloudSize = cloudRight:getContentSize()
    cloudRight:setPosition(w - cloudSize.width - LAYOUT.cloudRight.fromRight,
                           h - (cloudSize.height + LAYOUT.cloudRight.fromTop))
    cloudRight:setAnchorPoint(0, 0)
    self:addChild(cloudRight, Z.clouds)
    cloudRight:runAction(newFloatAction(LAYOUT.cloudRight.drift))

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
    local title = display.newSprite('#staff_title.png')
    title:setAnchorPoint(0, 0)
    title:setPosition(LAYOUT.title.x,
                      h - title:getContentSize().height - LAYOUT.title.fromTop)
    self:addChild(title, Z.title)

    -- credit sheets
    local credit01 = display.newSprite('#credits01.png')
    credit01:setPosition(w / 2 + LAYOUT.credit01.dx, h / 2 + LAYOUT.credit01.dy)
    self:addChild(credit01)

    local credit02 = display.newSprite('#credits02.png')
    credit02:setPosition(w / 2 + LAYOUT.credit02.dx, h / 2 + LAYOUT.credit02.dy)
    self:addChild(credit02)

    -- return button
    local returnItem = ui.newImageMenuItem({
        image = 'UI/return_btn.png',
        listener = function() self:onReturn() end
    })
    local returnMenu = ui.newMenu({returnItem})
    returnMenu:setPosition(w - LAYOUT.returnButton.fromRight,
                           LAYOUT.returnButton.y)
    self:addChild(returnMenu, Z.returnButton)

    if save.isBGM() then audio.playMusic(ns.music.CREDITS_MUSIC, true) end
end

-- Tapped the on-screen return button.  Same as the back key: stop listening
-- for it first so the scene change cannot be triggered twice.
function CreditsLayer:onReturn()
    self:setKeypadEnabled(false)
    CreditsLayer_BackToStartMenu()
end

--
-- Global entry points (called from C++ through LuaBridge)
--

function CreditsLayer_Init(layer) layer:build() end

function CreditsLayer_BackToStartMenu()
    audio.stopMusic(true)
    audio.playSound(ns.menu.Perfix .. 'cancel.ogg')
    local menuScene = CCScene:create()
    local menuLayer = StartMenu:create()

    hook.registerInitHandlerOnly(menuLayer)
    menuScene:addChild(menuLayer)
    director.replaceSceneWithFade(menuScene, 1)
end
