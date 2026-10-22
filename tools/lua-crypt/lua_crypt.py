#!/usr/bin/env python3
"""
lua_crypt.py - optional release-time encryption of a Lua file (for example custom.lua).

    python3 lua_crypt.py encrypt  path/to/custom.lua             -> dist/custom.luac
    python3 lua_crypt.py decrypt  dist/custom.luac -o back.lua    (recover / check a .luac)
    python3 lua_crypt.py check    dist/custom.luac                (is it valid for the current key?)

Read tools/lua-crypt/README.md first. In short:
  * Encrypt ONLY when you are about to publish. Keep editing the plain .lua.
  * NEVER delete the plain .lua. The encrypted file cannot be edited, and without the
    key (compiled into the game) it cannot be turned back into a script you can maintain.
  * The output goes to ./dist/ and never overwrites or deletes your source.

The key and signature are read from Classes/Constants/LuaCrypt.hpp, the same file the game is built
with, so they cannot drift apart. The algorithm is the XXTEA used by the engine's
scripting/lua/xxtea/xxtea.cpp (key zero-padded to 16 bytes, data as little-endian 32-bit words with the
byte length appended as a last word).
"""
import argparse
import os
import re
import struct
import sys

M = 0xFFFFFFFF
DELTA = 0x9E3779B9
HEADER = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'projects', 'NarutoSenki',
                      'Classes', 'Constants', 'LuaCrypt.hpp')


# ---------------------------------------------------------------------------------------------------
# XXTEA, byte-for-byte compatible with xxtea.cpp
# ---------------------------------------------------------------------------------------------------

def _mx(z, y, s, k, p, e):
    a = ((z >> 5) ^ ((y << 2) & M)) + ((y >> 3) ^ ((z << 4) & M))
    b = (s ^ y) + (k[(p & 3) ^ e] ^ z)
    return ((a & M) ^ (b & M)) & M


def _to_words(data, include_length):
    n = (len(data) + 3) // 4
    w = list(struct.unpack('<%dI' % n, data + b'\0' * (n * 4 - len(data)))) if n else []
    if include_length:
        w.append(len(data))
    return w


def _key_words(key):
    k = key[:16] + b'\0' * (16 - len(key[:16]))
    return list(struct.unpack('<4I', k))


def encrypt(data, key):
    v = _to_words(data, True)
    k = _key_words(key)
    n = len(v) - 1
    if n >= 1:
        q = 6 + 52 // (n + 1)
        s = 0
        z = v[n]
        while q > 0:
            q -= 1
            s = (s + DELTA) & M
            e = (s >> 2) & 3
            for p in range(n):
                y = v[p + 1]
                v[p] = (v[p] + _mx(z, y, s, k, p, e)) & M
                z = v[p]
            y = v[0]
            v[n] = (v[n] + _mx(z, y, s, k, n, e)) & M
            z = v[n]
    return struct.pack('<%dI' % len(v), *v)


def decrypt(data, key):
    """Returns the plain bytes, or None if the key is wrong / the data was changed (what the engine does too)."""
    if len(data) % 4 or len(data) < 8:
        return None
    v = list(struct.unpack('<%dI' % (len(data) // 4), data))
    k = _key_words(key)
    n = len(v) - 1
    if n >= 1:
        q = 6 + 52 // (n + 1)
        s = (q * DELTA) & M
        y = v[0]
        while s != 0:
            e = (s >> 2) & 3
            for p in range(n, 0, -1):
                z = v[p - 1]
                v[p] = (v[p] - _mx(z, y, s, k, p, e)) & M
                y = v[p]
            z = v[n]
            v[0] = (v[0] - _mx(z, y, s, k, 0, e)) & M
            y = v[0]
            s = (s - DELTA) & M
    total = len(v) * 4
    m = v[-1]
    if m < total - 7 or m > total - 4:
        return None
    return struct.pack('<%dI' % len(v), *v)[:m]


# ---------------------------------------------------------------------------------------------------

def read_key_and_sign(header):
    text = open(header, encoding='utf8').read()
    def grab(name):
        m = re.search(r'\b%s\s*=\s*"([^"]*)"' % name, text)
        if not m:
            sys.exit('error: could not find %s in %s' % (name, header))
        return m.group(1).encode('utf8')
    return grab('kKey'), grab('kSign')


def main():
    ap = argparse.ArgumentParser(description='Optional release-time encryption of a Lua file.')
    ap.add_argument('command', choices=['encrypt', 'decrypt', 'check'])
    ap.add_argument('input')
    ap.add_argument('-o', '--output', help='output path (default: ./dist/<name>.luac, or <name>.decrypted.lua)')
    ap.add_argument('--header', default=HEADER, help='LuaCrypt.hpp to read the key and signature from')
    ap.add_argument('--key', help='override the key (must match what the game was built with)')
    ap.add_argument('--sign', help='override the signature')
    a = ap.parse_args()

    key, sign = read_key_and_sign(a.header)
    if a.key:
        key = a.key.encode('utf8')
    if a.sign:
        sign = a.sign.encode('utf8')

    data = open(a.input, 'rb').read()
    base = os.path.splitext(os.path.basename(a.input))[0]

    if a.command == 'encrypt':
        if data.startswith(sign):
            sys.exit('error: %s is already encrypted. Encrypt the plain .lua instead.' % a.input)
        if not data:
            sys.exit('error: %s is empty; the game cannot load an empty encrypted file.' % a.input)
        if a.input.lower().endswith('.luac'):
            sys.exit('error: %s looks like an encrypted/compiled file, not a script.' % a.input)
        out = a.output or os.path.join('dist', base + '.luac')
        if os.path.abspath(out) == os.path.abspath(a.input):
            sys.exit('error: refusing to overwrite the input file.')
        blob = sign + encrypt(data, key)
        # prove it round-trips before anything is written
        if decrypt(blob[len(sign):], key) != data:
            sys.exit('error: internal check failed, nothing written.')
        os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
        open(out, 'wb').write(blob)
        print('encrypted  %s  ->  %s  (%d bytes)' % (a.input, out, len(blob)))
        print()
        print('Next:')
        print('  * Ship %s in place of the plain file, next to where %s.lua lives.' % (os.path.basename(out), base))
        print('  * Do NOT ship %s.lua with it: the game prefers the .luac and the plain one only invites edits.' % base)
        print('  * KEEP %s. The .luac cannot be edited, and it is your only editable copy.' % a.input)
        print('  * Don\'t keep a stale %s next to the .lua you keep editing: the game would load it and ignore your edits.'
              % os.path.basename(out))
        return

    if not data.startswith(sign):
        sys.exit('error: %s does not start with the signature %r (not encrypted, or a different signature).'
                 % (a.input, sign.decode('utf8', 'replace')))
    plain = decrypt(data[len(sign):], key)
    if plain is None:
        sys.exit('error: cannot decrypt %s (wrong key, or the file was changed after it was encrypted).' % a.input)
    if a.command == 'check':
        print('ok: %s decrypts with the current key (%d bytes of Lua).' % (a.input, len(plain)))
        return
    out = a.output or base + '.decrypted.lua'
    if os.path.abspath(out) == os.path.abspath(a.input):
        sys.exit('error: refusing to overwrite the input file.')
    open(out, 'wb').write(plain)
    print('decrypted  %s  ->  %s' % (a.input, out))


if __name__ == '__main__':
    main()
