--
-- Placement: one way to put a node on screen, shared by the screens that list
-- every element as a spec table (PauseLayer, GameOver, ...), like HudLayer does.
--
-- A spec is a plain table; every field is optional:
--
--   x, y        position, measured from the bottom-left corner of the parent
--   anchor      { ax, ay }  anchor point (sprites default to the centre)
--   scale       uniform scale
--   rotation    degrees
--   opacity     0 .. 255
--   flipX/flipY mirror a sprite
--   stretch     true = scale the sprite's width to the whole screen (menu bars)
--   visible     false = hide
--   z           draw order when added with Placement.add
--
ns.Placement = {}
local Placement = ns.Placement

function Placement.apply(node, spec)
    if not node or not spec then return end

    if spec.anchor then node:setAnchorPoint(spec.anchor[1], spec.anchor[2]) end
    if spec.flipX ~= nil then node:setFlipX(spec.flipX) end
    if spec.flipY ~= nil then node:setFlipY(spec.flipY) end
    if spec.stretch then node:fullScreen() end
    if spec.x then node:setPosition(spec.x, spec.y or 0) end
    if spec.scale then node:setScale(spec.scale) end
    if spec.rotation then node:setRotation(spec.rotation) end
    if spec.opacity then node:setOpacity(spec.opacity) end
    if spec.visible ~= nil then node:setVisible(spec.visible) end
end

-- Place `node`, then add it to `parent` at spec.z (default 0).
function Placement.add(parent, node, spec)
    Placement.apply(node, spec)
    parent:addChild(node, (spec and spec.z) or 0)
    return node
end

--
-- Groups: elements that move (and scale) together.
--
--   group = {
--       x, y     where the group's origin sits (a root group: on the screen,
--                a sub-group: relative to its parent's origin)
--       scale    scales the group's offsets AND its members (default 1)
--       members  = { nodeName = { dx, dy, anchor, scale, rotation, flipX, ... } }
--                  a member sits at origin + (dx, dy) * scale
--       groups   = { name = group, ... }   nested groups, same rules
--   }
--
-- Change x / y (or scale) of a group and everything in it follows.
-- `nodes` maps member names to the nodes: Placement.placeGroup(group, nodes).
--

local function placeGroup(group, nodes, px, py, parentScale)
    local gx = px + (group.x or 0) * parentScale
    local gy = py + (group.y or 0) * parentScale
    local s = (group.scale or 1) * parentScale

    for name, member in pairs(group.members or {}) do
        local node = nodes[name]
        if node then
            local spec = {}
            for k, v in pairs(member) do spec[k] = v end
            spec.x = gx + (member.dx or 0) * s
            spec.y = gy + (member.dy or 0) * s
            spec.scale = (member.scale or 1) * s
            Placement.apply(node, spec)
        end
    end

    for _, sub in pairs(group.groups or {}) do placeGroup(sub, nodes, gx, gy, s) end
end

function Placement.placeGroup(group, nodes) placeGroup(group, nodes, 0, 0, 1) end

return Placement
