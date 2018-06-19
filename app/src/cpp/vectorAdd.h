//
// Created by Khoa Tran on 5/23/18.
//
#include <jni.h>
#include <string>
#include <iostream>
#include <vector>
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/opencl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <chrono>// Replace <ctime>
#include <CL/err_code.h>

#ifndef MYFIRSTAPP_VECTORADD_H
#define MYFIRSTAPP_VECTORADD_H

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL Java_com_example_myfirstapp_MainActivity_vectorAdd(JNIEnv *, jobject, jint dataSize);

#ifdef __cplusplus
}
#endif

#endif //MYFIRSTAPP_VECTORADD_H
