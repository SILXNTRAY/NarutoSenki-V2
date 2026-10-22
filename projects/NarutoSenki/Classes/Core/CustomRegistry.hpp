#pragma once

/**
 * Custom content registry.
 *
 * Reads the ns.* tables declared in lua/class/custom.lua (ns.CustomCharacters,
 * ns.CustomClones, ns.CustomKuchiyose, ns.CustomMons, ns.CustomBullets,
 * ns.LinkSummon, ns.Transform, ns.ExtraPlists) once, the first time anything
 * asks for them, and answers queries for the rest of the game:
 *
 *   - which stock class (gimmick) a custom unit copies  -> Provider, getGimmickName()
 *   - which unit a transform chain moves to next         -> CharacterBase::setTransform()
 *   - which summons belong to which owner                -> CharacterBase::setSummon()/setClone()
 *   - which extra plists an owner needs                  -> LoadLayer
 *   - which custom characters may appear in random rosters
 *   - spawn profiles for custom mons and bullets
 *
 * Define CUSTOM_REGISTRY_STANDALONE to build this header without the engine
 * (only <lua.h> is needed); used to test the Lua parsing on its own.
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef CUSTOM_REGISTRY_STANDALONE
extern "C"
{
#include <lauxlib.h>
#include <lua.h>
}
#define CUSTOM_LOG(...)          \
	do                           \
	{                            \
		printf(__VA_ARGS__);     \
		printf("\n");            \
	} while (0)
#else
#include "Utils/Cocos2dxHelper.hpp"
#define CUSTOM_LOG(...) CCLOG(__VA_ARGS__)
#endif

namespace Custom
{

enum class Kind : uint8_t
{
	Hero,
	Clone,
	Kuchiyose,
};

struct Unit
{
	std::string name;
	std::string gimmick; // Stock character to copy, or "Generic"
	std::string team = "Any";
	int randomSet = -1; // -1 = not set (use default), 0 = false, 1 = true
	Kind kind = Kind::Hero;
};

enum class MonMode : uint8_t
{
	Attack, // plays its attack animation as soon as it is summoned
	Ai,		// idles/walks, looks for a target, attacks it once in range
	None,	// just appears, the owner or its xml drives it
};

enum class MonMoveType : uint8_t
{
	None,
	Direct, // straight line in the facing direction, removed at the end (Monster::setDirectMove)
	Ease,	// accelerating line, 1s long, `time` is the ease rate (Monster::setEaseIn)
	Chase,	// steps toward the owner's target (or forward) for `time` seconds (Monster::setDirectMoveBy)
};

struct MonMove
{
	MonMoveType type = MonMoveType::None;
	int length = 0;
	float time = 0.0f;
	bool reverse = false; // Direct only: goes there and comes back
};

struct MonProfile
{
	std::string name;
	std::string gimmick; // Stock mon to copy exactly (spawn, AI and hit quirks). Empty = generic mon.

	// The fields below only apply to generic mons (no gimmick)
	MonMode mode = MonMode::Attack;
	float offX = 32.0f;
	float offY = 0.0f;
	bool hasAnchor = false;
	float anchorX = 0.5f;
	float anchorY = 0.0f;
	bool armorBroken = false;
	std::string effect; // setSkillEffect, e.g. "smk"
	bool track = true;	// the owner keeps it in its monster array (cleanup on death, commands)
	MonMove move;
};

struct BulletProfile
{
	std::string name;
	float offX = 32.0f;
	float offY = 0.0f;
	bool hasOffY = false; // false: use half of the owner's height
	float scale = 1.0f;
	float moveDist = 192.0f;
	float moveTime = 2.0f;
};

struct Extra
{
	std::vector<std::string> skills;
	std::vector<std::string> kuchiyose;
	std::vector<std::string> files;
};

class Registry
{
public:
	static constexpr const char *kGeneric = "Generic";

	/** Registry used by the game. Loads from the running Lua engine on first use. */
	static Registry &get()
	{
		static Registry instance;
#ifndef CUSTOM_REGISTRY_STANDALONE
		if (!instance.loaded_)
		{
			auto L = LuaEngine::defaultEngine()->getLuaStack()->getLuaState();
			instance.loadFrom(L);
		}
#endif
		return instance;
	}

	/** Read every table from `ns`. Returns false (and stays empty) if `ns` isn't there yet. */
	bool loadFrom(lua_State *L)
	{
		clear();

		int top = lua_gettop(L);
		lua_getglobal(L, "ns");
		if (!lua_istable(L, -1))
		{
			lua_settop(L, top);
			return false;
		}
		int ns = lua_gettop(L);

		readUnits(L, ns, "CustomCharacters", Kind::Hero);
		readUnits(L, ns, "CustomClones", Kind::Clone);
		readUnits(L, ns, "CustomKuchiyose", Kind::Kuchiyose);
		readMons(L, ns);
		readBullets(L, ns);
		readLinks(L, ns);
		readBuffs(L, ns);
		readTransforms(L, ns);
		readExtras(L, ns);
		finalize();

		lua_settop(L, top);
		loaded_ = true;
		return true;
	}

	bool isLoaded() const { return loaded_; }

	/** ---- Units ---- */

	bool hasUnits() const { return !unitIndex_.empty(); }

	const Unit *findUnit(const std::string &name) const
	{
		auto it = unitIndex_.find(name);
		return it == unitIndex_.end() ? nullptr : &units_[it->second];
	}

	bool isCustomUnit(const std::string &name) const { return findUnit(name) != nullptr; }

	/** Clone or kuchiyose entry, nullptr for heroes and unknown names. */
	const Unit *findSummon(const std::string &name) const
	{
		auto u = findUnit(name);
		return (u && u->kind != Kind::Hero) ? u : nullptr;
	}

	/** Stock character or mon a custom unit/mon copies; for anything else the name itself. */
	std::string gimmickOf(const std::string &name) const
	{
		if (auto u = findUnit(name))
			return u->gimmick;
		auto m = mons_.find(name);
		if (m != mons_.end() && !m->second.gimmick.empty())
			return m->second.gimmick;
		return name;
	}

	/** Cheap check so gimmickOf() can be skipped entirely when nothing custom is registered. */
	bool hasAliases() const { return !unitIndex_.empty() || !mons_.empty(); }

	/** ---- Transform ---- */

	/**
	 * Optional AI hint of a transform chain (ns.Transform { ..., via = 'ougis2', below = 0.5 }):
	 * which skill slot (1-3 = skill1-3, 4 = ougis1, 5 = ougis2) holds the setTransform event,
	 * and the HP fraction at or below which the AI should use it. Set on every form of the chain.
	 */
	struct TransformAi
	{
		int slot = 0;
		float below = 1.0f;
	};

	const TransformAi *transformAiOf(const std::string &form) const
	{
		auto it = transformAi_.find(form);
		return it == transformAi_.end() ? nullptr : &it->second;
	}

	const std::string *transformNext(const std::string &name) const
	{
		auto it = next_.find(name);
		return it == next_.end() ? nullptr : &it->second;
	}

	/** True unless the name is position 2+ in a transform chain. */
	bool isBaseForm(const std::string &name) const { return nonBase_.find(name) == nonBase_.end(); }

	/** The first form of the chain a name belongs to (the name itself if it isn't a later form). */
	std::string baseOf(const std::string &name) const
	{
		auto it = base_.find(name);
		return it == base_.end() ? name : it->second;
	}

	/** Every form after the base one, in chain order. */
	const std::vector<std::string> &formsOf(const std::string &base) const
	{
		auto it = forms_.find(base);
		return it == forms_.end() ? kNoNames : it->second;
	}

	/** ---- Summons ---- */

	const std::string *ownerOf(const std::string &summon) const
	{
		auto it = owner_.find(summon);
		return it == owner_.end() ? nullptr : &it->second;
	}

	/** Linked summons of an owner, sorted by name. */
	const std::vector<std::string> &summonsOf(const std::string &owner) const
	{
		auto it = summons_.find(owner);
		return it == summons_.end() ? kNoNames : it->second;
	}

	bool isLinked(const std::string &summon, const std::string &owner) const
	{
		auto o = ownerOf(summon);
		return o && *o == owner;
	}

	/**
	 * Key of a keyed link ({ 'Owner', key } in ns.LinkSummon): the number the owner's
	 * <e type="setClone">key</e> must carry to spawn this summon. nullptr = unkeyed.
	 */
	const int *keyOf(const std::string &summon) const
	{
		auto it = key_.find(summon);
		return it == key_.end() ? nullptr : &it->second;
	}

	/** The summon of `owner` linked with exactly this key (clone or kuchiyose), or nullptr. */
	const Unit *linkedByKey(const std::string &owner, int key) const
	{
		for (auto &s : summonsOf(owner))
		{
			auto k = keyOf(s);
			if (!k || *k != key)
				continue;
			auto u = findUnit(s);
			if (u && u->kind != Kind::Hero)
				return u;
		}
		return nullptr;
	}

	/**
	 * First UNKEYED clone-kind summon of an owner: what a setClone spawns when no keyed
	 * link matches its number. Unkeyed kuchiyose are only spawned by setSummon.
	 */
	const Unit *firstLinkedClone(const std::string &owner) const
	{
		for (auto &s : summonsOf(owner))
		{
			if (keyOf(s))
				continue;
			auto u = findUnit(s);
			if (u && u->kind == Kind::Clone)
				return u;
		}
		return nullptr;
	}

	/**
	 * ns.CustomBuffs: the character whose cBuff mechanics `unit` uses for buff `id`
	 * (the number of the xml's <e type="setBuff">id</e>), or nullptr. Later forms of a
	 * transform chain use the base form's entries unless they declare their own.
	 */
	const std::string *buffDonor(const std::string &unit, int id) const
	{
		for (const std::string &name : {unit, baseOf(unit)})
		{
			auto it = buffs_.find(name);
			if (it == buffs_.end())
				continue;
			auto d = it->second.find(id);
			if (d != it->second.end())
				return &d->second;
		}
		return nullptr;
	}

	/** ---- Roster ---- */

	/** Custom characters allowed in random rosters. Strings stay valid (c_str) until reload. */
	const std::vector<std::string> &randomHeroNames() const { return randomNames_; }

	/** Custom characters that need a row in the CharRecord table: base forms only (wins are kept on the base). */
	std::vector<std::string> recordNames() const
	{
		std::vector<std::string> out;
		for (auto &u : units_)
			if (u.kind == Kind::Hero && isBaseForm(u.name))
				out.push_back(u.name);
		return out;
	}

	/** ---- Mons and bullets ---- */

	const MonProfile *findMon(const std::string &name) const
	{
		auto it = mons_.find(name);
		return it == mons_.end() ? nullptr : &it->second;
	}

	const BulletProfile *findBullet(const std::string &name) const
	{
		auto it = bullets_.find(name);
		return it == bullets_.end() ? nullptr : &it->second;
	}

	/** ---- Extra plists ---- */

	const Extra *extraOf(const std::string &name) const
	{
		auto it = extras_.find(name);
		return it == extras_.end() ? nullptr : &it->second;
	}

