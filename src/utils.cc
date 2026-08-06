#include "utils.h"

int abs_index(lua_State* L, int index) {
	if(index > 0 || index <= LUA_REGISTRYINDEX){
		return index;
	}
	return lua_gettop(L) + index + 1;
}

Napi::Value lua_to_value(Napi::Env env, lua_State* L, int index) {
	switch(lua_type(L, index)){
	case LUA_TBOOLEAN:
		return Napi::Boolean::New(env, lua_toboolean(L, index) != 0);
	case LUA_TNUMBER:
		return Napi::Number::New(env, lua_tonumber(L, index));
	case LUA_TSTRING:
		return Napi::String::New(env, lua_tostring(L, index));
	case LUA_TTABLE:
		{
			// lua_next pushes a key and a value, so a relative index would drift
			// as we iterate. Resolve it to an absolute one up front.
			int table = abs_index(L, index);

			Napi::Object obj = Napi::Object::New(env);
			lua_pushnil(L);
			while(lua_next(L, table) != 0){
				Napi::Value key = lua_to_value(env, L, -2);
				Napi::Value value = lua_to_value(env, L, -1);
				obj.Set(key, value);
				lua_pop(L, 1);
			}
			return obj;
		}
	default:
		return env.Undefined();
	}
}

void push_value_to_lua(lua_State* L, Napi::Value value){
	if(value.IsString()){
		// Push with an explicit length: Lua strings may contain embedded NULs.
		std::string str = value.As<Napi::String>().Utf8Value();
		lua_pushlstring(L, str.c_str(), str.size());
	}else if(value.IsNumber()){
		lua_pushnumber(L, value.As<Napi::Number>().DoubleValue());
	}else if(value.IsBoolean()){
		lua_pushboolean(L, value.As<Napi::Boolean>().Value() ? 1 : 0);
	}else if(value.IsObject()){
		Napi::Object obj = value.As<Napi::Object>();

		Napi::Array keys = obj.GetPropertyNames();

		lua_newtable(L);
		for(uint32_t i = 0; i < keys.Length(); ++i){
			Napi::Value key = keys.Get(i);
			Napi::Value val = obj.Get(key);

			push_value_to_lua(L, key);
			push_value_to_lua(L, val);
			lua_settable(L, -3);
		}
	}else{
		lua_pushnil(L);
	}
}

std::string lua_error_message(lua_State* L, const std::string& prefix){
	const char* message = lua_tostring(L, -1);
	return prefix + (message ? message : "unknown error");
}
