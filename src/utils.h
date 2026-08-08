#ifndef UTILS_H
#define UTILS_H

#include <initializer_list>
#include <string>

#include <napi.h>

extern "C" {
	#include <lua.h>
}

// Kinds of JavaScript argument the binding methods accept. `Any` only asserts that
// the argument was supplied.
enum class Arg { Number, String, Function, Any };

// Checks arity and argument types for a LuaState method, throwing a TypeError in
// this project's established wording if either is wrong. `hint` is appended to a
// type error when a method wants to point at the constants to use.
//
// Returns false when it threw, so callers read as:
//   if(!CheckArgs(info, "SetField", {Arg::Number, Arg::String, Arg::Any})) return env.Undefined();
bool CheckArgs(const Napi::CallbackInfo& info, const char* method,
	std::initializer_list<Arg> expected, const char* hint = NULL);

// Throws the Lua error on top of the stack as a JavaScript Error prefixed with
// `prefix`, and pops it. Popping here rather than at each call site is the point:
// leaving the error behind silently grows the stack.
void ThrowLuaError(Napi::Env env, lua_State* L, const std::string& prefix);

// Resolves a relative stack index to an absolute one, leaving positive indices
// and pseudo-indices (LUA_GLOBALSINDEX and friends) untouched. Needed wherever we
// push onto the stack before consuming a caller-supplied index.
int abs_index(lua_State* L, int index);

// Converts the Lua value at `index` into its JavaScript equivalent. Tables become
// plain objects; types with no JS counterpart become undefined.
Napi::Value lua_to_value(Napi::Env env, lua_State* L, int index);

// Pushes the JavaScript value onto the Lua stack. Values with no Lua counterpart
// are pushed as nil.
void push_value_to_lua(lua_State* L, Napi::Value value);

#endif
