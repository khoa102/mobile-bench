//
// Created by Khoa Tran on 5/21/18.
//
#include <jni.h>
#include <string>
#include "native-lib.h"
#include <iostream>
#include <vector>
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/opencl.h>


//float *srcA, *srcB, *dst;        // Host buffers for OpenCL test
//float *Golden;                   // Host buffer for host golden processing cross

//cl_context cxGPUContext;        //openCL context
//cl_command_queue cqCommandQueue;// OpenCL command que
cl_platform_id cpPlatform;      // OpenCL platform
cl_device_id cdDevice;          // OpenCL device
//cl_program cpProgram;           // OpenCL program
//cl_kernel ckKernel;             // OpenCL kernel
//cl_mem cmDevSrcA;               // OpenCL device source buffer A
//cl_mem cmDevSrcB;               // OpenCL device source buffer B
//cl_mem cmDevDst;                // OpenCL device destination buffer
//size_t szLocalWorkSize;     // 1D var for # of work items in the work group
//size_t szParmDataBytes;     // Byte size of context information
//size_t szKernelLength;      // Byte size of kernel code
cl_int ciErr1, ciErr2;      // Error code var
//char* cPathAndName = NULL;      // var for full paths to data, src, etc.
//char* cSourceCL = NULL;         // Buffer to hold source for compilation
void Cleanup (int argc, char **argv, int iExitCode);

std::string GetPlatformName (cl_platform_id id)
{
    std::string result;
    std::string temp;
    size_t size = 0;

    // Get platform name
    ciErr1 = clGetPlatformInfo (id, CL_PLATFORM_NAME, 0, nullptr, &size);


    temp.resize (size);
    ciErr1 |= clGetPlatformInfo (id, CL_PLATFORM_NAME, size,
                       const_cast<char*> (temp.data ()), nullptr);
    if (ciErr1 != CL_SUCCESS) {
        return "Error in clGetPlatformID, Line %u in file %s !!!\n\n";
    }
    temp.resize(temp.size() - 1);
    result += temp + "\n";

    // Get the platform vendor
    ciErr1 = clGetPlatformInfo (id, CL_PLATFORM_VENDOR, 0, nullptr, &size);
    temp.resize (size);
    ciErr1 |= clGetPlatformInfo (id, CL_PLATFORM_VENDOR, size,
                       const_cast<char*> (temp.data ()), nullptr);
    if (ciErr1 != CL_SUCCESS) {
        return "Error in clGetPlatformID, Line %u in file %s !!!\n\n";
    }
    temp.resize(temp.size() - 1);
    result += temp + "\n";

    // Get the platform OpenCL version
    ciErr1 = clGetPlatformInfo (id, CL_PLATFORM_VERSION, 0, nullptr, &size);
    temp.resize (size);
    ciErr1 |= clGetPlatformInfo (id, CL_PLATFORM_VERSION, size,
                       const_cast<char*> (temp.data ()), nullptr);
    if (ciErr1 != CL_SUCCESS) {
        return "Error in clGetPlatformID, Line %u in file %s !!!\n\n";
    }
    temp.resize(temp.size() - 1);
    result += temp + " ";

    ciErr1 = clGetPlatformInfo (id, CL_PLATFORM_PROFILE, 0, nullptr, &size);
    temp.resize (size);
    ciErr1 |= clGetPlatformInfo (id, CL_PLATFORM_PROFILE, size,
                       const_cast<char*> (temp.data ()), nullptr);
    if (ciErr1 != CL_SUCCESS) {
        return "Error in clGetPlatformID, Line %u in file %s !!!\n\n";
    }
    temp.resize(temp.size() - 1);
    result += temp;

    return result;
}

std::string GetDeviceName (cl_device_id id)
{
    size_t size = 0;
    clGetDeviceInfo (id, CL_DEVICE_NAME, 0, nullptr, &size);

    std::string result;
    result.resize (size);
    ciErr1 = clGetDeviceInfo (id, CL_DEVICE_NAME, size,
                     const_cast<char*> (result.data ()), nullptr);
    if (ciErr1 != CL_SUCCESS) {
        return "Error in clGetPlatformID, Line %u in file %s !!!\n\n";
    }
    result.resize(result.size() - 1);
    return result;
}

