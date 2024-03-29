// myextension.cpp
// Extension lib defines
#define EXTENSION_NAME speech_recognition
#define LIB_NAME "speech_recognition"
#define MODULE_NAME "speech_recognition"
// include the Defold SDK
#include <dmsdk/sdk.h>
#include "speech_recognition.h"
#include "speech_callback.h"

#if defined(DM_PLATFORM_ANDROID)

static int Lua_SpeechRecognitionInit(lua_State* L) {
    SpeechRecognition_Init();
    return 0;
}
static int Lua_SpeechRecognitionStart(lua_State* L) {
    SpeechRecognition_Start();
    return 0;
}
static int Lua_SpeechRecognitionStop(lua_State* L) {
     SpeechRecognition_Stop();
     return 0;
 }
static int Lua_SpeechRecognitionSetContinuous(lua_State* L) {
    SpeechRecognition_SetContinuous(lua_toboolean(L, 1));
    return 0;
}

static int Lua_SpeechCallbackSet(lua_State* L) {
    SpeechCallback_Set(L,1);
    return 0;
}

static const luaL_reg Module_methods[] = {
    {"init", Lua_SpeechRecognitionInit},
    {"start", Lua_SpeechRecognitionStart},
    {"stop", Lua_SpeechRecognitionStop},
    {"set_continuous", Lua_SpeechRecognitionSetContinuous},
    {"set_callback", Lua_SpeechCallbackSet},
    {0, 0}
};


static void LuaInit(lua_State* L){
	int top = lua_gettop(L);
	// Register lua names
	luaL_register(L, MODULE_NAME, Module_methods);
	lua_pop(L, 1);
	assert(top == lua_gettop(L));
}


static dmExtension::Result InitializeMyExtension(dmExtension::Params* params) {
    SpeechRecognition_Initialize();
    SpeechCallback_Initialize();
    LuaInit(params->m_L);
    printf("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeMyExtension(dmExtension::Params* params) {
    SpeechRecognition_Finalize();
    SpeechCallback_Finalize();
    return dmExtension::RESULT_OK;
}

static dmExtension::Result UpdateMyExtension(dmExtension::Params* params){
    SpeechCallback_Update();
    return dmExtension::RESULT_OK;
}

#else

dmExtension::Result InitializeMyExtension(dmExtension::Params* params) { return dmExtension::RESULT_OK;}
dmExtension::Result FinalizeMyExtension(dmExtension::Params* params) { return dmExtension::RESULT_OK;}
dmExtension::Result UpdateMyExtension(dmExtension::Params* params) { return dmExtension::RESULT_OK;}
#endif

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, NULL, NULL, InitializeMyExtension, UpdateMyExtension, 0, FinalizeMyExtension)
