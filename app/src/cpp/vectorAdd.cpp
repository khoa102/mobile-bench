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
#include "vectorAdd.h"
using namespace std::chrono;


// Inline your kernel or use a separate .cl file
const char *kernelSource =
"__kernel void VectorAdd(  __global float *a,                       \n" \
"                       __global float *b,                       \n" \
"                       __global float *c,                       \n" \
"                       int n)                    \n"	\
"{                                                               \n"	\
"    //Get our global thread ID                                  \n"	\
"    int id = get_global_id(0);                                  \n"	\
"                                                                \n"	\
"    //Make sure we do not go out of bounds                      \n"	\
"    if (id < n)                                                 \n"	\
"        c[id] = a[id] + b[id];                                  \n"	\
"}                                                               \n"	\
                                                                "\n" ;
float *srcA, *srcB, *dst;        // Host buffers for OpenCL test
float *Golden;                   // Host buffer for host golden processing cross check

cl_context cxGPUContext;        // OpenCL context
cl_command_queue cqCommandQueue;// OpenCL command que
cl_platform_id cpPlatform;      // OpenCL platform
cl_device_id cdDevice;          // OpenCL device
cl_program cpProgram;           // OpenCL program
cl_kernel ckKernel;             // OpenCL kernel
cl_mem cmDevSrcA;               // OpenCL device source buffer A
cl_mem cmDevSrcB;               // OpenCL device source buffer B
cl_mem cmDevDst;                // OpenCL device destination buffer
size_t szGlobalWorkSize;        // 1D var for Total # of work items
size_t szLocalWorkSize;		// 1D var for # of work items in the work group
//size_t szParmDataBytes;		// Byte size of context information
//size_t szKernelLength;		// Byte size of kernel code
cl_int ciErr1, ciErr2;		// Error code var
//char* cPathAndName = NULL;      // var for full paths to data, src, etc.
//char* cSourceCL = NULL;         // Buffer to hold source for compilation

int iNumElements;        //Number of elements on host machine
std::string error_ret;          // Store the error message returned from check_error
void VectorAddHost(const float* pfData1, const float* pfData2, float* pfResult, int iNumElements);
void Cleanup (std::string& output, int iExitCode);


