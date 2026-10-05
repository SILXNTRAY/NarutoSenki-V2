--[[--
	Custom content registry.

	This file is DATA ONLY. The C++ side (Classes/Core/CustomRegistry.hpp) reads
	the ns.* tables below the first time a custom unit is needed, so everything
	here is read once, after config.lua has created `ns`.

	It also acts as an "include" list: to add content, add a line to the
	matching table. Nothing else in Lua needs to change.

	An entry is either a string (the gimmick) or a table:

		MyChar = 'Suigetsu'
		MyChar = { gimmick = 'Suigetsu' }

	Optional table fields for ns.CustomCharacters:
		team   = 'Any' | 'Konoha' | 'Akatsuki'   (default 'Any', currently informational)
		random = true | false                    (default true, see "random" below)

	GIMMICK
		The name of an existing character whose C++ class (skills, AI, gimmick)
		the unit copies. Use 'Generic' for the plain template with no buff
		gimmick: Skill1-3 + Ougis1-2, with a basic AI.
		Forms of multi-form heroes can be copied too, e.g. 'SageNaruto' or
		'ImmortalSasuke', to get that form's AI and behaviour.

	RANDOM
		Custom characters join the random enemy/ally roster by default.
		Names that are NOT a base form (position 2+ in an ns.Transform chain)
		are never put in the random roster.

	Every unit needs its own assets, same as stock content:
		Unit/Ninja/<Name>/<Name>.xml            + <Name>.plist  (+ <Name>_Skill.plist, optional)
		Unit/Kuchiyose/<Name>/<Name>.xml        (kuchiyose)
		Unit/Kugutsu/<Name>/<Name>.xml          (kugutsu / puppets)
		Unit/Mon/<Name>.xml                     (mons)
		Unit/Projectile/<Name>.xml              (bullets)
]]

-- Heroes (and every form of a transforming hero)
ns.CustomCharacters = {
	-- Custom1 = 'Suigetsu',
	-- Custom2 = { gimmick = 'Generic', team = 'Konoha' },
}

-- Clones. Processed the same as characters, kept separate for readability.
ns.CustomClones = {
	-- Custom1Clone = 'Generic',
}

-- Kuchiyose and kugutsu. The folder (Unit/Kuchiyose or Unit/Kugutsu) is
-- detected automatically from where the unit's xml/plist actually is.
ns.CustomKuchiyose = {
	-- CustomKuchiyose = 'Generic',
}

-- Mons. Two ways to write one:
--
--   1. COPY a stock mon. Gets that mon's spawn position, immediate-attack-or-AI
--      choice, movement, AI, hit effects and special cases (SansyoBlue's fixed
--      spot, FutonSRK's homing, SmallSlug's 3-summon cap, ...) exactly:
--          MyBlue = 'SansyoBlue',
--      The copy is exact, so the fields below are ignored when a gimmick is set.
--      Your mon still loads its own Unit/Mon/<Name>.xml.
--
--   2. Describe a GENERIC mon. Every field is optional:
--          mode        'attack' (default) plays its attack as soon as it is summoned
--                      'ai'     idles/walks, finds a hero (else a flog), attacks it once in range
--                      'none'   just appears; the owner or its xml drives it
--          offset      { x, y }  from the owner, x is mirrored when facing left. Default { 32, 0 }
--          anchor      { x, y }  only set when given
--          armorBroken true/false  (default false)
--          effect      'smk'     a skill effect played on spawn
--          track       true/false  the owner keeps it in its monster array, so it is
--                      cleaned up with the owner and can be commanded. Default true
--          move        a movement, started right after the attack:
--                        { 'direct', length, seconds [, true] }  straight line in the facing
--                              direction, then removed. true = goes there and comes back
--                        { 'ease', length, rate }  accelerating line, always 1s long;
--                              rate 1.0 is linear, higher starts slower and ends faster
--                        { 'chase', seconds }  steps toward the owner's target (or straight
--                              ahead if none) for that long, then removed
ns.CustomMons = {
	-- MyBlue    = 'SansyoBlue',
	-- MySRK     = 'FutonSRK',
	-- Pet       = { mode = 'ai', effect = 'smk' },
	-- Fireball  = { mode = 'attack', offset = { 48, 0 }, move = { 'direct', 156, 2.0 } },
	-- Boomerang = { move = { 'direct', 128, 0.8, true } },
}

-- Bullets (projectiles). Every field is optional.
--   offset = { x, y }   offset from the owner. Default { 32, <half the owner's height> }
--   scale  = number     Default 1
--   move   = { distance, seconds }   Default { 192, 2.0 }
ns.CustomBullets = {
	-- CustomBullet = { offset = { 32, 40 }, scale = 0.8, move = { 192, 2.0 } },
}

-- summon = owner. Works for clones and kuchiyose/kugutsu alike.
-- A linked summon has its plist loaded and unloaded together with its owner.
-- Spawn it from the owner's xml with  <e type="setSummon">Name</e>  or
-- <e type="setSummon">Name:15</e>  (the number is a lifetime in seconds,
-- 0 or missing = no automatic despawn).
-- A linked CLONE is also what the owner's plain  <e type="setClone">  spawns.
ns.LinkSummon = {
	-- Custom1Clone    = 'Custom1',
	-- CustomKuchiyose = 'Custom1',
}

-- Transform chains, first entry is the base form. Triggered by the existing
-- <e type="setTransform">40</e> event in the xml of the form that transforms.
-- A chain stops at its last form. Forms' plists load with the base form.
ns.Transform = {
	-- { 'Custom1', 'Custom2', 'Custom3' },
}

-- Extra plists, for files the game can't discover by itself.
--   Skills    = { 'Custom1_skill1' }  basenames, looked up next to the unit's own plist
--   Kuchiyose = { 'CustomKuchiyose' } loads a kuchiyose/kugutsu plist WITHOUT linking it
--   Files     = { 'Unit/Ninja/X/X_extra.plist' }  full paths from Resources
-- Strings are used exactly as written, so match the real filename's case
-- (Android is case-sensitive). The default <Name>_Skill.plist is always tried.
ns.ExtraPlists = {
	-- Custom1 = { Skills = { 'Custom1_skill1' } },
}
