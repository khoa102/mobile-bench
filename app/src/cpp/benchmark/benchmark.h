//
// Created by Khoa Tran on 5/29/18.
//
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/opencl.h>
#include <CL/err_code.h>
#include <jni.h>
#include <string>
#include <benchmark/SHOC-level0/BusSpeedDownload.h>
#include <benchmark/SHOC-level0/BusSpeedReadback.h>
#include <benchmark/matrixMul/matrixMul.h>

#ifndef MYFIRSTAPP_BENCHMARK_H
#define MYFIRSTAPP_BENCHMARK_H

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL Java_com_example_myfirstapp_MainActivity_benchmark(JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif

#endif //MYFIRSTAPP_BENCHMARK_H
