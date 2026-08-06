#include <string>

#include "luastate.h"

// Compiled into this addon from vendor/lfs/lfs.c. Declared here rather than by
// including lfs.h, which macro-redefines chdir/getcwd/rmdir on Windows.
extern "C" int luaopen_lfs(lua_State* L);

// Lua 5.1 has no luaL_requiref, so make require('lfs') resolvable by putting the
// opener into package.preload ourselves.
static void preload_lfs(lua_State* L) {
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_pushcfunction(L, luaopen_lfs);
	lua_setfield(L, -2, "lfs");
	lua_pop(L, 2);
}

Napi::Object LuaState::Init(Napi::Env env, Napi::Object exports) {
	Napi::Function func = DefineClass(env, "LuaState", {
		InstanceMethod("LoadFile", &LuaState::LoadFile),
		InstanceMethod("LoadString", &LuaState::LoadString),

		InstanceMethod("AddPackagePath", &LuaState::AddPackagePath),

		InstanceMethod("DoFile", &LuaState::DoFile),
		InstanceMethod("DoString", &LuaState::DoString),

		InstanceMethod("Status", &LuaState::Status),
		InstanceMethod("CollectGarbage", &LuaState::CollectGarbage),

		InstanceMethod("SetGlobal", &LuaState::SetGlobal),
		InstanceMethod("GetGlobal", &LuaState::GetGlobal),

		InstanceMethod("SetField", &LuaState::SetField),
		InstanceMethod("GetField", &LuaState::GetField),

		InstanceMethod("ToValue", &LuaState::ToValue),
		InstanceMethod("Call", &LuaState::Call),

		InstanceMethod("Yield", &LuaState::Yield),
		InstanceMethod("Resume", &LuaState::Resume),

		InstanceMethod("Close", &LuaState::Close),

		InstanceMethod("RegisterFunction", &LuaState::RegisterFunction),

		InstanceMethod("Push", &LuaState::Push),
		InstanceMethod("Pop", &LuaState::Pop),
		InstanceMethod("GetTop", &LuaState::GetTop),
		InstanceMethod("SetTop", &LuaState::SetTop),
		InstanceMethod("Replace", &LuaState::Replace)
	});

	exports.Set("LuaState", func);
	return exports;
}

LuaState::LuaState(const Napi::CallbackInfo& info)
	: Napi::ObjectWrap<LuaState>(info), lua_(NULL), closed_(false) {
	lua_ = luaL_newstate();
	if(lua_ == NULL){
		closed_ = true;
		Napi::Error::New(info.Env(), "LuaState: Could Not Allocate A Lua State").ThrowAsJavaScriptException();
		return;
	}
	luaL_openlibs(lua_);
	preload_lfs(lua_);
}

LuaState::~LuaState() {
	if(!closed_ && lua_ != NULL){
		lua_close(lua_);
	}
	lua_ = NULL;
	closed_ = true;
}

bool LuaState::EnsureOpen(Napi::Env env) {
	if(closed_ || lua_ == NULL){
		Napi::Error::New(env, "LuaState Has Already Been Closed").ThrowAsJavaScriptException();
		return false;
	}
	return true;
}

int LuaState::CallFunction(lua_State* L){
	const char* func_name = lua_tostring(L, lua_upvalueindex(1));
	LuaState* self = static_cast<LuaState*>(lua_touserdata(L, lua_upvalueindex(2)));

	if(self == NULL || func_name == NULL){
		return 0;
	}

	std::map<std::string, Napi::FunctionReference>::iterator iter = self->functions.find(func_name);
	if(iter == self->functions.end()){
		return 0;
	}

	// Lua may be running us on a coroutine thread rather than the main state.
	// Point lua_ at it for the duration so stack operations issued by the
	// JavaScript callback act on the stack it was actually called with.
	lua_State* previous = self->lua_;
	self->lua_ = L;

	Napi::Env env = iter->second.Env();
	Napi::HandleScope scope(env);

	Napi::Value ret_val = iter->second.Call({});

	self->lua_ = previous;

	// A JavaScript exception cannot be thrown through Lua's C frames, so leave it
	// pending; it surfaces once control returns to JavaScript.
	if(env.IsExceptionPending()){
		return 0;
	}

	if(ret_val.IsNumber()){
		return ret_val.As<Napi::Number>().Int32Value();
	}
	return 0;
}

