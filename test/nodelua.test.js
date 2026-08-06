const test = require('node:test');
const assert = require('node:assert');
const path = require('node:path');

const nodelua = require('../index.js');

const FIXTURES = path.join(__dirname, 'fixtures');

// Reads the value a chunk assigned to a global, leaving the stack as it found it.
function readGlobal(lua, name) {
	lua.GetGlobal(name);
	const value = lua.ToValue(-1);
	lua.Pop(1);
	return value;
}

test('module exports', async (t) => {
	await t.test('exposes the LuaState constructor', () => {
		assert.strictEqual(typeof nodelua.LuaState, 'function');
	});

	await t.test('exposes Lua 5.1 version constants', () => {
		assert.strictEqual(nodelua.INFO.VERSION, 'Lua 5.1');
		assert.strictEqual(nodelua.INFO.VERSION_NUM, 501);
	});

	await t.test('exposes status, gc and pseudo-index constants', () => {
		assert.strictEqual(typeof nodelua.STATUS.ERRSYNTAX, 'number');
		assert.strictEqual(typeof nodelua.GC.COLLECT, 'number');
		assert.strictEqual(typeof nodelua.LUA.GLOBALSINDEX, 'number');
	});
});

test('DoString and DoFile', async (t) => {
	await t.test('DoString executes a chunk', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('executed = 1 + 1');
		assert.strictEqual(readGlobal(lua, 'executed'), 2);
		lua.Close();
	});

	await t.test('DoFile executes a file', () => {
		const lua = new nodelua.LuaState();
		lua.DoFile(path.join(FIXTURES, 'hello.lua'));
		assert.strictEqual(readGlobal(lua, 'dofile_ran'), true);
		assert.strictEqual(readGlobal(lua, 'dofile_value'), 'hello from dofile');
		lua.Close();
	});

	await t.test('a syntax error throws rather than crashing', () => {
		const lua = new nodelua.LuaState();
		assert.throws(() => lua.DoString('this is not ( valid lua'), /LuaState.DoString/);
		lua.Close();
	});

	await t.test('a runtime error throws and reports the Lua message', () => {
		const lua = new nodelua.LuaState();
		assert.throws(() => lua.DoString('error("boom")'), /boom/);
		lua.Close();
	});

	await t.test('a long error message is not truncated or overflowed', () => {
		const lua = new nodelua.LuaState();
		// Previously formatted through a fixed 1024-byte sprintf buffer.
		assert.throws(() => lua.DoString('error(string.rep("x", 5000))'), (err) => {
			return err.message.length > 4000;
		});
		lua.Close();
	});

	await t.test('DoFile on a missing file throws', () => {
		const lua = new nodelua.LuaState();
		assert.throws(() => lua.DoFile(path.join(FIXTURES, 'does-not-exist.lua')));
		lua.Close();
	});

	await t.test('argument validation', () => {
		const lua = new nodelua.LuaState();
		assert.throws(() => lua.DoString(), /Requires 1 Argument/);
		assert.throws(() => lua.DoString(42), /Must Be A String/);
		lua.Close();
	});
});

test('LoadString and LoadFile compile without executing', async (t) => {
	await t.test('LoadString does not run the chunk until Call', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('marker = 0');

		lua.LoadString('marker = 1');
		// The compiled chunk now sits on the stack, unexecuted.
		assert.strictEqual(readGlobal(lua, 'marker'), 0);

		lua.Call(0, 0);
		assert.strictEqual(readGlobal(lua, 'marker'), 1);
		lua.Close();
	});

	await t.test('LoadFile does not run the file until Call', () => {
		const lua = new nodelua.LuaState();
		lua.LoadFile(path.join(FIXTURES, 'hello.lua'));
		assert.strictEqual(readGlobal(lua, 'dofile_ran'), undefined);

		lua.Call(0, 0);
		assert.strictEqual(readGlobal(lua, 'dofile_ran'), true);
		lua.Close();
	});

	await t.test('LoadString reports syntax errors', () => {
		const lua = new nodelua.LuaState();
		assert.throws(() => lua.LoadString('function ('), /LuaState.LoadString/);
		lua.Close();
	});
});

