# node-lua-runner

Embed **Lua 5.1** in your Node.js programs.

Lua and [LuaFileSystem](https://github.com/lunarmodules/luafilesystem) are compiled directly into
the addon, so there is no system Lua to install and nothing to configure — `npm install` builds
everything from source on Linux, macOS and Windows, on both x64 and ARM64.

## Installation

```
npm install node-lua-runner
```

The addon is compiled at install time, so you need a working C/C++ toolchain:

- **Linux** — `build-essential` (or your distribution's equivalent) and Python 3
- **macOS** — the Xcode Command Line Tools (`xcode-select --install`)
- **Windows** — the "Desktop development with C++" workload from Visual Studio Build Tools

Nothing else. Earlier versions needed you to build and install LuaJIT yourself on Linux; that is
no longer the case.

> **Windows with Visual Studio 2026:** `node-gyp` 11.x cannot detect that version and fails with
> `find VS unknown version "undefined"`. It is what npm bundles on Node.js 20 and 22. Install
> node-gyp 12 and point npm at it — note the explicit `@12`, since `@latest` still resolves to
> 11.x on those Node versions:
>
> ```
> npm install -g node-gyp@12
> set npm_config_node_gyp=%APPDATA%\npm\node_modules\node-gyp\bin\node-gyp.js
> npm install node-lua-runner
> ```

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

## Breaking changes in 2.0.0

2.0.0 is a maintenance release that makes the package build and run on current Node.js. It fixes
several long-standing bugs, and those fixes change behaviour:

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

Other things worth knowing if you are upgrading:

- **LuaJIT has been replaced by stock Lua 5.1.5.** Windows builds used to link LuaJIT 2.0.3, so
  Windows users lose JIT compilation. In exchange, macOS on Apple Silicon and Linux on ARM64 work
  at all, which they previously did not. The Lua C API and language are unchanged.
- **`require('lfs')` now works on every platform.** It used to be a Windows-only prebuilt DLL
  loaded through an `LUA_CPATH` hack; LuaFileSystem is now compiled into the addon.
- Node.js 18 or newer is required.

## About

Built on the [Lua 5.1 C API](https://www.lua.org/manual/5.1/manual.html). The binding uses
[Node-API](https://nodejs.org/api/n-api.html), which is ABI-stable — a compiled build keeps
working across future Node.js releases instead of breaking on each major version.

**Based on:**
- nodelua ( https://github.com/brettlangdon/NodeLua )
- node-luajit ( https://github.com/whtiehack/node-luajit )
- LuaFileSystem ( https://github.com/lunarmodules/luafilesystem )

**Features:**
- Low-level API mapping closely onto the Lua C API
- Synchronous only

## Examples

- [Simple](https://github.com/mschmicking/node-lua-runner/blob/master/examples/simple/index.js)
- [Using the lua require function](https://github.com/mschmicking/node-lua-runner/blob/master/examples/lua_require/index.js)
- [Using LuaFileSystem](https://github.com/mschmicking/node-lua-runner/tree/master/examples/lua_lfs)

## API

```javascript

const nodelua = require('node-lua-runner');

var lua = new nodelua.LuaState();


/**
 * [Add a path to the lua package.path variable. Set a root path for lua require (see example)]
 * @param  {String} path
 */
lua.AddPackagePath(__dirname);


/**
 * [Loads and runs the given file]
 * @type {String} file
 * @throws {Exception}
 */
lua.DoFile(__dirname + "/test.lua");


/**
 * [Loads and runs the given string]
 * @type {String} str
 * @throws {Exception}
 */
lua.DoString("print('Hello world!')");


/**
 * [Compiles the given file and pushes it onto the stack WITHOUT running it.
 *  Use Call to run it.]
 * @type {String} file
 * @throws {Exception}
 */
lua.LoadFile(__dirname + "/test.lua");


/**
 * [Compiles the given string and pushes it onto the stack WITHOUT running it.
 *  Use Call to run it.]
 * @type {String} str
 * @throws {Exception}
 */
lua.LoadString("print('Hello world!')");


/**
 * [Sets the function f as the new value of global name.
 *  Arguments are read off the Lua stack with ToValue; the return value is the
 *  number of results the function pushed.]
 * @param  {String} name [name of the global in lua]
 * @param  {Function} f [function to set]
 */
lua.RegisterFunction('add', function() {
	var a = lua.ToValue(1);
	var b = lua.ToValue(2);
	lua.Pop(2);
	lua.Push(a + b);
	return 1;
});


/**
 * [Pops a value from the stack and sets it as the new value of global name]
 * @type {String} name
 */
lua.SetGlobal("myVar");


/**
 * [Pushes onto the stack the value of the global name]
 * @type {String} name
 */
lua.GetGlobal('myVar');


/**
 * [Does the equivalent to t[k] = v, where t is the value at the given valid index.
 *  Unlike the raw C API, v is passed as an argument rather than taken from the
 *  top of the stack.]
 * @type {Number} index
 * @type {String} key
 * @type {*} value
 * @throws {Exception} [if the value at index is not a table]
 */
lua.SetField(index, "key", value);


/**
 * [Pushes onto the stack the value t[key], where t is the value at the given valid index.]
 * @type {Number} index
 * @type {String} key
 * @throws {Exception} [if the value at index is not a table]
 */
lua.GetField(index, "key");


/**
 * [Get value at the given acceptable index]
 * @type {Number} index
 * @return value
 */
var value = lua.ToValue(-1);


/**
 * [Calls a function. Gets the function and arguments from the stack. Pushes the results onto the stack. See https://www.lua.org/manual/5.1/manual.html#pdf-pcall for more information]
 * @type {Number} args
 * @type {Number} results
 * @throws {Exception}
 */
lua.Call(args, results);


/**
 * [Yields a coroutine.]
 * @type {Number} args
 */
lua.Yield(args);


/**
 * [Starts and resumes a coroutine in a given thread.]
 * @type {Number} args
 * @return {Number} [status code]
 */
lua.Resume(args);


/**
 * [Pushes a value n onto the stack]
 * @type n
 */
lua.Push(5);


/**
 * [Pops n elements from the stack. Default value is 1]
 * @type {Number} n
 */
lua.Pop();
lua.Pop(n);


/**
 * [Returns the index of the top element in the stack. Because indices start at 1, this result is equal to the number of elements in the stack (and so 0 means an empty stack)]
 * @return {Number}
 */
var size = lua.GetTop();


/**
 * [Accepts any acceptable index, or 0, and sets the stack top to this index. If the new top is larger than the old one, then the new elements are filled with nil. If index is 0, then all stack elements are removed]
 * @type {Number} index
 */
lua.SetTop(index);


/**
 * [Moves the top element into the given position (and pops it), without shifting any element (therefore replacing the value at the given position)]
 * @type {Number} index
 */
lua.Replace(index);


/**
 * [Returns the status of the state]
 * @return {Number} [compare against nodelua.STATUS.*]
 */
lua.Status();


/**
 * [Controls the Lua garbage collector]
 * @type {Number} what [one of nodelua.GC.*]
 * @return {Number}
 */
lua.CollectGarbage(nodelua.GC.COLLECT);


/**
 * [Destroys the Lua state and frees its memory. Safe to call more than once.
 *  Any further use of the state throws.]
 */
lua.Close();

```

### Constants

```javascript
nodelua.INFO.VERSION       // "Lua 5.1"
nodelua.INFO.VERSION_NUM   // 501
nodelua.INFO.COPYRIGHT
nodelua.INFO.AUTHORS

nodelua.LUA.GLOBALSINDEX   // pseudo-index of the globals table
nodelua.LUA.REGISTRYINDEX  // pseudo-index of the registry

nodelua.STATUS.YIELD
nodelua.STATUS.ERRRUN
nodelua.STATUS.ERRSYNTAX
nodelua.STATUS.ERRMEM
nodelua.STATUS.ERRERR

nodelua.GC.STOP
nodelua.GC.RESTART
nodelua.GC.COLLECT
nodelua.GC.COUNT
nodelua.GC.COUNTB
nodelua.GC.STEP
nodelua.GC.SETPAUSE
nodelua.GC.SETSTEPMUL
```

## Caveats

This is a thin wrapper over the Lua C API, and it does not shield you from every way of misusing
that API. Some stack operations on values of an unexpected type raise an *unprotected* Lua error,
which aborts the process rather than throwing a JavaScript exception. `SetField` and `GetField`
guard against this explicitly; other methods do not. Keep track of what is on the stack.

## Development

```
npm install
npm test
```

## License

ISC — see [LICENSE.md](LICENSE.md), which also covers the vendored Lua and LuaFileSystem sources.
