#include "com_jvmti_utils_ClassUtils.h"
#include "jvmti.h"
#include <memory.h>
//#include <iostream>


using namespace std;


static JavaVM* javavm = nullptr;
static jvmtiEnv* jvmti = NULL;

void initialize(JavaVM* vm, JNIEnv* env);

static jobject mainClassloader = NULL;
static jclass classClassloader = NULL;

static jobject classUtil = NULL;
static jclass classClassUtil = NULL;


static jmethodID methodID = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    //cout << "[JVMTI] jni loaded" << endl;
    javavm = vm;
    return JNI_VERSION_1_8;
}

jbyteArray asByteArray(JNIEnv* env, const unsigned char* buf, int len) {
    jbyteArray array = env->NewByteArray(len);
    env->SetByteArrayRegion(array, 0, len, (const jbyte*)buf);
    return array;
}

unsigned char* asUnsignedCharArray(JNIEnv* env, jbyteArray array) {
    int len = env->GetArrayLength(array);
    unsigned char* buf = new unsigned char[len];
    env->GetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte*>(buf));
    return buf;
}


void JNICALL classFileLoadHook(jvmtiEnv *jvmti,
    JNIEnv* env,
    jclass class_being_redefined,
    jobject loader,
    const char* name,
    jobject protection_domain,
    jint data_len,
    const unsigned char* data,
    jint* new_data_len,
    unsigned char** new_data) {

    if (name == NULL)
    {
        return;
    }

    //printf("loaded -> %s \n ",name);

    jvmti->Allocate(data_len, new_data);
    *new_data_len = data_len;
    memcpy(*new_data, data, data_len);

    const jbyteArray plainBytes = asByteArray(env, *new_data, *new_data_len);
    env->CallStaticVoidMethod(classClassUtil, methodID, env->NewStringUTF(name), plainBytes);

    //unsigned char* newChars = asUnsignedCharArray(env, newByteArray);
    //const jint newLength = (jint)env->GetArrayLength(newByteArray);

    //jvmti->Allocate(newLength, new_data);
    //*new_data_len = newLength;
    //memcpy(*new_data, newChars, newLength);

}


JNIEXPORT void JNICALL Java_com_jvmti_utils_ClassUtils_retransformClass
(JNIEnv* env, jclass caller, jclass target) {
    jvmti->RetransformClasses(1, &target);
}


JNIEXPORT void JNICALL Java_com_jvmti_utils_ClassUtils_initialize
(JNIEnv* env, jclass caller, jobject classloader, jobject object) {
    mainClassloader = classloader;
    classClassloader = env->GetObjectClass(mainClassloader);

    classClassUtil = env->GetObjectClass(object);
    classUtil = object;

    methodID = (jmethodID)env->GetStaticMethodID(classClassUtil, "transformHook", "(Ljava/lang/String;[B)V");
    

    initialize(javavm, env);
}

JNIEXPORT void JNICALL Java_com_jvmti_utils_ClassUtils_redefineClass
(JNIEnv* env, jclass caller, jclass target, jbyteArray classBytes) {

    jvmtiClassDefinition classDef; //结构体

    classDef.klass = target;
    classDef.class_byte_count = env->GetArrayLength(classBytes);
    classDef.class_bytes = (unsigned char*)env->GetByteArrayElements(classBytes, NULL);

    jvmti->RedefineClasses(1, &classDef);

}


void initialize(JavaVM* vm, JNIEnv* env)
{
    vm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_1); //获取JVMTI环境

    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));

    

    caps.can_generate_all_class_hook_events = 1;
    caps.can_retransform_any_class = 1;
    caps.can_retransform_classes = 1;
    caps.can_redefine_any_class = 1;
    caps.can_redefine_classes = 1;
    jvmti->AddCapabilities(&caps);//设置参数

    

    jvmtiEventCallbacks callbacks = {};
    callbacks.ClassFileLoadHook = classFileLoadHook;
    jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));


    //启动jvmti事件
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);

}