#ifndef CUSTOM_REGISTRY_STANDALONE
	/**
	 * Where a unit's files live: "Ninja", "Kuchiyose" or "Kugutsu" ("" if not found).
	 * Same probing order as Hero::setID uses for the xml. Summons look in
	 * Kuchiyose/Kugutsu first, everything else looks in Ninja first.
	 */
	static std::string folderOf(const std::string &name, bool summonFirst)
	{
		static const char *kSummonFirst[] = {"Kuchiyose", "Kugutsu", "Ninja"};
		static const char *kNinjaFirst[] = {"Ninja", "Kuchiyose", "Kugutsu"};
		auto fu = FileUtils::sharedFileUtils();
		for (auto folder : (summonFirst ? kSummonFirst : kNinjaFirst))
		{
			std::string base = std::string("Unit/") + folder + "/" + name + "/" + name;
			if (fu->isFileExist((base + ".plist").c_str()) || fu->isFileExist((base + ".xml").c_str()))
				return folder;
		}
		return "";
	}

	/**
	 * Every extra plist `owner` needs besides its own <Name>.plist / <Name>_Skill.plist
	 * (the game already loads those): the extras from ns.ExtraPlists, the plists of
	 * every form in its transform chain, and the plists of every linked summon.
	 * Only files that exist are returned.
	 */
	std::vector<std::string> extraPlistsOf(const std::string &owner) const
	{
		std::vector<std::string> out;
		std::unordered_set<std::string> seenFiles;
		std::unordered_set<std::string> seenUnits;

		auto add = [&](const std::string &path, bool warnIfMissing)
		{
			if (!FileUtils::sharedFileUtils()->isFileExist(path.c_str()))
			{
				if (warnIfMissing)
					CUSTOM_LOG("[Custom] plist not found: %s", path.c_str());
				return;
			}
			if (seenFiles.insert(path).second)
				out.push_back(path);
		};

		auto withPlist = [](const std::string &s)
		{
			return (s.size() > 6 && s.compare(s.size() - 6, 6, ".plist") == 0) ? s : s + ".plist";
		};

		// A name with a '/' is a full path from Resources, otherwise it sits next to the unit's own plist
		auto plistPath = [&](const std::string &folder, const std::string &unit, const std::string &s)
		{
			if (s.find('/') != std::string::npos)
				return withPlist(s);
			return withPlist("Unit/" + folder + "/" + unit + "/" + s);
		};

		auto visit = [&](const std::string &unit, bool isOwner, bool isSummon, auto &&self) -> void
		{
			if (!seenUnits.insert(unit).second)
				return;

			auto folder = folderOf(unit, isSummon);
			if (folder.empty())
			{
				if (!isOwner)
					CUSTOM_LOG("[Custom] no files found for '%s' (looked in Ninja, Kuchiyose, Kugutsu)", unit.c_str());
				folder = "Ninja";
			}

			if (!isOwner)
			{
				// The owner's own plists are loaded by the stock code
				add("Unit/" + folder + "/" + unit + "/" + unit + ".plist", true);
				add("Unit/" + folder + "/" + unit + "/" + unit + "_Skill.plist", false);
			}

			if (auto ex = extraOf(unit))
			{
				for (auto &s : ex->skills)
					add(plistPath(folder, unit, s), true);
				for (auto &f : ex->files)
					add(withPlist(f), true);
				for (auto &k : ex->kuchiyose)
				{
					auto kf = folderOf(k, true);
					if (kf.empty())
					{
						CUSTOM_LOG("[Custom] no files found for kuchiyose '%s' listed in ExtraPlists.%s", k.c_str(), unit.c_str());
						continue;
					}
					add("Unit/" + kf + "/" + k + "/" + k + ".plist", true);
					add("Unit/" + kf + "/" + k + "/" + k + "_Skill.plist", false);
				}
			}

			for (auto &s : summonsOf(unit))
				self(s, false, true, self);
		};

		visit(owner, true, false, visit);
		for (auto &form : formsOf(owner))
			visit(form, false, false, visit);

		return out;
	}
