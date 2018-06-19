////
//// Created by Khoa Tran on 5/21/18.
////
#include <jni.h>
#include <string>
#include <iostream>
#include <vector>
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/opencl.h>

#ifndef MYFIRSTAPP_NATIVE_LIB_H
#define MYFIRSTAPP_NATIVE_LIB_H
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL Java_com_example_myfirstapp_MainActivity_getInfo(JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif MYFIRSTAPP_NATIVE_LIB_H