std::string GetDeviceInfo (cl_device_id id)
{
    size_t size = 0;
    clGetDeviceInfo (id, CL_DEVICE_NAME, 0, nullptr, &size);

    std::string result;
    result.resize (size);
    ciErr1 = clGetDeviceInfo (id, CL_DEVICE_NAME, size,
                     const_cast<char*> (result.data ()), nullptr);
    if (ciErr1 != CL_SUCCESS) {
        return "Error in clGetPlatformID, Line %u in file %s !!!\n\n";
    }
    result.resize(result.size() - 1);
    return result;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_example_myfirstapp_MainActivity_getInfo(JNIEnv *env, jobject thisObj){
    std::string output = "Hello from C++!!!\n\n";
//    return env->NewStringUTF(hello.c_str());
    cl_uint platformIdCount = 0;
    clGetPlatformIDs (0, nullptr, &platformIdCount);
    if (platformIdCount == 0) {
        std::cerr << "No OpenCL platform found" << std::endl;
        return env->NewStringUTF("No OpenCL platform found");
    } else {
        output += "*** Got platform\n";
        output +=  "Found " + std::to_string(platformIdCount) + " platform(s)\n";
    }
    std::vector<cl_platform_id> platformIds (platformIdCount);
    clGetPlatformIDs (platformIdCount, platformIds.data (), nullptr);

    for (cl_uint i = 0; i < platformIdCount; ++i) {
         output += "\t (" + std::to_string(i+1) + ") : " + GetPlatformName (platformIds [i]) + "\n";
    }
//=========================================================================================
    cl_uint deviceIdCount = 0;
    clGetDeviceIDs (platformIds [0], CL_DEVICE_TYPE_ALL, 0, nullptr,
                    &deviceIdCount);

    if (deviceIdCount == 0) {
        std::cerr << "No OpenCL devices found" << std::endl;
        return env->NewStringUTF("No OpenCL devices found");
    } else {
        output += "\n*** Got device\n";
        output += "Found " + std::to_string(deviceIdCount) + " device(s)\n";
    }

    std::vector<cl_device_id> deviceIds (deviceIdCount);
    clGetDeviceIDs (platformIds [0], CL_DEVICE_TYPE_ALL, deviceIdCount,
                    deviceIds.data (), nullptr);

    for (cl_uint i = 0; i < deviceIdCount; ++i) {
        output += "\t (" + std::to_string(i+1) + ") : " + GetDeviceName (deviceIds [i]) + "\n";
    }

    // Investigate each device
    for (int j = 0; j < deviceIdCount; j++)
    {
        output += "\t-------------------------\n";
        std::string string;
        size_t size;

        // Get device name
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_NAME, 0, nullptr, &size);
        string.resize(size);
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_NAME, size, const_cast<char*> (string.data()), nullptr);
        string.resize(string.size() - 1);
        if (ciErr1 != CL_SUCCESS) {
            return env->NewStringUTF("Error in clGetDeviceInfo, Line %u in file %s !!!\n\n");
        }
        output += "\t\tName: " + string + "\n";


        // Get device OpenCL version
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_OPENCL_C_VERSION, 0, nullptr, &size);
        string.resize(size);
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_OPENCL_C_VERSION, size, const_cast<char*> (string.data()), NULL);
        string.resize(string.size() - 1);
        if (ciErr1 != CL_SUCCESS) {
            return env->NewStringUTF("Error in clGetPlatformID, Line %u in file %s !!!\n\n");
        }
        output += "\t\tVersion: " + string + "\n";

        // Get Max. Compute units
        cl_uint num;
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &num, NULL);
        if (ciErr1 != CL_SUCCESS) {
            return env->NewStringUTF("Error in clGetPlatformID, Line %u in file %s !!!\n\n");
        }
        output += "\t\tMax. Compute Units: " + std::to_string(num) + "\n";

        // Get local memory size
        cl_ulong mem_size;
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &mem_size, NULL);
        if (ciErr1 != CL_SUCCESS) {
            return env->NewStringUTF("Error in clGetPlatformID, Line %u in file %s !!!\n\n");
        }
        output += "\t\tLocal Memory Size: " + std::to_string(mem_size/1024) + " KB\n";

        // Get global memory size
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &mem_size, NULL);
        if (ciErr1 != CL_SUCCESS) {
            return env->NewStringUTF("Error in clGetPlatformID, Line %u in file %s !!!\n\n");
        }
        output += "\t\tGlobal Memory Size: " + std::to_string(mem_size/(1024*1024)) + " MB\n";

        // Get maximum buffer alloc. size
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &mem_size, NULL);
        if (ciErr1 != CL_SUCCESS) {
            return env->NewStringUTF("Error in clGetPlatformID, Line %u in file %s !!!\n\n");
        }
        output += "\t\tMax Alloc Size: " + std::to_string(mem_size/(1024*1024)) + " MB\n";

        // Get work-group size information
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &size, NULL);
        if (ciErr1 != CL_SUCCESS) {
            return env->NewStringUTF("Error in clGetPlatformID, Line %u in file %s !!!\n\n");
        }
        output += "\t\tMax Work-group Total Size: " + std::to_string(size) + "\n";


        // Find the maximum dimensions of the work-groups
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &num, NULL);
        if (ciErr1 != CL_SUCCESS) {
            return env->NewStringUTF("Error in clGetPlatformID, Line %u in file %s !!!\n\n");
        }
        // Get the max. dimensions of the work-groups
        size_t dims[num];
        ciErr1 = clGetDeviceInfo(deviceIds[j], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(dims), &dims, NULL);
        if (ciErr1 != CL_SUCCESS) {
            return env->NewStringUTF("Error in clGetPlatformID, Line %u in file %s !!!\n\n");
        }
        output += "\t\tMax Work-group Dims: ( ";
        for (size_t k = 0; k < num; k++)
        {
            output += std::to_string(dims[k]) + " ";
        }
        output += ")\n";

        output += "\t-------------------------\n";
    }

    output += "\n-------------------------\n";


    return env->NewStringUTF(output.c_str());
//=========================================================================================

}

//// Helper functions
void Cleanup (int argc, char **argv, int iExitCode)
{
//    if(cPathAndName)free(cPathAndName);
//    if(cSourceCL)free(cSourceCL);
//    if(ckKernel)clReleaseKernel(ckKernel);
//    if(cpProgram)clReleaseProgram(cpProgram);
//    if(cqCommandQueue)clReleaseCommandQueue(cqCommandQueue);
//    if(cxGPUContext)clReleaseContext(cxGPUContext);
//    if(cmDevSrcA)clReleaseMemObject(cmDevSrcA);
//    if(cmDevSrcB)clReleaseMemObject(cmDevSrcB);
//    if(cmDevDst)clReleaseMemObject(cmDevDst);
//
//    free(srcA);
//    free(srcB);
//    free (dst);
//    free(Golden);
//
//    if(iExitCode == EXIT_SUCCESS)
//        printf("\n******* PASSed\n");
//    else
//        printf("\n******* FAILed\n");
}