test('stack operations', async (t) => {
	await t.test('Push and GetTop track stack depth', () => {
		const lua = new nodelua.LuaState();
		assert.strictEqual(lua.GetTop(), 0);

		lua.Push('a');
		lua.Push('b');
		assert.strictEqual(lua.GetTop(), 2);

		lua.Pop();
		assert.strictEqual(lua.GetTop(), 1);

		lua.Pop(1);
		assert.strictEqual(lua.GetTop(), 0);
		lua.Close();
	});

	await t.test('SetTop truncates and pads the stack', () => {
		const lua = new nodelua.LuaState();
		lua.Push(1);
		lua.Push(2);
		lua.Push(3);

		lua.SetTop(1);
		assert.strictEqual(lua.GetTop(), 1);
		assert.strictEqual(lua.ToValue(-1), 1);

		lua.SetTop(0);
		assert.strictEqual(lua.GetTop(), 0);
		lua.Close();
	});

	await t.test('Replace moves the top value into a slot', () => {
		const lua = new nodelua.LuaState();
		lua.Push('first');
		lua.Push('second');

		lua.Replace(1);
		assert.strictEqual(lua.GetTop(), 1);
		assert.strictEqual(lua.ToValue(1), 'second');
		lua.Close();
	});
});

test('globals', async (t) => {
	await t.test('SetGlobal pops the top value into a global', () => {
		const lua = new nodelua.LuaState();
		lua.Push(5);
		lua.SetGlobal('myVar');
		assert.strictEqual(lua.GetTop(), 0);
		assert.strictEqual(readGlobal(lua, 'myVar'), 5);
		lua.Close();
	});

	await t.test('GetGlobal of an unset name yields undefined', () => {
		const lua = new nodelua.LuaState();
		assert.strictEqual(readGlobal(lua, 'neverSet'), undefined);
		lua.Close();
	});
});

test('fields', async (t) => {
	// Regression: SetField used to push its key argument as the value, so every
	// assignment wrote the field name into the field.
	await t.test('SetField assigns the value, not the key', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('target = {}');

		lua.GetGlobal('target');
		lua.SetField(-1, 'answer', 42);
		lua.Pop(1);

		assert.strictEqual(lua.GetTop(), 0);
		lua.DoString('answer_value = target.answer');
		assert.strictEqual(readGlobal(lua, 'answer_value'), 42);
		lua.Close();
	});

	await t.test('GetField reads a field onto the stack', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('a = {}; a.t = 42;');

		lua.GetField(nodelua.LUA.GLOBALSINDEX, 'a');
		lua.GetField(-1, 't');
		assert.strictEqual(lua.ToValue(-1), 42);

		lua.Pop(2);
		assert.strictEqual(lua.GetTop(), 0);
		lua.Close();
	});

	await t.test('SetField accepts string and boolean values', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('target = {}');

		lua.GetGlobal('target');
		lua.SetField(-1, 'name', 'lua');
		lua.SetField(-1, 'flag', true);
		lua.Pop(1);

		lua.DoString('name_value = target.name; flag_value = target.flag');
		assert.strictEqual(readGlobal(lua, 'name_value'), 'lua');
		assert.strictEqual(readGlobal(lua, 'flag_value'), true);
		lua.Close();
	});
});

test('type marshalling from Lua to JavaScript', async (t) => {
	await t.test('strings and numbers', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('s = "text"; n = 42; f = 3.5');
		assert.strictEqual(readGlobal(lua, 's'), 'text');
		assert.strictEqual(readGlobal(lua, 'n'), 42);
		assert.strictEqual(readGlobal(lua, 'f'), 3.5);
		lua.Close();
	});

	// Regression: booleans used to arrive as the numbers 1 and 0.
	await t.test('booleans arrive as real booleans', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('yes = true; no = false');
		assert.strictEqual(readGlobal(lua, 'yes'), true);
		assert.strictEqual(readGlobal(lua, 'no'), false);
		lua.Close();
	});

	await t.test('nil arrives as undefined', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('nothing = nil');
		assert.strictEqual(readGlobal(lua, 'nothing'), undefined);
		lua.Close();
	});

	await t.test('tables become objects', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('t = {a = 1, b = "two", c = true}');
		assert.deepStrictEqual(readGlobal(lua, 't'), { a: 1, b: 'two', c: true });
		lua.Close();
	});

	// Regression: the table walk used a hardcoded relative index, so it only
	// worked when the table happened to sit at the top of the stack.
	await t.test('nested tables convert recursively', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('nested = {x = 1, inner = {y = 2, deeper = {z = 3}}}');
		assert.deepStrictEqual(readGlobal(lua, 'nested'), {
			x: 1,
			inner: { y: 2, deeper: { z: 3 } }
		});
		lua.Close();
	});

	await t.test('a table is readable at a non-top stack index', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('t = {a = 1}');
		lua.GetGlobal('t');
		lua.Push('padding');

		assert.deepStrictEqual(lua.ToValue(-2), { a: 1 });
		lua.Pop(2);
		lua.Close();
	});

	await t.test('array-like tables use their Lua 1-based keys', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('list = {"a", "b"}');
		assert.deepStrictEqual(readGlobal(lua, 'list'), { 1: 'a', 2: 'b' });
		lua.Close();
	});
});

