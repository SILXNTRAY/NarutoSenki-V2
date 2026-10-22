#pragma once

// Optional encryption of Lua files (see tools/lua-crypt/README.md).
//
// The engine decrypts any Lua file that STARTS WITH kSign using kKey (XXTEA). Files that don't start with
// it load as plain Lua, exactly as before, so nothing changes until you ship an encrypted .luac.
//
// tools/lua-crypt/lua_crypt.py reads these two values from this file, so there is only one place to edit.
//
// Change kKey before you publish anything. It is compiled into the game, so it only deters casual
// editing; it is not real security. If you change either value, re-encrypt your files with the new one.
namespace LuaCrypt
{
static constexpr const char *kKey = "476d466afacca9c6";
static constexpr const char *kSign = "NSLUA1";
} // namespace LuaCrypt
