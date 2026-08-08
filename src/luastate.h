#ifndef LUASTATE_H
#define LUASTATE_H

#include <map>
#include <string>

#include <napi.h>

#include "utils.h"

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

class LuaState : public Napi::ObjectWrap<LuaState> {
public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);

	LuaState(const Napi::CallbackInfo& info);
	~LuaState();

	// Trampoline invoked by Lua for every function registered from JavaScript.
	static int CallFunction(lua_State* L);

private:
	// Functions registered from JS, keyed by their Lua global name.
	std::map<std::string, Napi::FunctionReference> functions;

	lua_State* lua_;
	bool closed_;

	// Throws and returns false if the state has already been closed, so that
	// calling into a closed LuaState raises instead of dereferencing freed memory.
	bool EnsureOpen(Napi::Env env);

	// Throws and returns false if the value at `index` cannot be indexed, which
	// would otherwise raise an unprotected Lua error and abort the process.
	bool EnsureIndexable(Napi::Env env, int index, const char* method);

	Napi::Value Close(const Napi::CallbackInfo& info);

	Napi::Value CollectGarbage(const Napi::CallbackInfo& info);
	Napi::Value Status(const Napi::CallbackInfo& info);

	Napi::Value AddPackagePath(const Napi::CallbackInfo& info);

	Napi::Value LoadFile(const Napi::CallbackInfo& info);
	Napi::Value LoadString(const Napi::CallbackInfo& info);

	Napi::Value DoFile(const Napi::CallbackInfo& info);
	Napi::Value DoString(const Napi::CallbackInfo& info);

	Napi::Value SetGlobal(const Napi::CallbackInfo& info);
	Napi::Value GetGlobal(const Napi::CallbackInfo& info);

	Napi::Value SetField(const Napi::CallbackInfo& info);
	Napi::Value GetField(const Napi::CallbackInfo& info);

	Napi::Value ToValue(const Napi::CallbackInfo& info);
	Napi::Value Call(const Napi::CallbackInfo& info);

	Napi::Value Yield(const Napi::CallbackInfo& info);
	Napi::Value Resume(const Napi::CallbackInfo& info);

	Napi::Value RegisterFunction(const Napi::CallbackInfo& info);

	Napi::Value Push(const Napi::CallbackInfo& info);
	Napi::Value Pop(const Napi::CallbackInfo& info);
	Napi::Value GetTop(const Napi::CallbackInfo& info);
	Napi::Value SetTop(const Napi::CallbackInfo& info);
	Napi::Value Replace(const Napi::CallbackInfo& info);
};

#endif