#endif

private:
	inline static const std::vector<std::string> kNoNames{};

	bool loaded_ = false;

	std::vector<Unit> units_;
	std::unordered_map<std::string, size_t> unitIndex_;
	std::unordered_map<std::string, MonProfile> mons_;
	std::unordered_map<std::string, BulletProfile> bullets_;
	std::unordered_map<std::string, std::string> owner_;				  // summon -> owner
	std::unordered_map<std::string, std::unordered_map<int, std::string>> buffs_; // unit -> buff id -> donor
	std::unordered_map<std::string, int> key_;							  // summon -> setClone key (keyed links only)
	std::unordered_map<std::string, std::vector<std::string>> summons_;	  // owner -> summons
	std::unordered_map<std::string, std::string> next_;					  // form -> next form
	std::unordered_map<std::string, TransformAi> transformAi_;			  // form -> AI hint (optional)
	std::unordered_map<std::string, std::vector<std::string>> forms_;	  // base -> later forms
	std::unordered_set<std::string> nonBase_;
	std::unordered_map<std::string, std::string> base_;					  // later form -> base form
	std::unordered_map<std::string, Extra> extras_;
	std::vector<std::string> randomNames_;

	void clear()
	{
		loaded_ = false;
		units_.clear();
		unitIndex_.clear();
		mons_.clear();
		bullets_.clear();
		owner_.clear();
		key_.clear();
		buffs_.clear();
		summons_.clear();
		next_.clear();
		transformAi_.clear();
		forms_.clear();
		nonBase_.clear();
		base_.clear();
		extras_.clear();
		randomNames_.clear();
	}

	/** ---- Lua helpers (all indices are absolute) ---- */

	template <class Fn>
	static void eachStringKey(lua_State *L, int tbl, Fn fn)
	{
		lua_pushnil(L);
		while (lua_next(L, tbl) != 0)
		{
			// key at -2, value at -1
			if (lua_type(L, -2) == LUA_TSTRING)
				fn(std::string(lua_tostring(L, -2)), lua_gettop(L));
			lua_pop(L, 1);
		}
	}

	static std::vector<std::string> readStrings(lua_State *L, int tbl, const char *key)
	{
		std::vector<std::string> out;
		lua_getfield(L, tbl, key);
		if (lua_istable(L, -1))
			out = readArray(L, lua_gettop(L));
		lua_pop(L, 1);
		return out;
	}

	static std::vector<std::string> readArray(lua_State *L, int arr)
	{
		std::vector<std::string> out;
		for (int i = 1;; i++)
		{
			lua_rawgeti(L, arr, i);
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				break;
			}
			if (lua_type(L, -1) == LUA_TSTRING)
				out.push_back(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		return out;
	}

	static bool readPair(lua_State *L, int tbl, const char *key, float &a, float &b)
	{
		bool ok = false;
		lua_getfield(L, tbl, key);
		if (lua_istable(L, -1))
		{
			int t = lua_gettop(L);
			lua_rawgeti(L, t, 1);
			lua_rawgeti(L, t, 2);
			if (lua_isnumber(L, -2) && lua_isnumber(L, -1))
			{
				a = (float)lua_tonumber(L, -2);
				b = (float)lua_tonumber(L, -1);
				ok = true;
			}
			lua_pop(L, 2);
		}
		lua_pop(L, 1);
		return ok;
	}

	static bool readNumber(lua_State *L, int tbl, const char *key, float &out)
	{
		bool ok = false;
		lua_getfield(L, tbl, key);
		if (lua_isnumber(L, -1))
		{
			out = (float)lua_tonumber(L, -1);
			ok = true;
		}
		lua_pop(L, 1);
		return ok;
	}

	static bool readBool(lua_State *L, int tbl, const char *key, bool &out)
	{
		bool ok = false;
		lua_getfield(L, tbl, key);
		if (lua_isboolean(L, -1))
		{
			out = lua_toboolean(L, -1) != 0;
			ok = true;
		}
		lua_pop(L, 1);
		return ok;
	}

	static bool readString(lua_State *L, int tbl, const char *key, std::string &out)
	{
		bool ok = false;
		lua_getfield(L, tbl, key);
		if (lua_type(L, -1) == LUA_TSTRING)
		{
			out = lua_tostring(L, -1);
			ok = true;
		}
		lua_pop(L, 1);
		return ok;
	}

	/** ---- Section readers ---- */

	void readUnits(lua_State *L, int ns, const char *tableName, Kind kind)
	{
		lua_getfield(L, ns, tableName);
		if (lua_istable(L, -1))
		{
			int tbl = lua_gettop(L);
			eachStringKey(L, tbl, [&](const std::string &name, int v)
			{
				Unit u;
				u.name = name;
				u.kind = kind;

				if (lua_type(L, v) == LUA_TSTRING)
				{
					u.gimmick = lua_tostring(L, v);
				}
				else if (lua_istable(L, v))
				{
					readString(L, v, "gimmick", u.gimmick);
					readString(L, v, "team", u.team);
					bool r;
					if (readBool(L, v, "random", r))
						u.randomSet = r ? 1 : 0;
				}
				else
				{
					CUSTOM_LOG("[Custom] %s.%s must be a string or a table, skipped", tableName, name.c_str());
					return;
				}

				if (u.gimmick.empty())
				{
					CUSTOM_LOG("[Custom] %s.%s has no gimmick, using %s", tableName, name.c_str(), kGeneric);
					u.gimmick = kGeneric;
				}

				if (unitIndex_.count(name))
				{
					CUSTOM_LOG("[Custom] '%s' is declared twice, keeping the first one", name.c_str());
					return;
				}
				unitIndex_[name] = units_.size();
				units_.push_back(u);
			});
		}
		lua_pop(L, 1);
	}

	void readMons(lua_State *L, int ns)
	{
		lua_getfield(L, ns, "CustomMons");
		if (lua_istable(L, -1))
		{
			eachStringKey(L, lua_gettop(L), [&](const std::string &name, int v)
			{
				MonProfile p;
				p.name = name;
				if (lua_type(L, v) == LUA_TSTRING)
				{
					p.gimmick = lua_tostring(L, v);
				}
				else if (lua_istable(L, v))
				{
					readString(L, v, "gimmick", p.gimmick);

					std::string mode;
					if (readString(L, v, "mode", mode))
					{
						if (mode == "attack")
							p.mode = MonMode::Attack;
						else if (mode == "ai")
							p.mode = MonMode::Ai;
						else if (mode == "none")
							p.mode = MonMode::None;
						else
							CUSTOM_LOG("[Custom] CustomMons.%s: unknown mode '%s' (use attack, ai or none), using attack", name.c_str(), mode.c_str());
					}

					readPair(L, v, "offset", p.offX, p.offY);
					p.hasAnchor = readPair(L, v, "anchor", p.anchorX, p.anchorY);
					readBool(L, v, "armorBroken", p.armorBroken);
					readString(L, v, "effect", p.effect);
					readBool(L, v, "track", p.track);
					readMonMove(L, v, name, p.move);

					if (!p.gimmick.empty())
					{
						lua_getfield(L, v, "mode");
						bool hasTweaks = !lua_isnil(L, -1);
						lua_pop(L, 1);
						for (const char *key : {"offset", "anchor", "armorBroken", "effect", "track", "move"})
						{
							lua_getfield(L, v, key);
							hasTweaks = hasTweaks || !lua_isnil(L, -1);
							lua_pop(L, 1);
						}
						if (hasTweaks)
							CUSTOM_LOG("[Custom] CustomMons.%s copies '%s' exactly: mode/offset/anchor/armorBroken/effect/track/move are ignored", name.c_str(), p.gimmick.c_str());
					}
				}
				else
				{
					CUSTOM_LOG("[Custom] CustomMons.%s must be a string or a table, skipped", name.c_str());
					return;
				}
				mons_[name] = p;
			});
		}
		lua_pop(L, 1);
	}

	/** move = { 'direct', length, seconds [, reverse] } | { 'ease', length, rate } | { 'chase', seconds } */
	static void readMonMove(lua_State *L, int tbl, const std::string &mon, MonMove &out)
	{
		lua_getfield(L, tbl, "move");
		if (lua_istable(L, -1))
		{
			int m = lua_gettop(L);
			auto num = [&](int i, double fallback)
			{
				lua_rawgeti(L, m, i);
				double n = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : fallback;
				lua_pop(L, 1);
				return n;
			};

			lua_rawgeti(L, m, 1);
			std::string type = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "";
			lua_pop(L, 1);

			if (type == "direct")
			{
				out.type = MonMoveType::Direct;
				out.length = (int)num(2, 128);
				out.time = (float)num(3, 2.0);
				lua_rawgeti(L, m, 4);
				out.reverse = lua_isboolean(L, -1) && lua_toboolean(L, -1);
				lua_pop(L, 1);
			}
			else if (type == "ease")
			{
				out.type = MonMoveType::Ease;
				out.length = (int)num(2, 224);
				out.time = (float)num(3, 1.0);
			}
			else if (type == "chase")
			{
				out.type = MonMoveType::Chase;
				out.time = (float)num(2, 2.0);
			}
			else
			{
				CUSTOM_LOG("[Custom] CustomMons.%s: move must start with 'direct', 'ease' or 'chase', ignored", mon.c_str());
			}
		}
		lua_pop(L, 1);
	}

	void readBullets(lua_State *L, int ns)
	{
		lua_getfield(L, ns, "CustomBullets");
		if (lua_istable(L, -1))
		{
			eachStringKey(L, lua_gettop(L), [&](const std::string &name, int v)
			{
				BulletProfile p;
				p.name = name;
				if (lua_istable(L, v))
				{
					p.hasOffY = readPair(L, v, "offset", p.offX, p.offY);
					readNumber(L, v, "scale", p.scale);
					readPair(L, v, "move", p.moveDist, p.moveTime);
				}
				bullets_[name] = p;
			});
		}
		lua_pop(L, 1);
	}

	/** LinkSummon.Name = 'Owner'  |  { 'Owner' }  |  { 'Owner', key } */
	void readLinks(lua_State *L, int ns)
	{
		lua_getfield(L, ns, "LinkSummon");
		if (lua_istable(L, -1))
		{
			eachStringKey(L, lua_gettop(L), [&](const std::string &summon, int v)
			{
				if (lua_type(L, v) == LUA_TSTRING)
				{
					owner_[summon] = lua_tostring(L, v);
					return;
				}

				if (lua_istable(L, v))
				{
					lua_rawgeti(L, v, 1);
					bool ownerOk = lua_type(L, -1) == LUA_TSTRING;
					std::string owner = ownerOk ? lua_tostring(L, -1) : "";
					lua_pop(L, 1);

					if (!ownerOk)
					{
						CUSTOM_LOG("[Custom] LinkSummon.%s: the first entry must be the owner's name, skipped", summon.c_str());
						return;
					}

					owner_[summon] = owner;

					lua_rawgeti(L, v, 2);
					if (lua_isnumber(L, -1))
						key_[summon] = (int)lua_tonumber(L, -1);
					else if (!lua_isnil(L, -1))
						CUSTOM_LOG("[Custom] LinkSummon.%s: the key must be a number, link kept without a key", summon.c_str());
					lua_pop(L, 1);
					return;
				}

				CUSTOM_LOG("[Custom] LinkSummon.%s must be the owner's name or { owner, key }, skipped", summon.c_str());
			});
		}
		lua_pop(L, 1);
	}

	/** Optional  via = 'skill1'|'skill2'|'skill3'|'ougis1'|'ougis2',  below = 0..1  on a chain table. */
	void readTransformAi(lua_State *L, int chainTbl, const std::vector<std::string> &chain)
	{
		std::string via;
		if (!readString(L, chainTbl, "via", via) || chain.size() < 2)
			return;

		TransformAi ai;
		if (via == "skill1")
			ai.slot = 1;
		else if (via == "skill2")
			ai.slot = 2;
		else if (via == "skill3")
			ai.slot = 3;
		else if (via == "ougis1")
			ai.slot = 4;
		else if (via == "ougis2")
			ai.slot = 5;
		else
		{
			CUSTOM_LOG("[Custom] Transform chain '%s': via must be skill1, skill2, skill3, ougis1 or ougis2, got '%s'; ignored", chain[0].c_str(), via.c_str());
			return;
		}

		float below = 1.0f;
		if (readNumber(L, chainTbl, "below", below))
		{
			if (below <= 0.0f || below > 1.0f)
			{
				CUSTOM_LOG("[Custom] Transform chain '%s': below must be between 0 and 1, using 1 (any HP)", chain[0].c_str());
				below = 1.0f;
			}
			ai.below = below;
		}

		for (auto &form : chain)
			transformAi_[form] = ai;
	}

	/** CustomBuffs = { Unit = { [17] = 'Suigetsu', [18] = 'Kakashi' } } */
	void readBuffs(lua_State *L, int ns)
	{
		lua_getfield(L, ns, "CustomBuffs");
		if (lua_istable(L, -1))
		{
			eachStringKey(L, lua_gettop(L), [&](const std::string &unit, int v)
			{
				if (!lua_istable(L, v))
				{
					CUSTOM_LOG("[Custom] CustomBuffs.%s must be a table { [id] = 'Character' }, skipped", unit.c_str());
					return;
				}

				lua_pushnil(L);
				while (lua_next(L, v) != 0)
				{
					// key at -2, value at -1
					if (lua_type(L, -2) == LUA_TNUMBER && lua_type(L, -1) == LUA_TSTRING)
						buffs_[unit][(int)lua_tonumber(L, -2)] = lua_tostring(L, -1);
					else
						CUSTOM_LOG("[Custom] CustomBuffs.%s: entries must look like [17] = 'Suigetsu', one skipped", unit.c_str());
					lua_pop(L, 1);
				}
			});
		}
		lua_pop(L, 1);
	}

	void readTransforms(lua_State *L, int ns)
	{
		lua_getfield(L, ns, "Transform");
		if (lua_istable(L, -1))
		{
			int tbl = lua_gettop(L);
			for (int i = 1;; i++)
			{
				lua_rawgeti(L, tbl, i);
				if (lua_isnil(L, -1))
				{
					lua_pop(L, 1);
					break;
				}
				if (lua_istable(L, -1))
				{
					int chainTbl = lua_gettop(L);
					auto chain = readArray(L, chainTbl);
					readTransformAi(L, chainTbl, chain);
					for (size_t k = 0; k + 1 < chain.size(); k++)
					{
						if (next_.count(chain[k]))
							CUSTOM_LOG("[Custom] '%s' is in more than one transform chain, keeping the first", chain[k].c_str());
						else
							next_[chain[k]] = chain[k + 1];
					}
					if (chain.size() >= 2)
					{
						auto &list = forms_[chain[0]];
						for (size_t k = 1; k < chain.size(); k++)
						{
							list.push_back(chain[k]);
							nonBase_.insert(chain[k]);
							base_[chain[k]] = chain[0];
						}
					}
				}
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);
	}

	void readExtras(lua_State *L, int ns)
	{
		lua_getfield(L, ns, "ExtraPlists");
		if (lua_istable(L, -1))
		{
			eachStringKey(L, lua_gettop(L), [&](const std::string &name, int v)
			{
				if (!lua_istable(L, v))
					return;
				Extra e;
				e.skills = readStrings(L, v, "Skills");
				e.kuchiyose = readStrings(L, v, "Kuchiyose");
				e.files = readStrings(L, v, "Files");
				extras_[name] = e;
			});
		}
		lua_pop(L, 1);
	}

	/** Lua table order is arbitrary: sort so behaviour doesn't change between runs. */
	void finalize()
	{
		std::sort(units_.begin(), units_.end(), [](const Unit &a, const Unit &b) { return a.name < b.name; });
		unitIndex_.clear();
		for (size_t i = 0; i < units_.size(); i++)
			unitIndex_[units_[i].name] = i;

		for (auto &kv : owner_)
			summons_[kv.second].push_back(kv.first);
		for (auto &kv : summons_)
			std::sort(kv.second.begin(), kv.second.end());

		for (auto &s : owner_)
		{
			if (!unitIndex_.count(s.first))
				CUSTOM_LOG("[Custom] LinkSummon.%s is not declared in CustomClones or CustomKuchiyose", s.first.c_str());
			else if (units_[unitIndex_[s.first]].kind == Kind::Hero)
				CUSTOM_LOG("[Custom] LinkSummon.%s is a character; summons belong in CustomClones or CustomKuchiyose", s.first.c_str());
		}

		for (auto &b : buffs_)
			if (!unitIndex_.count(b.first))
				CUSTOM_LOG("[Custom] CustomBuffs.%s is not a declared custom unit", b.first.c_str());

		// two summons of one owner sharing a key: the first (by name) wins at runtime
		for (auto &kv : summons_)
		{
			std::unordered_map<int, std::string> seen;
			for (auto &name : kv.second)
			{
				auto k = key_.find(name);
				if (k == key_.end())
					continue;
				auto dup = seen.find(k->second);
				if (dup != seen.end())
					CUSTOM_LOG("[Custom] LinkSummon: '%s' and '%s' both use key %d on '%s', '%s' wins", dup->second.c_str(), name.c_str(), k->second, kv.first.c_str(), dup->second.c_str());
				else
					seen[k->second] = name;
			}
		}

		for (auto &u : units_)
		{
			if (u.kind != Kind::Hero)
				continue;
			bool random = u.randomSet >= 0 ? (u.randomSet == 1) : isBaseForm(u.name);
			if (random)
				randomNames_.push_back(u.name);
		}
	}
};

} // namespace Custom
