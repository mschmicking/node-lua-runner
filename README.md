# node-lua-runner

Embed **Lua 5.1** in your Node.js programs.

Lua and [LuaFileSystem](https://github.com/lunarmodules/luafilesystem) are compiled directly into
the addon, so there is no system Lua to install and nothing to configure — `npm install` builds
everything from source on Linux, macOS and Windows, on both x64 and ARM64.

- [Installation](#installation)
- [Quick start](#quick-start)
- [Documentation](#documentation)
- [Examples](#examples)
- [How it works](#how-it-works)
- [Caveats](#caveats)
- [License](#license)

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
[Node-API](https://nodejs.org/api/n-api.html), which is ABI-stable — a compiled build keeps working
across future Node.js releases instead of breaking on each major version.

The API is low-level and maps closely onto the C API, and it is synchronous throughout.

Descended from [NodeLua](https://github.com/brettlangdon/NodeLua) and
[node-luajit](https://github.com/whtiehack/node-luajit).

## Caveats

This is a thin wrapper over the Lua C API, and it does not shield you from every way of misusing
it. Some stack operations on a value of an unexpected type raise an *unprotected* Lua error, which
aborts the process rather than throwing a JavaScript exception. `SetField` and `GetField` guard
against this; other methods do not. Keep track of what is on the stack — see
[stack indices](https://github.com/mschmicking/node-lua-runner/blob/master/docs/api.md#stack-indices).

## License

ISC — see [LICENSE](LICENSE). The vendored Lua and LuaFileSystem sources are MIT; their notices are
in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
