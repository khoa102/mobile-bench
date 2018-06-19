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
#include "benchmark.h"

//float *srcA, *srcB, *dst;        // Host buffers for OpenCL test
//float *Golden;                   // Host buffer for host golden processing cross check

cl_context cxGPUContext;        // OpenCL context
cl_command_queue cqCommandQueue;// OpenCL command que
cl_platform_id cpPlatform;      // OpenCL platform
cl_device_id cdDevice;          // OpenCL device
//cl_program cpProgram;           // OpenCL program
//cl_kernel ckKernel;             // OpenCL kernel
//cl_mem cmDevSrcA;               // OpenCL device source buffer A
//cl_mem cmDevSrcB;               // OpenCL device source buffer B
//cl_mem cmDevDst;                // OpenCL device destination buffer
//size_t szGlobalWorkSize;        // 1D var for Total # of work items
//size_t szLocalWorkSize;		// 1D var for # of work items in the work group
//size_t szParmDataBytes;		// Byte size of context information
//size_t szKernelLength;		// Byte size of kernel code
cl_int ciErr1, ciErr2;		// Error code var
//char* cPathAndName = NULL;      // var for full paths to data, src, etc.
//char* cSourceCL = NULL;         // Buffer to hold source for compilation

//int iNumElements;        //Number of elements on host machine
std::string error_ret;          // Store the error message returned from check_error
void Cleanup (std::string& output, int iExitCode);


extern "C" JNIEXPORT jstring JNICALL Java_com_example_myfirstapp_MainActivity_benchmark(JNIEnv *env, jobject thisObj){
    std::string output = "Hello from C++!!!\n\n";
//=========================================================================================
    ciErr1 = clGetPlatformIDs(1, &cpPlatform, NULL);
    if (checkError(ciErr1, "Getting platform", error_ret)){
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += "*** Got platform\n";
//=========================================================================================
    ciErr1 = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &cdDevice, NULL);
    if (checkError(ciErr1, "Getting device", error_ret)) {
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += "*** Got device\n";
//=========================================================================================
    cxGPUContext = clCreateContext(0, 1, &cdDevice, NULL, NULL, &ciErr1);
    if (checkError(ciErr1, "Getting context", error_ret)) {
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += ("*** Got context\n");
//=========================================================================================
    cl_queue_properties props[] = {
            CL_QUEUE_PROPERTIES,
            CL_QUEUE_PROFILING_ENABLE,
            0
    };
    cqCommandQueue = clCreateCommandQueueWithProperties(cxGPUContext, cdDevice, props, &ciErr1);
    if (checkError(ciErr1, "Getting command queue", error_ret)) {
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += "*** Got commandqueue\n";
//=========================================================================================
    // Bus Speed Download
    std::string message;
//    std::string message2;
//    output += BusSpeedDownload(cdDevice, cxGPUContext, cqCommandQueue, message, false);
//    output += BusSpeedDownload(cdDevice, cxGPUContext, cqCommandQueue, message2, true);
//    message += message2;
////=========================================================================================
    // Get a class reference for this object
    jclass thisClass = env->GetObjectClass(thisObj);

    // Get the Method ID for method "callback", which takes no arg and return void
    jmethodID midwriteFile = env->GetMethodID(thisClass, "writeFile", "(Ljava/lang/String;Ljava/lang/String;)V");
//    if (NULL == midwriteFile)
//        output += "Java's method not found!\n ";
//    else {
//        // Call back the method (which returns void), baed on the Method ID
//        jstring jmessage = env->NewStringUTF(message.c_str());
//        jstring fileName = env->NewStringUTF("busSpeedDownload.csv");
//        env->CallVoidMethod(thisObj, midwriteFile, jmessage, fileName);
//        output += "*** In C, call back Java's writeFile()\n";
//    }
////=========================================================================================
//    // Bus Speed Readback
//    output += "\n\n\n";
//    //output += BusSpeedReadback(cdDevice, cxGPUContext, cqCommandQueue, message, false);
//    //output += BusSpeedReadback(cdDevice, cxGPUContext, cqCommandQueue, message2, true);
//    output += matrixMul(cdDevice, cxGPUContext, cqCommandQueue, message);
//    message += message2;
////=========================================================================================
//    if (NULL == midwriteFile)
//        output += "Java's method not found!\n ";
//    else {
//        // Call back the method (which returns void), baed on the Method ID
//        jstring jmessage = env->NewStringUTF(message.c_str());
//        jstring fileName = env->NewStringUTF("busSpeedReadback.csv");
//        env->CallVoidMethod(thisObj, midwriteFile, jmessage, fileName);
//        output += "*** In C, call back Java's writeFile()\n";
//    }
//=========================================================================================
    output += matrixMul(cdDevice, cxGPUContext, cqCommandQueue, message);
//=========================================================================================
    if (NULL == midwriteFile)
        output += "Java's method not found!\n ";
    else {
        // Call back the method (which returns void), baed on the Method ID
        jstring jmessage = env->NewStringUTF(message.c_str());
        jstring fileName = env->NewStringUTF("matrixMul.csv");
        env->CallVoidMethod(thisObj, midwriteFile, jmessage, fileName);
        output += "*** In C, call back Java's writeFile()\n";
    }
//=========================================================================================
    Cleanup (output, EXIT_SUCCESS);
    return env->NewStringUTF(output.c_str());
}
void Cleanup (std::string& output, int iExitCode) {
//    if(cPathAndName)free(cPathAndName);
//    if(cSourceCL)free(cSourceCL);
//    if (ckKernel)clReleaseKernel(ckKernel);
//    if (cpProgram)clReleaseProgram(cpProgram);
    if (cqCommandQueue)clReleaseCommandQueue(cqCommandQueue);
    if (cxGPUContext)clReleaseContext(cxGPUContext);
//    if (cmDevSrcA)clReleaseMemObject(cmDevSrcA);
//    if (cmDevSrcB)clReleaseMemObject(cmDevSrcB);
//    if (cmDevDst)clReleaseMemObject(cmDevDst);

//    free(srcA);
//    free(srcB);
//    free(dst);
//    free(Golden);

    if (iExitCode == EXIT_SUCCESS)
        output += "\n******* PASSed\n\n\n";
    else
        output += "\n******* FAILed\n\n\n";
}