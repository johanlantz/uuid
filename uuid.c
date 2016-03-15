#include <string.h>
#define MAX_UUID_BUF 37

//For Android there is no native uuid support. We have to use jni to call the Java version
#if defined(__ANDROID__)
#include <stdlib.h>
#include <stdint.h>
#include <jni.h>

extern JavaVM *pj_jni_jvm;

static pj_bool_t attach_jvm(JNIEnv **jni_env)
{
    if ((*pj_jni_jvm)->GetEnv(pj_jni_jvm, (void **)jni_env,
                              JNI_VERSION_1_4) < 0) {
        if ((*pj_jni_jvm)->AttachCurrentThread(pj_jni_jvm, jni_env, NULL) < 0) {
            jni_env = NULL;
            return PJ_FALSE;
        }
        return PJ_TRUE;
    }

    return PJ_FALSE;
}

#define detach_jvm(attached) \
if (attached) \
   (*pj_jni_jvm)->DetachCurrentThread(pj_jni_jvm);

int uuid_get_uuid(void* msg_buf, int buf_len)
{
    jclass uuid_class;
    jmethodID get_uuid_method;
    jmethodID to_string_method;
    JNIEnv *jni_env = 0;
    jobject javaUuid;
    jstring uuid_string;
    int error_code = 0;
    const char *native_string;

    if (buf_len < MAX_UUID_BUF) {
        error_code = 8;
        goto on_error;
    }

    pj_bool_t attached = attach_jvm(&jni_env);
    if (!jni_env) {
        error_code = 1;
        goto on_error;
    }

    uuid_class = (jclass)(*jni_env)->NewGlobalRef(jni_env,
                 (*jni_env)->FindClass(jni_env, "java/util/UUID"));

    if (uuid_class == 0) {
        error_code = 2;
        goto on_error;
    }

    get_uuid_method = (*jni_env)->GetStaticMethodID(jni_env, uuid_class,
                      "randomUUID",
                      "()Ljava/util/UUID;");

    if (get_uuid_method == 0) {
        error_code = 3;
        goto on_error;
    }

    javaUuid = (*jni_env)->CallStaticObjectMethod(jni_env, uuid_class, get_uuid_method);
    if (javaUuid == 0) {
        error_code = 4;
        goto on_error;
    }

    to_string_method = (*jni_env)->GetMethodID(jni_env, uuid_class, "toString", "()Ljava/lang/String;");

    if (to_string_method == 0) {
        error_code = 5;
        goto on_error;
    }

    uuid_string = (*jni_env)->CallObjectMethod(jni_env, javaUuid, to_string_method);
    if (uuid_string == 0) {
        error_code = 6;
        goto on_error;
    }

    native_string = (*jni_env)->GetStringUTFChars(jni_env, uuid_string, JNI_FALSE);
    if (native_string == 0) {
        error_code = 7;
        goto on_error;
    }

    strcpy(msg_buf, native_string);

    (*jni_env)->ReleaseStringUTFChars(jni_env, uuid_string, native_string);
    detach_jvm(attached);

    return strlen(msg_buf);

on_error:
    snprintf(msg_buf, buf_len, "-ERR%d-", error_code);
    detach_jvm(attached);
    return strlen(msg_buf);
}
#elif defined(__unix) || defined(__linux) || defined(__APPLE__)
#include <uuid/uuid.h>
int uuid_get_uuid(void* msg_buf, int buf_len)
{
    uuid_t uuid_generated;
    if (buf_len < MAX_UUID_BUF) {
        return 0;
    }
    uuid_generate(uuid_generated);
    uuid_unparse(uuid_generated, msg_buf);
    return (int)strlen(msg_buf);
}
#elif defined(_WIN32)
#if defined(_WIN32_WINPHONE)
#include<guiddef.h>
#else
#include <stdio.h>
#include <objbase.h>
#endif
int uuid_get_uuid(void* msg_buf, int buf_len)
{
    GUID guid;
    CoCreateGuid(&guid);
    snprintf(msg_buf, buf_len, "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
             guid.Data1, guid.Data2, guid.Data3,
             guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return strlen(msg_buf);
}
#endif