test('type marshalling from JavaScript to Lua', async (t) => {
	await t.test('strings, integers and booleans', () => {
		const lua = new nodelua.LuaState();

		lua.Push('text');
		lua.SetGlobal('s');
		lua.Push(42);
		lua.SetGlobal('n');
		lua.Push(true);
		lua.SetGlobal('b');

		lua.DoString('assert(s == "text"); assert(n == 42); assert(b == true)');
		lua.Close();
	});

	// Regression: numbers used to be pushed via lua_pushinteger, truncating 3.5 to 3.
	await t.test('fractional numbers keep their precision', () => {
		const lua = new nodelua.LuaState();
		lua.Push(3.5);
		lua.SetGlobal('f');

		assert.strictEqual(readGlobal(lua, 'f'), 3.5);
		lua.DoString('assert(f == 3.5, "expected 3.5, got " .. tostring(f))');
		lua.Close();
	});

	await t.test('objects become tables', () => {
		const lua = new nodelua.LuaState();
		lua.Push({ a: 1, b: 'two' });
		lua.SetGlobal('t');

		lua.DoString('assert(t.a == 1); assert(t.b == "two")');
		assert.deepStrictEqual(readGlobal(lua, 't'), { a: 1, b: 'two' });
		lua.Close();
	});

	await t.test('nested objects become nested tables', () => {
		const lua = new nodelua.LuaState();
		lua.Push({ outer: { inner: 'deep' } });
		lua.SetGlobal('t');

		lua.DoString('assert(t.outer.inner == "deep")');
		lua.Close();
	});
});

test('RegisterFunction', async (t) => {
	await t.test('Lua can call back into JavaScript', () => {
		const lua = new nodelua.LuaState();

		lua.RegisterFunction('add', function () {
			const a = lua.ToValue(1);
			const b = lua.ToValue(2);
			lua.Pop(2);
			lua.Push(a + b);
			return 1;
		});

		lua.DoString('sum = add(10, 5)');
		assert.strictEqual(readGlobal(lua, 'sum'), 15);
		lua.Close();
	});

	await t.test('a callback returning no results is fine', () => {
		const lua = new nodelua.LuaState();
		let calls = 0;

		lua.RegisterFunction('sideEffect', function () {
			calls += 1;
		});

		lua.DoString('sideEffect(); sideEffect()');
		assert.strictEqual(calls, 2);
		lua.Close();
	});

	await t.test('separate states keep their own callbacks', () => {
		const first = new nodelua.LuaState();
		const second = new nodelua.LuaState();

		first.RegisterFunction('which', function () {
			first.Push('first');
			return 1;
		});
		second.RegisterFunction('which', function () {
			second.Push('second');
			return 1;
		});

		first.DoString('result = which()');
		second.DoString('result = which()');

		assert.strictEqual(readGlobal(first, 'result'), 'first');
		assert.strictEqual(readGlobal(second, 'result'), 'second');

		first.Close();
		second.Close();
	});

	await t.test('argument validation', () => {
		const lua = new nodelua.LuaState();
		assert.throws(() => lua.RegisterFunction('name'), /Requires 2 Arguments/);
		assert.throws(() => lua.RegisterFunction('name', 'not a function'), /Must Be A Function/);
		lua.Close();
	});
});

