# node-lua-runner

**Real Lua 5.1 in Node.js** — for tooling that has to interoperate with a Lua 5.1 runtime you
don't control.

Lua 5.1 is what Redis scripting, OpenResty/nginx, NodeMCU and ESP-based firmware, and a great
deal of game modding still run on. If you are writing an editor plugin, a build tool or a test
harness that has to speak to one of those, the language version has to match exactly — and it has
to be the real interpreter, not a reimplementation.

This package embeds the genuine Lua 5.1.5 interpreter, compiled into a native addon, with the full
standard library and real filesystem and process access. Lua and
[LuaFileSystem](https://github.com/lunarmodules/luafilesystem) are compiled in, so there is no
system Lua to install — `npm install` builds everything from source on Linux, macOS and Windows,
on x64 and ARM64.

- [Is this the right package?](#is-this-the-right-package)
- [Installation](#installation)
- [Quick start](#quick-start)
- [Documentation](#documentation)
- [Examples](#examples)
- [How it works](#how-it-works)
- [Caveats](#caveats)
- [License](#license)

## Is this the right package?

**Use this when** you need Lua **5.1 specifically**, or you need Lua scripts to touch the real
filesystem and run real processes, or you need the actual Lua C API rather than an approximation
of it.

**Use something else when** you just want to run some Lua and don't care which version. Two good
options that need **no C++ toolchain at all**, which is a genuine advantage:

| | Approach | Toolchain needed |
|---|---|---|
| [wasmoon](https://www.npmjs.com/package/wasmoon) | "A real lua VM with JS bindings made with webassembly" | none |
| [fengari](https://www.npmjs.com/package/fengari) | "A Lua VM written in JS ES6" | none |
| **node-lua-runner** | Native addon around the genuine Lua 5.1.5 C sources | C/C++ compiler |

Both target a newer Lua than 5.1, and both run sandboxed — which is often what you want, and
exactly what you can't use when the point is matching a 5.1 target or reaching the real OS.

> **Note:** this is stock Lua 5.1.5 (PUC-Rio), not LuaJIT. Source-level and C API compatibility
> with a LuaJIT target is fine, but **compiled bytecode is not interchangeable** between LuaJIT
> and PUC-Rio Lua. If you exchange precompiled chunks with a LuaJIT runtime, this is not a drop-in
> replacement.

## Installation

```
npm install node-lua-runner
```

The addon is compiled at install time, so you need a working C/C++ toolchain:

- **Linux** — `build-essential` (or your distribution's equivalent) and Python 3
- **macOS** — the Xcode Command Line Tools (`xcode-select --install`)
- **Windows** — the "Desktop development with C++" workload from Visual Studio Build Tools

Nothing else. Node.js 18 or newer.

If the install fails, see
[troubleshooting](https://github.com/mschmicking/node-lua-runner/blob/master/docs/troubleshooting.md)
— the common ones are Visual Studio 2026 on Windows and npm 12 blocking build scripts.

## Quick start

```javascript
const nodelua = require('node-lua-runner');

const lua = new nodelua.LuaState();

lua.DoString('print("Hello from Lua!")');

// Expose a JavaScript function to Lua
lua.RegisterFunction('add', function () {
	const a = lua.ToValue(1);
	const b = lua.ToValue(2);
	lua.Pop(2);
	lua.Push(a + b);
	return 1;
});

lua.DoString('print("2 + 3 = " .. add(2, 3))');

lua.Close();
```

## Documentation

- [**API reference**](https://github.com/mschmicking/node-lua-runner/blob/master/docs/api.md) —
  every method, type conversion, and how stack indices behave
- [**Migrating to 2.0**](https://github.com/mschmicking/node-lua-runner/blob/master/docs/migrating-to-2.0.md)
  — what changed and what to check in existing code
- [**Troubleshooting**](https://github.com/mschmicking/node-lua-runner/blob/master/docs/troubleshooting.md)
  — build and install problems
- [**Development**](https://github.com/mschmicking/node-lua-runner/blob/master/docs/development.md)
  — layout, CI, and how releases are cut
- [**Changelog**](https://github.com/mschmicking/node-lua-runner/blob/master/CHANGELOG.md)

## Examples

- [Simple](https://github.com/mschmicking/node-lua-runner/blob/master/examples/simple/index.js) —
  running code, registering a JavaScript function, reading globals
- [Using `require`](https://github.com/mschmicking/node-lua-runner/blob/master/examples/lua_require/index.js)
  — loading Lua modules from disk
- [Using LuaFileSystem](https://github.com/mschmicking/node-lua-runner/tree/master/examples/lua_lfs)

## How it works

Built on the [Lua 5.1 C API](https://www.lua.org/manual/5.1/manual.html) through
[Node-API](https://nodejs.org/api/n-api.html). The API is low-level and maps closely onto the C
API, and it is synchronous throughout — a long-running Lua script blocks the event loop.

Node-API matters here beyond mere future-proofing: it is ABI-stable across Node.js **and Electron**
versions, so a single build keeps working as the host updates. That is what makes this usable
inside a VS Code extension, where the extension host's ABI changes with every release.

Descended from [NodeLua](https://github.com/brettlangdon/NodeLua) and
[node-luajit](https://github.com/whtiehack/node-luajit).

## Caveats

This is a thin wrapper over the Lua C API, and it does not shield you from every way of misusing
it. Some stack operations on a value of an unexpected type raise an *unprotected* Lua error, which
aborts the process rather than throwing a JavaScript exception. `SetField` and `GetField` guard
against this; other methods do not. Keep track of what is on the stack — see
[stack indices](https://github.com/mschmicking/node-lua-runner/blob/master/docs/api.md#stack-indices).

Lua scripts get the full standard library, including `os.execute` and `io.open`. That is the
point of this package, and it means **you should not run Lua you do not trust**.

## License

ISC — see [LICENSE](LICENSE). The vendored Lua and LuaFileSystem sources are MIT; their notices are
in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
