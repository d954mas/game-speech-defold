#include "speech_callback.h"
#include "djni.h"
#include <stdlib.h>

static Speech_Callback          m_callback;
static dmArray<CallbackData>    m_callbacksQueue;
static dmMutex::HMutex          m_mutex;

#if defined(DM_PLATFORM_ANDROID)

static void RegisterCallback(lua_State* L, int index){
    Speech_Callback *cbk = &m_callback;
    if(cbk->m_Callback != LUA_NOREF){
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
    }

    cbk->m_L = dmScript::GetMainThread(L);

    luaL_checktype(L, index, LUA_TFUNCTION);
    lua_pushvalue(L, index);
    cbk->m_Callback = dmScript::Ref(L, LUA_REGISTRYINDEX);

    dmScript::GetInstance(L);
    cbk->m_Self = dmScript::Ref(L, LUA_REGISTRYINDEX);
}

static void UnregisterCallback(){
    Speech_Callback *cbk = &m_callback;
    if(cbk->m_Callback != LUA_NOREF)
   {
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
        cbk->m_Callback = LUA_NOREF;
    }
}

static void invoke_callback(MESSAGE_ID type, char*json){
    Speech_Callback *cbk = &m_callback;
    if(cbk->m_Callback == LUA_NOREF){
        dmLogInfo("Speech callback do not exist.");
        return;
    }

    lua_State* L = cbk->m_L;
    int top = lua_gettop(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbk->m_Callback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbk->m_Self);
    lua_pushvalue(L, -1);
    dmScript::SetInstance(L);

    if (!dmScript::IsInstanceValid(L)){
        UnregisterCallback();
        dmLogError("Could not run Speech callback because the instance has been deleted.");
        lua_pop(L, 2);
    }
    else {
        lua_pushnumber(L, type);
        int count_table_elements = 1;
        bool is_fail = false;
        dmJson::Document doc;
        dmJson::Result r = dmJson::Parse(json, &doc);
        if (r == dmJson::RESULT_OK && doc.m_NodeCount > 0) {
            char error_str_out[128];
            if (dmScript::JsonToLua(L, &doc, 0, error_str_out, sizeof(error_str_out)) < 0) {
                dmLogError("Failed converting object JSON to Lua; %s", error_str_out);
                is_fail = true;
            }
        } else {
            dmLogError("Failed to parse JSON object(%d): (%s)", r, json);
            is_fail = true;
        }
        dmJson::Free(&doc);
        if (is_fail) {
            lua_pop(L, 2);
            assert(top == lua_gettop(L));
            return;
        }

        int number_of_arguments = 3;
        int ret = lua_pcall(L, number_of_arguments, 0, 0);
        if(ret != 0)
        {
            dmLogError("Error running callback: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    assert(top == lua_gettop(L));
}

static void SpeechCallback_AddToQueueJNI(JNIEnv * env, jclass cls, jint jmsg, jstring jjson){
    const char* json = env->GetStringUTFChars(jjson, 0);
    SpeechCallback_AddToQueue((MESSAGE_ID)jmsg, json);
    env->ReleaseStringUTFChars(jjson, json);
}

static void SpeechCallback_RegisterNatives(JNIEnv* env) {
    JNINativeMethod nativeMethods[] = {
        {"speechCallbackAddToQueue", "(ILjava/lang/String;)V", (void*)&SpeechCallback_AddToQueueJNI}
    };

	jclass jclass_SpeechRecognition = djni::GetClass(env, "com.d954mas.defold.speech.recognition.SpeechRecognitionManager");
    env->RegisterNatives( jclass_SpeechRecognition , nativeMethods, sizeof(nativeMethods) / sizeof(nativeMethods[0]));
    env->DeleteLocalRef( jclass_SpeechRecognition );
}

void SpeechCallback_Initialize(){
    m_mutex = dmMutex::New();
    JNIEnv* env = djni::env();
    SpeechCallback_RegisterNatives(env);
}

void SpeechCallback_Finalize(){
    dmMutex::Delete(m_mutex);
    UnregisterCallback();
}

void SpeechCallback_Set(lua_State* L, int pos){
    int type = lua_type(L, pos);
    if (type == LUA_TNONE || type == LUA_TNIL){
        UnregisterCallback();
    }
    else{
        RegisterCallback(L, pos);
    }
}

void SpeechCallback_AddToQueue(MESSAGE_ID msg, const char*json){
    DM_MUTEX_SCOPED_LOCK(m_mutex);

    CallbackData data;
    data.msg = msg;
    data.json = json ? strdup(json) : NULL;

    if(m_callbacksQueue.Full()) {
        m_callbacksQueue.OffsetCapacity(1);
    }
    m_callbacksQueue.Push(data);
}

void SpeechCallback_Update(){
    if (m_callbacksQueue.Empty()) {return;}

    DM_MUTEX_SCOPED_LOCK(m_mutex);
    
    for(uint32_t i = 0; i != m_callbacksQueue.Size(); ++i){
        CallbackData* data = &m_callbacksQueue[i];
        invoke_callback(data->msg, data->json);
        if(data->json) {
            free(data->json);
            data->json = 0;
        }
        m_callbacksQueue.EraseSwap(i--);
    }
}

#endif