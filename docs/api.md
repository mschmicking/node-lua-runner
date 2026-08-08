# API reference

A thin wrapper over the [Lua 5.1 C API](https://www.lua.org/manual/5.1/manual.html). If a method
looks unfamiliar, the corresponding C function in that manual is the authoritative description of
what it does to the stack.

Every method is synchronous.

- [Creating a state](#creating-a-state)
- [Running code](#running-code)
- [Loading without running](#loading-without-running)
- [Globals and fields](#globals-and-fields)
- [The stack](#the-stack)
- [Calling functions](#calling-functions)
- [Calling JavaScript from Lua](#calling-javascript-from-lua)
- [Coroutines](#coroutines)
- [Lifecycle and diagnostics](#lifecycle-and-diagnostics)
- [Constants](#constants)
- [Type conversion](#type-conversion)
- [Stack indices](#stack-indices)

## Creating a state

```javascript
const nodelua = require('node-lua-runner');

const lua = new nodelua.LuaState();
```

Each `LuaState` owns an independent Lua interpreter with the standard library open and
`require('lfs')` available. States do not share globals, and callbacks registered on one are
invisible to the other.

Call [`Close()`](#close) when you are done. If you do not, the interpreter is freed when the object
is garbage collected.

## Running code

### `DoString(code)`

Compiles and runs a chunk of Lua.

- `code` `{String}`
- Throws if the chunk fails to compile or raises an error.

```javascript
lua.DoString('print("Hello world!")');
```

### `DoFile(path)`

Compiles and runs a file.

- `path` `{String}`
- Throws if the file cannot be read, fails to compile, or raises an error.

```javascript
lua.DoFile(__dirname + '/test.lua');
```

### `AddPackagePath(path)`

Appends a directory to Lua's `package.path` so `require` can find `.lua` files in it.

- `path` `{String}` — a directory. Backslashes are normalised and a trailing separator is ignored.

```javascript
lua.AddPackagePath(__dirname + '/lua');
lua.DoString('local m = require("mymodule")');
```

## Loading without running

### `LoadString(code)`

Compiles a chunk and pushes it onto the stack as a function **without running it**. Use
[`Call`](#callargs-results) to run it.

- `code` `{String}`
- Throws if the chunk fails to compile.

```javascript
lua.LoadString('counter = counter + 1');
lua.Call(0, 0);   // now it runs
```

### `LoadFile(path)`

The same, for a file.

- `path` `{String}`
- Throws if the file cannot be read or fails to compile.

> Before 2.0.0 these two methods executed the chunk, behaving identically to `DoString` / `DoFile`.
> See [migrating to 2.0](migrating-to-2.0.md).

## Globals and fields

### `SetGlobal(name)`

Pops the value on top of the stack and stores it in the global `name`.

- `name` `{String}`

```javascript
lua.Push(5);
lua.SetGlobal('myVar');
```

### `GetGlobal(name)`

Pushes the value of the global `name` onto the stack.

- `name` `{String}`

```javascript
lua.GetGlobal('myVar');
const value = lua.ToValue(-1);
lua.Pop(1);
```

### `SetField(index, key, value)`

Does `t[key] = value`, where `t` is the value at `index`.

- `index` `{Number}` — stack index of the table. Relative indices are resolved before the value is
  pushed, so `-1` means the table you are looking at.
- `key` `{String}`
- `value` `{*}` — converted per [type conversion](#type-conversion).
- Throws if the value at `index` is not a table.

Unlike the raw C API, the value is passed as an argument rather than taken from the top of the
stack.

```javascript
lua.DoString('t = {}');
lua.GetGlobal('t');
lua.SetField(-1, 'answer', 42);
lua.Pop(1);
```

### `GetField(index, key)`

Pushes `t[key]` onto the stack, where `t` is the value at `index`.

- `index` `{Number}`
- `key` `{String}`
- Throws if the value at `index` is not a table.

```javascript
lua.GetField(nodelua.LUA.GLOBALSINDEX, 'a');  // same as GetGlobal('a')
lua.GetField(-1, 't');
const t = lua.ToValue(-1);
lua.Pop(2);
```

## The stack

### `Push(value)`

Pushes a JavaScript value onto the stack. See [type conversion](#type-conversion).

### `Pop([n])`

Pops `n` elements. `n` defaults to `1`.

### `ToValue(index)`

Returns the value at `index` as a JavaScript value, leaving the stack unchanged.

- `index` `{Number}`
- Returns `{*}`

### `GetTop()`

Returns the index of the top element, which equals the number of elements on the stack. `0` means
empty.

- Returns `{Number}`

### `SetTop(index)`

Sets the stack top to `index`. Growing it fills the new slots with `nil`; `0` clears the stack.

- `index` `{Number}`

### `Replace(index)`

Moves the top element into `index` and pops it, without shifting anything else.

- `index` `{Number}`

## Calling functions

### `Call(args, results)`

Calls a function. The function and its arguments are taken from the stack; the results are pushed
back onto it. Equivalent to `lua_pcall`.

- `args` `{Number}` — how many arguments are on the stack
- `results` `{Number}` — how many results to keep
- Throws if the function raises an error.

```javascript
lua.DoString('function join(a, b) return a .. "-" .. b end');

lua.GetGlobal('join');
lua.Push('left');
lua.Push('right');
lua.Call(2, 1);

console.log(lua.ToValue(-1));   // "left-right"
lua.Pop(1);
```

## Calling JavaScript from Lua

### `RegisterFunction(name, fn)`

Makes `fn` callable from Lua as the global `name`.

- `name` `{String}`
- `fn` `{Function}` — receives **no JavaScript arguments**. Read the Lua arguments off the stack
  with [`ToValue`](#tovalueindex), starting at index `1`. Push your results and return how many you
  pushed.

```javascript
lua.RegisterFunction('add', function () {
	const a = lua.ToValue(1);
	const b = lua.ToValue(2);
	lua.Pop(2);
	lua.Push(a + b);
	return 1;         // one result pushed
});

lua.DoString('print(add(2, 3))');   // 5
```

Returning nothing (or a non-number) means no results.

If the callback throws, the exception cannot travel through Lua's C frames, so it is left pending
and surfaces once control returns to JavaScript. Lua carries on with no results from the call.

## Coroutines

### `Yield(args)`

Yields a coroutine.

- `args` `{Number}`

> Only meaningful inside a running coroutine. Calling it otherwise raises an unprotected Lua error,
> which aborts the process — see [caveats](#stack-indices).

### `Resume(args)`

Starts or resumes a coroutine.

- `args` `{Number}`
- Returns `{Number}` — the Lua status code; compare against `nodelua.STATUS.*`.

## Lifecycle and diagnostics

### `Close()`

Destroys the Lua state and frees its memory. Safe to call more than once. Any further use of the
state throws rather than crashing.

### `Status()`

Returns the state's status.

- Returns `{Number}` — compare against `nodelua.STATUS.*`. `0` means runnable.

### `CollectGarbage(what)`

Controls the Lua garbage collector.

- `what` `{Number}` — one of `nodelua.GC.*`
- Returns `{Number}`

```javascript
lua.CollectGarbage(nodelua.GC.COLLECT);
const kb = lua.CollectGarbage(nodelua.GC.COUNT);
```

## Constants

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

## Type conversion

### Lua to JavaScript

| Lua | JavaScript |
|---|---|
| `string` | `String` |
| `number` | `Number` |
| `boolean` | `Boolean` |
| `table` | `Object` — converted recursively, at any stack index |
| `nil` and everything else | `undefined` |

Lua tables are always objects, never arrays. A table written as `{"a", "b"}` arrives as
`{ 1: 'a', 2: 'b' }`, keeping Lua's 1-based keys.

### JavaScript to Lua

| JavaScript | Lua |
|---|---|
| `String` | `string` (embedded NULs preserved) |
| `Number` | `number` (fractional values kept) |
| `Boolean` | `boolean` |
| `Object` | `table` — converted recursively |
| everything else | `nil` |

Arrays are objects, so they become tables keyed by the **string** forms of their indices
(`"0"`, `"1"`, …), not 1-based Lua arrays. Build the table explicitly if that matters.

## Stack indices

Positive indices count from the bottom of the stack, negative from the top: `-1` is the top
element. `nodelua.LUA.GLOBALSINDEX` and `nodelua.LUA.REGISTRYINDEX` are pseudo-indices and are never
treated as relative.

**This is a thin wrapper and it does not shield you from every misuse of the C API.** Some stack
operations on a value of an unexpected type raise an *unprotected* Lua error, which aborts the
process rather than throwing a JavaScript exception. `SetField` and `GetField` guard against this
explicitly; other methods do not. Keep track of what you have put on the stack.
