# Troubleshooting

This package compiles from source at install time, so most problems are build problems.

## Windows: `find VS unknown version "undefined"`

```
gyp ERR! find VS unknown version "undefined" found at "C:\Program Files\Microsoft Visual Studio\18\..."
gyp ERR! find VS could not find a version of Visual Studio 2017 or newer to use
```

`node-gyp` 11.x cannot detect Visual Studio 2026, and it is what npm bundles on Node.js 20 and 22.
Node.js 24 already ships a newer node-gyp and is unaffected.

Install node-gyp 12 and point npm at it. Note the explicit `@12` — `@latest` still resolves to 11.x
on those Node versions, so it does not fix anything:

```
npm install -g node-gyp@12
set npm_config_node_gyp=%APPDATA%\npm\node_modules\node-gyp\bin\node-gyp.js
npm install node-lua-runner
```

## npm blocks the build script

```
npm warn install-scripts node-lua-runner@2.0.0 (install: node-gyp rebuild)
```

npm 12 blocks install scripts by default, and this package needs its one to compile the addon.
Approve it:

```
npm install-scripts approve node-lua-runner
npm install
```

This affects every native addon, not just this package.

## `Cannot find module '.../build/Release/nodelua'`

The addon was not compiled. Usually the install script was blocked (see above) or the build failed
earlier in the log. Rebuild and read the output:

```
npx node-gyp rebuild
```

## The build cannot find a compiler

You need a C/C++ toolchain:

- **Linux** — `build-essential` (or your distribution's equivalent) and Python 3
- **macOS** — the Xcode Command Line Tools (`xcode-select --install`)
- **Windows** — the "Desktop development with C++" workload from Visual Studio Build Tools

## The process exits without an error

Some stack operations on a value of an unexpected type raise an *unprotected* Lua error, which
aborts the process instead of throwing. `SetField` and `GetField` guard against this; other methods
do not.

If a call vanishes without a JavaScript exception, check what was actually on the stack at that
point — usually an index is off by one, or a value was popped earlier than intended. See
[stack indices](api.md#stack-indices).

## `require('lfs')` fails

LuaFileSystem is compiled into the addon and registered in `package.preload`, so `require('lfs')`
should work on every platform. If it does not, you are almost certainly running 1.x, where it was
Windows-only. Check `require('node-lua-runner/package.json').version`.
