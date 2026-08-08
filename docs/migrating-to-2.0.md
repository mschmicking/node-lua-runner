# Migrating to 2.0

2.0.0 is a maintenance release that makes the package build and run on current Node.js. It fixes
several long-standing bugs, and those fixes change behaviour.

## Behaviour changes

| Change | Before | Now |
|---|---|---|
| `SetField(index, key, value)` | Wrote the *key* into the field, ignoring the value | Writes the value, and resolves a relative `index` before pushing |
| `LoadFile` / `LoadString` | Executed the chunk, identical to `DoFile` / `DoString` | Compile and push the chunk without running it; call `Call(0, 0)` to run it |
| Lua booleans read into JS | Arrived as the numbers `1` and `0` | Arrive as `true` and `false` |
| `Push(3.5)` | Truncated to `3` | Keeps the fractional value |
| `AddPackagePath` | Appended without a separator, corrupting `package.path`, so `require` usually failed | Appends correctly, and no longer breaks on paths containing quotes |
| `ToValue` on a table | Only converted correctly when the table was at the top of the stack | Works at any stack index, including nested tables |
| Calling a method after `Close()` | Use-after-free | Throws |
| `Resume(args)` | Returned `undefined` | Returns the Lua status code |

Most of these were broken enough that working 1.x code is unlikely to depend on them. The two worth
checking in your own code:

- **`SetField`** did not do what its name says, so anyone using it either worked around it or gave
  up on it.
- **`LoadFile` / `LoadString`** used to run the chunk. If you called one and relied on the side
  effects, add a `Call(0, 0)`.

## Environment changes

- **LuaJIT has been replaced by stock Lua 5.1.5.** Windows builds used to link LuaJIT 2.0.3, so
  Windows users lose JIT compilation. In exchange, macOS on Apple Silicon and Linux on ARM64 work
  at all, which they previously did not. The Lua C API and the language are unchanged.
- **`require('lfs')` now works on every platform.** It used to be a Windows-only prebuilt DLL
  loaded through an `LUA_CPATH` hack; LuaFileSystem is now compiled into the addon.
- **No system Lua is needed.** Linux previously required you to build and install LuaJIT by hand.
- **Node.js 18 or newer is required.**

## Why the binding was rewritten

1.x was built on NAN, which tracks V8's unstable C++ API and no longer compiles on current Node —
the build failed inside `nan.h` itself, before reaching any of this project's code.

2.0.0 uses [Node-API](https://nodejs.org/api/n-api.html), which is ABI-stable. A compiled build
keeps working across future Node.js major versions instead of breaking on each one.
