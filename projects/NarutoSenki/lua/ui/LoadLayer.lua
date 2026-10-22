--
-- LoadLayer (loading screen before a match)
--
-- The layer instance is created by C++ (SelectLayer).  Everything that *does*
-- something stays there: loading the textures, plists and sounds of the
-- heroes and effects, working out which heroes get an avatar, picking the
-- random tip, the timers (loading music, starting the match) and building the
-- game scene.  What lives in Lua:
--
--   * The menu bars, the title and the two moving clouds.
--   * The full-screen background.
--   * The tip picture and the blinking "loading" text.
--   * The walking hero avatars at the bottom right, including their animation.
--
-- Edit the LAYOUT table below to move things; no C++ rebuild is needed.
--
-- Entry points called from C++ (see Classes/Constants/UiFlowKeys.hpp,
-- namespace LoadFlowKeys):
--
--   LoadLayer_InitDecor(layer)
--   LoadLayer_SetBackground(layer, imageName)
--   LoadLayer_AddAvatar(layer, heroName, slot, frameCount)
--   LoadLayer_ShowTip(layer, tipNumber)
--   LoadLayer_ShowLoadingText(layer)
--
ns.LoadLayer = LoadLayer

--
-- Layout (y values are measured from the bottom unless noted)
--

local LAYOUT = {
    title = {x = 2, fromTop = 2},
    clouds = {y = 15, moveTime = 1, moveDx = 15},
    loadingText = {fromRight = 120, y = 45, fadeTime = 1.0},
    -- walking heroes, one slot every `step` pixels
    avatar = {fromRight = 100, y = 30, step = 16, frameTime = 1.0 / 10.0}
}

local Z = {background = -5, clouds = 1, bars = 2, title = 3}

--
-- Decoration
--

-- A cloud that drifts `dx` pixels to one side and back, forever.
local function drift(dx)
    local move = CCMoveBy:create(LAYOUT.clouds.moveTime, CCPoint(dx, 0))
    return CCRepeatForever:create(transition.sequence({move, move:reverse()}))
end

function LoadLayer:initDecor()
    local w, h = display.width, display.height

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
    local title = display.newSprite('#loading_title.png')
    title:setAnchorPoint(0, 0)
    title:setPosition(LAYOUT.title.x,
                      h - title:getContentSize().height - LAYOUT.title.fromTop)
    self:addChild(title, Z.title)

    -- clouds: bottom left (mirrored) and top right
    local cloudLeft = display.newSprite('#cloud.png')
    cloudLeft:setPosition(0, LAYOUT.clouds.y)
    cloudLeft:setFlipX(true)
    cloudLeft:setFlipY(true)
    cloudLeft:setAnchorPoint(0, 0)
    self:addChild(cloudLeft, Z.clouds)
    cloudLeft:runAction(drift(-LAYOUT.clouds.moveDx))

    local cloudRight = display.newSprite('#cloud.png')
    local size = cloudRight:getContentSize()
    cloudRight:setPosition(w - size.width, h - (size.height + LAYOUT.clouds.y))
    cloudRight:setAnchorPoint(0, 0)
    self:addChild(cloudRight, Z.clouds)
    cloudRight:runAction(drift(LAYOUT.clouds.moveDx))
end

function LoadLayer:setBackground(imageName)
    local bg = CCSprite:create(imageName)
    bg:fullScreen()
    bg:setAnchorPoint(0, 0)
    bg:setPosition(0, 0)
    self:addChild(bg, Z.background)
end

--
-- Tip and "loading" text
--

function LoadLayer:showTip(tipNumber)
    local tip = display.newSprite(string.format('#tip%d.png', tipNumber))
    tip:setPosition(display.width / 2, display.height / 2)
    self:addChild(tip)
end

function LoadLayer:showLoadingText()
    local text = display.newSprite('#loading_font.png')
    text:setPosition(display.width - LAYOUT.loadingText.fromRight,
                     LAYOUT.loadingText.y)
    self:addChild(text)

    local fade = CCFadeOut:create(LAYOUT.loadingText.fadeTime)
    text:runAction(CCRepeatForever:create(
                       transition.sequence({fade, fade:reverse()})))
end

--
-- Walking hero avatars
--

-- Frames are named <hero>_Walk_01 ... (no extension).  The sprite shows frame
-- 1; the animation plays frames 1 .. frameCount - 1, so frameCount 1 (a hero
-- with a single frame) stands still.
function LoadLayer:addAvatar(heroName, slot, frameCount)
    local a = LAYOUT.avatar

    local avatar = display.newSprite(string.format('#%s_Walk_01', heroName))
    avatar:setFlipX(true)
    avatar:setPosition(display.width - a.fromRight + slot * a.step, a.y)
    avatar:setAnchorPoint(0, 0)
    self:addChild(avatar)

    if frameCount > 1 then
        local frames = {}
        for i = 1, frameCount - 1 do
            frames[#frames + 1] = display.newSpriteFrame(
                                      string.format('%s_Walk_%02d', heroName, i))
        end
        avatar:runAction(CCRepeatForever:create(
                             CCAnimate:create(
                                 display.newAnimation(frames, a.frameTime))))
    end
end

--
-- Global entry points (called from C++ through LuaBridge)
--

function LoadLayer_InitDecor(layer) layer:initDecor() end

function LoadLayer_SetBackground(layer, imageName) layer:setBackground(imageName) end

function LoadLayer_AddAvatar(layer, heroName, slot, frameCount)
    layer:addAvatar(heroName, slot, frameCount)
end

function LoadLayer_ShowTip(layer, tipNumber) layer:showTip(tipNumber) end

function LoadLayer_ShowLoadingText(layer) layer:showLoadingText() end