Napi::Value LuaState::RegisterFunction(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();

	if(info.Length() < 2){
		Napi::TypeError::New(env, "LuaState.RegisterFunction Requires 2 Arguments").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsString()){
		Napi::TypeError::New(env, "LuaState.RegisterFunction Argument 1 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[1].IsFunction()){
		Napi::TypeError::New(env, "LuaState.RegisterFunction Argument 2 Must Be A Function").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	std::string func_name = info[0].As<Napi::String>().Utf8Value();
	functions[func_name] = Napi::Persistent(info[1].As<Napi::Function>());

	// Upvalue 1 is the name we look the callback up by, upvalue 2 is the owning
	// LuaState. Carrying the receiver on the closure keeps separate LuaState
	// instances from clashing.
	lua_pushstring(lua_, func_name.c_str());
	lua_pushlightuserdata(lua_, this);
	lua_pushcclosure(lua_, CallFunction, 2);
	lua_setglobal(lua_, func_name.c_str());

	return env.Undefined();
}

Napi::Value LuaState::AddPackagePath(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.AddPackagePath Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsString()){
		Napi::TypeError::New(env, "LuaState.AddPackagePath Argument 1 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	std::string path = info[0].As<Napi::String>().Utf8Value();
	for(size_t i = 0; i < path.size(); ++i){
		if(path[i] == '\\'){
			path[i] = '/';
		}
	}
	while(!path.empty() && path[path.size() - 1] == '/'){
		path.erase(path.size() - 1);
	}

	// Edit package.path through the C API rather than by running generated Lua:
	// a path containing a quote would otherwise break out of the string literal.
	lua_getglobal(lua_, "package");
	lua_getfield(lua_, -1, "path");

	const char* current = lua_tostring(lua_, -1);
	std::string package_path = current ? current : "";
	lua_pop(lua_, 1);

	if(!package_path.empty() && package_path[package_path.size() - 1] != ';'){
		package_path += ";";
	}
	package_path += path + "/?.lua";

	lua_pushlstring(lua_, package_path.c_str(), package_path.size());
	lua_setfield(lua_, -2, "path");
	lua_pop(lua_, 1);

	return env.Undefined();
}

Napi::Value LuaState::LoadFile(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.LoadFile Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsString()){
		Napi::TypeError::New(env, "LuaState.LoadFile Argument 1 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	std::string file_name = info[0].As<Napi::String>().Utf8Value();

	if(luaL_loadfile(lua_, file_name.c_str())){
		std::string message = lua_error_message(lua_, "LuaState.LoadFile: Parsing Of File " + file_name + " Has Failed:\n");
		lua_pop(lua_, 1);
		Napi::Error::New(env, message).ThrowAsJavaScriptException();
		return env.Undefined();
	}

	return env.Undefined();
}

Napi::Value LuaState::LoadString(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.LoadString Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsString()){
		Napi::TypeError::New(env, "LuaState.LoadString Argument 1 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	std::string lua_code = info[0].As<Napi::String>().Utf8Value();

	if(luaL_loadstring(lua_, lua_code.c_str())){
		std::string message = lua_error_message(lua_, "LuaState.LoadString: Parsing Of Lua Code Has Failed:\n");
		lua_pop(lua_, 1);
		Napi::Error::New(env, message).ThrowAsJavaScriptException();
		return env.Undefined();
	}

	return env.Undefined();
}

Napi::Value LuaState::DoFile(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.DoFile Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsString()){
		Napi::TypeError::New(env, "LuaState.DoFile Argument 1 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	std::string file_name = info[0].As<Napi::String>().Utf8Value();

	if(luaL_dofile(lua_, file_name.c_str())){
		std::string message = lua_error_message(lua_, "LuaState.DoFile: Execution Of File " + file_name + " Has Failed:\n");
		lua_pop(lua_, 1);
		Napi::Error::New(env, message).ThrowAsJavaScriptException();
		return env.Undefined();
	}

	return env.Undefined();
}

Napi::Value LuaState::DoString(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.DoString Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsString()){
		Napi::TypeError::New(env, "LuaState.DoString Argument 1 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	std::string lua_code = info[0].As<Napi::String>().Utf8Value();

	if(luaL_dostring(lua_, lua_code.c_str())){
		std::string message = lua_error_message(lua_, "LuaState.DoString: Execution Of Lua Code Has Failed:\n");
		lua_pop(lua_, 1);
		Napi::Error::New(env, message).ThrowAsJavaScriptException();
		return env.Undefined();
	}

	return env.Undefined();
}

Napi::Value LuaState::SetGlobal(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.SetGlobal Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsString()){
		Napi::TypeError::New(env, "LuaState.SetGlobal Argument 1 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	std::string global_name = info[0].As<Napi::String>().Utf8Value();
	lua_setglobal(lua_, global_name.c_str());

	return env.Undefined();
}

Napi::Value LuaState::GetGlobal(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.GetGlobal Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsString()){
		Napi::TypeError::New(env, "LuaState.GetGlobal Argument 1 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	std::string global_name = info[0].As<Napi::String>().Utf8Value();
	lua_getglobal(lua_, global_name.c_str());

	return env.Undefined();
}

Napi::Value LuaState::SetField(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 3){
		Napi::TypeError::New(env, "LuaState.SetField Requires 3 Arguments").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsNumber()){
		Napi::TypeError::New(env, "LuaState.SetField Argument 1 Must Be A Number").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[1].IsString()){
		Napi::TypeError::New(env, "LuaState.SetField Argument 2 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	// Resolve the index before pushing: pushing the value shifts every relative
	// index by one, which would otherwise leave us assigning into the value itself.
	int index = abs_index(lua_, info[0].As<Napi::Number>().Int32Value());
	std::string field_name = info[1].As<Napi::String>().Utf8Value();

	// Indexing a non-table raises an unprotected Lua error, which aborts the whole
	// process rather than throwing. Reject it here instead.
	if(!lua_istable(lua_, index) && lua_type(lua_, index) != LUA_TUSERDATA){
		Napi::TypeError::New(env, "LuaState.SetField: Value At The Given Index Is Not A Table").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	// Push the value, not the key: lua_setfield takes the key as a C string and
	// pops the value from the top of the stack.
	push_value_to_lua(lua_, info[2]);
	lua_setfield(lua_, index, field_name.c_str());

	return env.Undefined();
}

Napi::Value LuaState::GetField(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 2){
		Napi::TypeError::New(env, "LuaState.GetField Requires 2 Arguments").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsNumber()){
		Napi::TypeError::New(env, "LuaState.GetField Argument 1 Must Be A Number").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[1].IsString()){
		Napi::TypeError::New(env, "LuaState.GetField Argument 2 Must Be A String").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	int index = abs_index(lua_, info[0].As<Napi::Number>().Int32Value());
	std::string field_name = info[1].As<Napi::String>().Utf8Value();

	// Indexing a non-table raises an unprotected Lua error, which aborts the whole
	// process rather than throwing. Reject it here instead.
	if(!lua_istable(lua_, index) && lua_type(lua_, index) != LUA_TUSERDATA){
		Napi::TypeError::New(env, "LuaState.GetField: Value At The Given Index Is Not A Table").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	lua_getfield(lua_, index, field_name.c_str());

	return env.Undefined();
}

Napi::Value LuaState::ToValue(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.ToValue Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsNumber()){
		Napi::TypeError::New(env, "LuaState.ToValue Argument 1 Must Be A Number").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	int index = info[0].As<Napi::Number>().Int32Value();
	return lua_to_value(env, lua_, index);
}

Napi::Value LuaState::Call(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 2){
		Napi::TypeError::New(env, "LuaState.Call Requires 2 Arguments").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsNumber()){
		Napi::TypeError::New(env, "LuaState.Call Argument 1 Must Be A Number").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[1].IsNumber()){
		Napi::TypeError::New(env, "LuaState.Call Argument 2 Must Be A Number").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	int args = info[0].As<Napi::Number>().Int32Value();
	int results = info[1].As<Napi::Number>().Int32Value();

	if(lua_pcall(lua_, args, results, 0)){
		std::string message = lua_error_message(lua_, "LuaState.Call: Execution Of Lua Function Has Failed:\n");
		lua_pop(lua_, 1);
		Napi::Error::New(env, message).ThrowAsJavaScriptException();
		return env.Undefined();
	}

	return env.Undefined();
}

Napi::Value LuaState::Yield(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.Yield Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsNumber()){
		Napi::TypeError::New(env, "LuaState.Yield Argument 1 Must Be A Number").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	int args = info[0].As<Napi::Number>().Int32Value();
	lua_yield(lua_, args);

	return env.Undefined();
}

Napi::Value LuaState::Resume(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.Resume Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsNumber()){
		Napi::TypeError::New(env, "LuaState.Resume Argument 1 Must Be A Number").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	int args = info[0].As<Napi::Number>().Int32Value();
	int status = lua_resume(lua_, args);

	return Napi::Number::New(env, status);
}

Napi::Value LuaState::Close(const Napi::CallbackInfo& info) {
	if(!closed_ && lua_ != NULL){
		lua_close(lua_);
		lua_ = NULL;
		closed_ = true;
		functions.clear();
	}
	return info.Env().Undefined();
}

Napi::Value LuaState::Status(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	return Napi::Number::New(env, lua_status(lua_));
}

Napi::Value LuaState::CollectGarbage(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.CollectGarbage Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsNumber()){
		Napi::TypeError::New(env, "LuaState.CollectGarbage Argument 1 Must Be A Number, try nodelua.GC.[TYPE]").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	int type = info[0].As<Napi::Number>().Int32Value();
	return Napi::Number::New(env, lua_gc(lua_, type, 0));
}

Napi::Value LuaState::Push(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.Push Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	push_value_to_lua(lua_, info[0]);
	return env.Undefined();
}

Napi::Value LuaState::Pop(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	int pop_n = 1;
	if(info.Length() > 0 && info[0].IsNumber()){
		pop_n = info[0].As<Napi::Number>().Int32Value();
	}

	lua_pop(lua_, pop_n);
	return env.Undefined();
}

Napi::Value LuaState::GetTop(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	return Napi::Number::New(env, lua_gettop(lua_));
}

Napi::Value LuaState::SetTop(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	int set_n = 0;
	if(info.Length() > 0 && info[0].IsNumber()){
		set_n = info[0].As<Napi::Number>().Int32Value();
	}

	lua_settop(lua_, set_n);
	return env.Undefined();
}

Napi::Value LuaState::Replace(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	if(info.Length() < 1){
		Napi::TypeError::New(env, "LuaState.Replace Requires 1 Argument").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!info[0].IsNumber()){
		Napi::TypeError::New(env, "LuaState.Replace Argument 1 Must Be A Number").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if(!EnsureOpen(env)){
		return env.Undefined();
	}

	int index = info[0].As<Napi::Number>().Int32Value();
	lua_replace(lua_, index);

	return env.Undefined();
}