extern "C" JNIEXPORT jstring JNICALL Java_com_example_myfirstapp_MainActivity_vectorAdd(JNIEnv *env, jobject thisObj, jint dataSize=2560){
    std::string output = "Hello from C++!!!\n\n";

    // Getting the source size
    size_t sourceSize[]    = { strlen(kernelSource) };
//=========================================================================================
    iNumElements = (int) dataSize;
    szGlobalWorkSize = iNumElements;
    szLocalWorkSize = 256;
    output += "*** Get Data Size: " + std::to_string(iNumElements) + " \n";

    srcA = (float *)malloc(sizeof(cl_float) * szGlobalWorkSize);
    srcB = (float *)malloc(sizeof(cl_float) * szGlobalWorkSize);
    dst = (float *)malloc(sizeof(cl_float) * szGlobalWorkSize);
    Golden = (float *)malloc(sizeof(cl_float) * iNumElements);
    for(int idx = 0; idx < szGlobalWorkSize; idx++) {
        srcA[idx] = (float)idx;
        srcB[idx] = (float)idx;
    }
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
    cpProgram = clCreateProgramWithSource(cxGPUContext, 1, &kernelSource, sourceSize, &ciErr1);
    if (checkError(ciErr1, "Creating Program with source", error_ret)) {
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += "*** Got createprogramwithsource\n";
//=========================================================================================
    ciErr1 = clBuildProgram(cpProgram, 0, NULL, NULL, NULL, NULL);
    if (ciErr1 != CL_SUCCESS) {
        char buffer [1000];
        int n;
        std::string error;
        std::string temp;
        n = sprintf(buffer, "Error in clBuildProgram, Line %u in file %s !!! Error code = %d\n\n", __LINE__, __FILE__, ciErr1);
        if (n > 0) {
            temp = buffer;
            error += temp;
        }
        size_t length;
        char buffer2[2048];
        clGetProgramBuildInfo(cpProgram, cdDevice, CL_PROGRAM_BUILD_LOG, sizeof(buffer2), buffer2, &length);
        n = sprintf(buffer, "--- Build log ---\n%s\n", buffer2);
        if (n > 0) {
            temp = buffer;
            error += temp;
        }
        Cleanup(error, EXIT_FAILURE);
        return env->NewStringUTF(error.c_str());
    }
    output += "*** Got buildprogram\n";
//=========================================================================================
    ckKernel = clCreateKernel(cpProgram, "VectorAdd", &ciErr1);
    if (checkError(ciErr1, "Creating Kernel", error_ret)) {
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += "*** Got createkernel\n";
//=========================================================================================
    cmDevSrcA = clCreateBuffer(cxGPUContext, CL_MEM_READ_ONLY, sizeof(cl_float) * szGlobalWorkSize, NULL, &ciErr1);
    cmDevSrcB = clCreateBuffer(cxGPUContext, CL_MEM_READ_ONLY, sizeof(cl_float) * szGlobalWorkSize, NULL, &ciErr2);
    ciErr1 |= ciErr2;           // What is |= means?
    cmDevDst = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, sizeof(cl_float) * szGlobalWorkSize, NULL, &ciErr2);
    ciErr1 |= ciErr2;
    if (checkError(ciErr1, "Creating buffer", error_ret)) {
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += "*** Got createbuffer\n";
//=========================================================================================
    ciErr1 = clSetKernelArg(ckKernel, 0, sizeof(cl_mem), (void*)&cmDevSrcA);
    ciErr1 |= clSetKernelArg(ckKernel, 1, sizeof(cl_mem), (void*)&cmDevSrcB);
    ciErr1 |= clSetKernelArg(ckKernel, 2, sizeof(cl_mem), (void*)&cmDevDst);
    ciErr1 |= clSetKernelArg(ckKernel, 3, sizeof(cl_int), (void*)&iNumElements);
    if (checkError(ciErr1, "Setting kernel arguments", error_ret)) {
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += "*** Got setkernelarg\n";
//=========================================================================================
    ciErr1 = clEnqueueWriteBuffer(cqCommandQueue, cmDevSrcA, CL_FALSE, 0, sizeof(cl_float) * szGlobalWorkSize, srcA, 0, NULL, NULL);
    ciErr1 |= clEnqueueWriteBuffer(cqCommandQueue, cmDevSrcB, CL_FALSE, 0, sizeof(cl_float) * szGlobalWorkSize, srcB, 0, NULL, NULL);
    if (checkError(ciErr1, "Writing buffer", error_ret)) {
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += "*** Got enqueuewritebuffer\n";
//=========================================================================================
    cl_event prof_event;      // Create an event to profile preformance
    output += "*** Set profiling event\n";
    double averageGpuRunTime = 0;
    cl_ulong startTime = (cl_ulong) 0;
    cl_ulong endTime = (cl_ulong) 0;
    size_t return_bytes;
    for (int i = 0; i < 10; i ++) {
        ciErr1 = clEnqueueNDRangeKernel(cqCommandQueue, ckKernel, 1, NULL, &szGlobalWorkSize,
                                        &szLocalWorkSize, 0, NULL, &prof_event);
        if (checkError(ciErr1, "Enqueueing kernel", error_ret)) {
            Cleanup(error_ret, EXIT_FAILURE);
            return env->NewStringUTF(error_ret.c_str());
        }
        // Make sure the kernel is executed
        clFinish(cqCommandQueue);

        ciErr1 = clGetEventProfilingInfo(prof_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong),
                                         &startTime, &return_bytes);
        ciErr1 |= clGetEventProfilingInfo(prof_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong),
                                          &endTime, &return_bytes);
        if (checkError(ciErr1, "Getting Event Profiling Info", error_ret)) {
            Cleanup(error_ret, EXIT_FAILURE);
            return env->NewStringUTF(error_ret.c_str());
        }
        averageGpuRunTime += (double) (endTime - startTime)/10.0;
    }
    output += "*** Got enqueuendrangekernel\n";
    clFinish(cqCommandQueue);
//=========================================================================================
    ciErr1 = clEnqueueReadBuffer(cqCommandQueue, cmDevDst, CL_TRUE, 0, sizeof(cl_float) * szGlobalWorkSize, dst, 0, NULL, NULL);
    if (checkError(ciErr1, "Reading result buffer", error_ret)) {
        Cleanup(error_ret, EXIT_FAILURE);
        return env->NewStringUTF(error_ret.c_str());
    }
    output += "*** Got enqueuereadbuffer\n";
//=========================================================================================
    output += ("\n");
    output += "*** Got result from GPU \n";
    char buffer [1000];
    int n;
    std::string temp;
    for(int idx = 0; idx < 5; idx++) {
        n = sprintf(buffer, "%f ", dst[idx]);
        if (n > 0) {
            temp = buffer;
            output += temp;
        }
    }
    output += (" ... ...");
//=========================================================================================
    milliseconds start;
    milliseconds end;
    milliseconds cpuRunTime;
    double averageCpuRunTime = 0;
    for (int i = 0; i < 10; i++) {
         start = duration_cast<milliseconds>(
                system_clock::now().time_since_epoch()
        );

        VectorAddHost((const float *) srcA, (const float *) srcB, (float *) Golden, iNumElements);

        end = duration_cast<milliseconds>(
                system_clock::now().time_since_epoch()
        );
        cpuRunTime = end - start;
        averageCpuRunTime += (float)cpuRunTime.count()/ 10.0;
    }

    output += "\n\n*** Got result from HOST\n";

    for(int idx = 0; idx < 5; idx++) {
        n = sprintf(buffer, "%f ", Golden[idx]);
        if (n > 0) {
            temp = buffer;
            output += temp;
        }
    }
    output += " ... ...";
//=========================================================================================
    output += "\n\n*** Got GPU run time\n";
    n = sprintf(buffer, "Kernel is queued at %f ms\n", (cl_double)startTime*1.0e-6);
    if (n > 0) {
        temp = buffer;
        output += temp;
    }
    n = sprintf(buffer, "Kernel is end at %f ms\n", (cl_double)endTime*1.0e-6);
    if (n > 0) {
        temp = buffer;
        output += temp;
    }
    n = sprintf(buffer, "Kernel runtime is: %f ms\n", averageGpuRunTime*1.0e-6);
    if (n > 0) {
        temp = buffer;
        output += temp;
    }


    output += "\n\n*** Got CPU run time\n";
    n = sprintf(buffer, "CPU starts at %lld ms\n", start.count());
    if (n > 0) {
        temp = buffer;
        output += temp;
    }
    n = sprintf(buffer, "CPU ends at %lld ms\n", end.count());
    if (n > 0) {
        temp = buffer;
        output += temp;
    }
    n = sprintf(buffer, "CPU  runtime is: %f ms\n\n", averageCpuRunTime);
    if (n > 0) {
        temp = buffer;
        output += temp;
    }
//=========================================================================================
    bool bMatch = true;
    for(int idx = 0; idx < szGlobalWorkSize; idx++) {
        if(Golden[idx] != dst[idx]) {
            bMatch = false;
            n = sprintf(buffer,"idx %d doesn't match. %f vs %f", idx, Golden[idx], dst[idx]);
            if (n > 0) {
                temp = buffer;
                output += temp;
            }
            break;
        }
    }

    output += "\n\n*** Got checkMatch";
    Cleanup (output, (bMatch == true) ? EXIT_SUCCESS : EXIT_FAILURE);

//=========================================================================================
    // Get a class reference for this object
    jclass thisClass = env->GetObjectClass(thisObj);

    // Get the Method ID for method "callback", which takes no arg and return void
    jmethodID midwriteFile = env->GetMethodID(thisClass, "writeFile", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (NULL == midwriteFile)
        output += "Java's method not found!\n ";
    else {
        // Call back the method (which returns void), baed on the Method ID
        std::string temp = std::to_string(iNumElements) + "," + std::to_string(averageCpuRunTime) + "," + std::to_string(averageGpuRunTime*1.0e-6) + "\n";
        jstring message = env->NewStringUTF(temp.c_str());
        jstring fileName = env->NewStringUTF("graph-data.csv");
        env->CallVoidMethod(thisObj, midwriteFile, message, fileName);
        output += "*** In C, call back Java's writeFile()\n";
    }

//=========================================================================================
    return env->NewStringUTF(output.c_str());
}
//Helper functions
void Cleanup (std::string& output, int iExitCode)
{
//    if(cPathAndName)free(cPathAndName);
//    if(cSourceCL)free(cSourceCL);
    if(ckKernel)clReleaseKernel(ckKernel);
    if(cpProgram)clReleaseProgram(cpProgram);
    if(cqCommandQueue)clReleaseCommandQueue(cqCommandQueue);
    if(cxGPUContext)clReleaseContext(cxGPUContext);
    if(cmDevSrcA)clReleaseMemObject(cmDevSrcA);
    if(cmDevSrcB)clReleaseMemObject(cmDevSrcB);
    if(cmDevDst)clReleaseMemObject(cmDevDst);

    free(srcA);
    free(srcB);
    free (dst);
    free(Golden);

    if(iExitCode == EXIT_SUCCESS)
        output += "\n******* PASSed\n\n\n";
    else
        output += "\n******* FAILed\n\n\n";
}

void VectorAddHost(const float* pfData1, const float* pfData2, float* pfResult, int iNumElements) {
    int i;
    for (i = 0; i < iNumElements; i++) {
        pfResult[i] = pfData1[i] + pfData2[i];
    }
}
