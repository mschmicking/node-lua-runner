#include <napi.h>

#include "luastate.h"

extern "C" {
	#include <lua.h>
}

static void init_info_constants(Napi::Env env, Napi::Object target){
	Napi::Object constants = Napi::Object::New(env);
	constants.Set("VERSION", Napi::String::New(env, LUA_VERSION));
	constants.Set("VERSION_NUM", Napi::Number::New(env, LUA_VERSION_NUM));
	constants.Set("COPYRIGHT", Napi::String::New(env, LUA_COPYRIGHT));
	constants.Set("AUTHORS", Napi::String::New(env, LUA_AUTHORS));
	target.Set("INFO", constants);
}

static void init_status_constants(Napi::Env env, Napi::Object target){
	Napi::Object constants = Napi::Object::New(env);
	constants.Set("YIELD", Napi::Number::New(env, LUA_YIELD));
	constants.Set("ERRRUN", Napi::Number::New(env, LUA_ERRRUN));
	constants.Set("ERRSYNTAX", Napi::Number::New(env, LUA_ERRSYNTAX));
	constants.Set("ERRMEM", Napi::Number::New(env, LUA_ERRMEM));
	constants.Set("ERRERR", Napi::Number::New(env, LUA_ERRERR));
	target.Set("STATUS", constants);
}

static void init_gc_constants(Napi::Env env, Napi::Object target){
	Napi::Object constants = Napi::Object::New(env);
	constants.Set("STOP", Napi::Number::New(env, LUA_GCSTOP));
	constants.Set("RESTART", Napi::Number::New(env, LUA_GCRESTART));
	constants.Set("COLLECT", Napi::Number::New(env, LUA_GCCOLLECT));
	constants.Set("COUNT", Napi::Number::New(env, LUA_GCCOUNT));
	constants.Set("COUNTB", Napi::Number::New(env, LUA_GCCOUNTB));
	constants.Set("STEP", Napi::Number::New(env, LUA_GCSTEP));
	constants.Set("SETPAUSE", Napi::Number::New(env, LUA_GCSETPAUSE));
	constants.Set("SETSTEPMUL", Napi::Number::New(env, LUA_GCSETSTEPMUL));
	target.Set("GC", constants);
}

static void init_lua_constants(Napi::Env env, Napi::Object target){
	Napi::Object constants = Napi::Object::New(env);
	constants.Set("GLOBALSINDEX", Napi::Number::New(env, LUA_GLOBALSINDEX));
	constants.Set("REGISTRYINDEX", Napi::Number::New(env, LUA_REGISTRYINDEX));
	target.Set("LUA", constants);
}

static Napi::Object Init(Napi::Env env, Napi::Object exports) {
	LuaState::Init(env, exports);
	init_gc_constants(env, exports);
	init_status_constants(env, exports);
	init_info_constants(env, exports);
	init_lua_constants(env, exports);
	return exports;
}

NODE_API_MODULE(nodelua, Init)
