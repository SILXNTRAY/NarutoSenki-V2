# lua-crypt: optional release-time encryption of `custom.lua`

**This is optional.** The game runs fine, and you normally work, with the plain `custom.lua`. Use this only
when you are **sure you want to upload (publish) your `custom.lua` with your edits and don't want it
tampered with**.

If you are not about to upload it, **don't do this.** An encrypted file can't be edited, nobody (including
you) can read it, and it adds nothing while you are still working.

## Please keep the plain `custom.lua`

**Save a spare copy of the plain `.lua` and never delete it.** The encrypted file is a one-way street for
editing: you can't change it, and it is only recoverable with the game's key. Your plain `.lua` is your
only editable copy. The tool never touches or deletes it, and writes its output to `./dist/`.

## Using it

```
cd tools/lua-crypt
python3 lua_crypt.py encrypt ../../projects/NarutoSenki/lua/class/custom.lua
#   -> dist/custom.luac

python3 lua_crypt.py check   dist/custom.luac                 # does it decrypt with the current key?
python3 lua_crypt.py decrypt dist/custom.luac -o back.lua     # get the script back (needs the key)
```

To publish: ship `custom.luac` at `lua/class/custom.luac` **instead of** `custom.lua`. Don't ship both: the
game loads the `.luac` first, so the `.lua` next to it would only invite edits that do nothing.

### Gotcha: a stale `custom.luac` hides your edits

The loader prefers `class/custom.luac` over `class/custom.lua`. If a `custom.luac` is sitting next to the
`custom.lua` you are editing, the game loads the `.luac` and **ignores your changes**. That is why the tool
writes to `./dist/` and not next to the source. Keep any `.luac` out of your working `lua/class/` folder.

## The key

The key and signature live in `projects/NarutoSenki/Classes/Constants/LuaCrypt.hpp` (`kKey`, `kSign`). The
script reads them from that same file, so the game and the tool can't drift apart. **Change `kKey` before
you publish**, rebuild the game, and re-encrypt. A `.luac` made with another key is rejected.

The engine only decrypts files that start with the signature. Everything else loads as plain Lua, so
enabling this changes nothing for anyone who never ships an encrypted file.

## What it does and doesn't protect

* It stops casual editing and reading. Changing even one byte of the file, a wrong key, or a truncated
  file all make the game refuse it with a clear error in the log (`cannot decrypt "class/custom" ...`),
  instead of silently loading nothing.
* It is **not** real security. The key is compiled into the game, so a determined person can extract it.
  There is also no signature check: the game still accepts a plain script, so someone can delete the
  `.luac` and put their own `custom.lua` there. It makes tampering a deliberate act, not an accident.

## Notes

* XXTEA only. It does not compile to bytecode: the engine runs LuaJIT, so the stock `luac` would not
  work anyway, and `luajit -b` output is tied to one LuaJIT version and word size.
* Works for any Lua file loaded with `require`, not just `custom.lua`.
