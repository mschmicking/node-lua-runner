#ifndef UTILS_H
#define UTILS_H

#include <string>

#include <napi.h>

extern "C" {
	#include <lua.h>
}

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

// Builds "<prefix><lua error message>" from the error at the top of the stack.
std::string lua_error_message(lua_State* L, const std::string& prefix);

#endif