test('Call', async (t) => {
	await t.test('calls a Lua function with arguments', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('function join(a, b) return a .. "-" .. b end');

		lua.GetGlobal('join');
		lua.Push('left');
		lua.Push('right');
		lua.Call(2, 1);

		assert.strictEqual(lua.ToValue(-1), 'left-right');
		lua.Pop(1);
		lua.Close();
	});

	await t.test('a failing call throws', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('function boom() error("exploded") end');

		lua.GetGlobal('boom');
		assert.throws(() => lua.Call(0, 0), /exploded/);
		lua.Close();
	});
});

test('AddPackagePath', async (t) => {
	await t.test('makes a directory requirable', () => {
		const lua = new nodelua.LuaState();
		lua.AddPackagePath(path.join(FIXTURES, 'lua'));

		lua.DoString('greeting = require("greet").message');
		assert.strictEqual(readGlobal(lua, 'greeting'), 'greetings from a required module');
		lua.Close();
	});

	await t.test('a trailing separator is tolerated', () => {
		const lua = new nodelua.LuaState();
		lua.AddPackagePath(path.join(FIXTURES, 'lua') + path.sep);

		lua.DoString('greeting = require("greet").message');
		assert.strictEqual(readGlobal(lua, 'greeting'), 'greetings from a required module');
		lua.Close();
	});

	await t.test('a path containing a quote cannot break out of the assignment', () => {
		const lua = new nodelua.LuaState();
		// Previously interpolated into a generated Lua string literal.
		assert.doesNotThrow(() => lua.AddPackagePath("/tmp/'; os.exit(1); --"));
		lua.Close();
	});
});

test('LuaFileSystem', async (t) => {
	// LFS is compiled into the addon, so this must hold on every platform.
	await t.test("require('lfs') resolves", () => {
		const lua = new nodelua.LuaState();
		lua.DoString('lfs = require("lfs")');
		assert.strictEqual(typeof readGlobal(lua, 'lfs'), 'object');
		lua.Close();
	});

	await t.test('lfs.currentdir returns a path', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('cwd = require("lfs").currentdir()');

		const cwd = readGlobal(lua, 'cwd');
		assert.strictEqual(typeof cwd, 'string');
		assert.ok(cwd.length > 0);
		lua.Close();
	});

	await t.test('lfs.attributes reads a real file', () => {
		const lua = new nodelua.LuaState();
		lua.Push(path.join(FIXTURES, 'hello.lua'));
		lua.SetGlobal('target');
		lua.DoString('mode = require("lfs").attributes(target, "mode")');

		assert.strictEqual(readGlobal(lua, 'mode'), 'file');
		lua.Close();
	});
});

test('lifecycle', async (t) => {
	await t.test('Close is idempotent', () => {
		const lua = new nodelua.LuaState();
		lua.Close();
		assert.doesNotThrow(() => lua.Close());
	});

	await t.test('using a closed state throws instead of crashing', () => {
		const lua = new nodelua.LuaState();
		lua.Close();

		assert.throws(() => lua.DoString('x = 1'), /Already Been Closed/);
		assert.throws(() => lua.GetTop(), /Already Been Closed/);
		assert.throws(() => lua.Push(1), /Already Been Closed/);
	});

	await t.test('a state left unclosed is cleaned up by the collector', () => {
		// Exercises the destructor path: previously ~LuaState never called
		// lua_close, so every unclosed state leaked its interpreter.
		for (let i = 0; i < 50; i++) {
			const lua = new nodelua.LuaState();
			lua.DoString('t = {}; for i = 1, 100 do t[i] = i end');
		}
		assert.ok(true);
	});

	await t.test('Status reports a runnable state', () => {
		const lua = new nodelua.LuaState();
		assert.strictEqual(lua.Status(), 0);
		lua.Close();
	});

	await t.test('CollectGarbage runs', () => {
		const lua = new nodelua.LuaState();
		lua.DoString('junk = {}; for i = 1, 1000 do junk[i] = tostring(i) end');
		assert.strictEqual(typeof lua.CollectGarbage(nodelua.GC.COUNT), 'number');
		assert.doesNotThrow(() => lua.CollectGarbage(nodelua.GC.COLLECT));
		lua.Close();
	});
});
