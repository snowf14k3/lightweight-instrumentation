#include "jvmti.h"
#include <memory.h>

static jvmtiEnv *jvmti = NULL;
static jclass classClassUtil = NULL;
static jmethodID methodID = NULL;

jbyteArray asByteArray(JNIEnv *env, const unsigned char *buf, int len)
{
    jbyteArray array = env->NewByteArray(len);
    env->SetByteArrayRegion(array, 0, len, (const jbyte *)buf);
    return array;
}

unsigned char *asUnsignedCharArray(JNIEnv *env, jbyteArray array)
{
    int len = env->GetArrayLength(array);
    unsigned char *buf = new unsigned char[len];
    env->GetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte *>(buf));
    return buf;
}

void JNICALL classFileLoadHook(jvmtiEnv *jvmti,
                               JNIEnv *env,
                               jclass class_being_redefined,
                               jobject loader,
                               const char *name,
                               jobject protection_domain,
                               jint data_len,
                               const unsigned char *data,
                               jint *new_data_len,
                               unsigned char **new_data)
{

    if (name == NULL)
    {
        return;
    }

    jvmti->Allocate(data_len, new_data);
    *new_data_len = data_len;
    memcpy(*new_data, data, data_len);

    const jbyteArray plainBytes = asByteArray(env, *new_data, *new_data_len);
    jbyteArray newByteArray = (jbyteArray)env->CallStaticObjectMethod(classClassUtil, methodID, env->NewStringUTF(name), plainBytes);

    unsigned char *newChars = asUnsignedCharArray(env, newByteArray);
    const jint newLength = (jint)env->GetArrayLength(newByteArray);

    jvmti->Allocate(newLength, new_data);
    *new_data_len = newLength;
    memcpy(*new_data, newChars, newLength);
}

JNIEXPORT void JNICALL retransformClass(JNIEnv *env, jclass caller, jclass target)
{
    jvmti->RetransformClasses(1, &target);
}

JNIEXPORT void JNICALL redefineClass(JNIEnv *env, jclass caller, jclass target, jbyteArray classBytes)
{

    jvmtiClassDefinition classDef;

    classDef.klass = target;
    classDef.class_byte_count = env->GetArrayLength(classBytes);
    classDef.class_bytes = (unsigned char *)env->GetByteArrayElements(classBytes, NULL);

    jvmti->RedefineClasses(1, &classDef);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env;

    vm->GetEnv((void **)&env, JNI_VERSION_1_8);
    vm->GetEnv((void **)&jvmti, JVMTI_VERSION_1_1);

    classClassUtil = (jclass)env->FindClass("com/utils/ClassUtils");

    JNINativeMethod table[] = {
        {"redefineClass", "(Ljava/lang/Class;[B)V,", redefineClass},
        {"retransformClass", "(Ljava/lang/Class;)V", retransformClass}};

    env->RegisterNatives(classClassUtil, table, 2);

    methodID = env->GetStaticMethodID(classClassUtil, "tansformer", "(Ljava/lang/String;[B)[B");

    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));

    caps.can_generate_all_class_hook_events = 1;
    caps.can_retransform_any_class = 1;
    caps.can_retransform_classes = 1;
    caps.can_redefine_any_class = 1;
    caps.can_redefine_classes = 1;
    jvmti->AddCapabilities(&caps);

    jvmtiEventCallbacks callbacks = {};
    callbacks.ClassFileLoadHook = &classFileLoadHook;
    jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));

    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);

    return JNI_VERSION_1_8;
}