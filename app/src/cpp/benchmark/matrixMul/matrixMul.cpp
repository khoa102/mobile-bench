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
#include "matrixMul.h"
#include "Event.h"

using namespace std::chrono;

const char *kernelSource =
        "__kernel void MatrixMul(int N,int M, int P, __global const float* a, __global const float* b, __global float* c){\n"\
    "\t// get index into global data array  \n"\
    "    int row, col, k;\n"\
    "    row = get_global_id(1);\n"\
    "    col = get_global_id(0);\n"\
    "\n"\
    "    // bound check (equivalent to the limit on a 'for' loop for standard/serial C code\n"\
    "    if (row >= N || col >= M)\n"\
    "    {   \n"\
    "        return; \n"\
    "    }\n"\
    "\n"\
    "    // C(i, j) = sum(over K) A(i, k) * B(k, j)\n"\
    "    // C[N][M] = A[N][P] * B[P][M]\n"\
    "    float temp = 0.0f;\n"\
    "    for (k = 0; k < P; k ++){                              \n"\
    "         temp += a[row * P + k] * b[k * M + col];          \n"\
    "    }                                                      \n"\
    "    c[row * M + col] = temp;                               \n"\
    "}                                                          \n"    \
                                                                "\n";

float *srcA, *srcB, *dst;        // Host buffers for OpenCL test
float *Golden;                   // Host buffer for host golden processing cross check

//cl_context cxGPUContext;        // OpenCL context
//cl_command_queue cqCommandQueue;// OpenCL command que
//cl_platform_id cpPlatform;      // OpenCL platform
//cl_device_id cdDevice;          // OpenCL device
cl_program cpProgram;           // OpenCL program
cl_kernel ckKernel;             // OpenCL kernel
cl_mem cmDevSrcA;               // OpenCL device source buffer A
cl_mem cmDevSrcB;               // OpenCL device source buffer B 
cl_mem cmDevDst;                // OpenCL device destination buffer 
size_t szLocalWorkSize;     // 1D var for # of work items in the work group 
//size_t szParmDataBytes;     // Byte size of context information
size_t szKernelLength;      // Byte size of kernel code
cl_int ciErr1, ciErr2;      // Error code var
//char* cPathAndName = NULL;      // var for full paths to data, src, etc.
//char* cSourceCL = NULL;         // Buffer to hold source for compilation

int iNumElements = 2560;        //Number of elements on host machine
std::string error_ret;          // Store the error message returned from check_error
int npasses = 3;

void
MatrixMulHost(int M, int N, int P, const float *pfData1, const float *pfData2, float *pfResult);

void Cleanup(std::string &output, int iExitCode);

std::string matrixMul(cl_device_id id,
                      cl_context ctx,
                      cl_command_queue queue, std::string &message) {
    std::string output = "Hello from C++!!!\n\n";
    int dataInput = 3;
    message = "FileSize,AverageCPU,AverageGPU,AverageRead,AverageWrite\n";

    // Execution (start-end) time
    double averageGPU[dataInput];
    double averageCPU[dataInput];
    double averageRead[dataInput];
    double averageWrite[dataInput];

    // Getting the source size
    size_t sourceSize[] = {strlen(kernelSource)};
    bool bMatch;
    int sizes[10] = {1, 256, 512, 768, 1024, 1280, 1536, 1792, 2048};

    for (int i = 0; i < dataInput; i ++){
        averageCPU[i] = 0;
        averageGPU[i] = 0;
        averageRead[i] = 0;
        averageWrite[i] = 0;
    }
    for (int i = 0; i < dataInput; i++) {
//=========================================================================================
        int N, M, P; // C[N][M] = A[N][P] * B[P, M]
        size_t szA, szB, szC;

        N = M = P = sizes[i];
        const size_t szGlobalWorkSize[2] = {(size_t) N, (size_t) M};
        szLocalWorkSize = 256;  //assigned but not yet used
        szA = N * P;
        szB = P * M;
        szC = N * M;

        srcA = (float *) malloc(sizeof(cl_float) * szA);
        srcB = (float *) malloc(sizeof(cl_float) * szB);
        dst = (float *) malloc(sizeof(cl_float) * szC);
        Golden = (float *) malloc(sizeof(cl_float) * szC);

        for (int idx = 0; idx < szA; idx++) {
            srcA[idx] = (float) idx;
        }

        for (int idx = 0; idx < szB; idx++) {
            srcB[idx] = (float) idx;
        }
//=========================================================================================
        cpProgram = clCreateProgramWithSource(ctx, 1, &kernelSource, sourceSize, &ciErr1);
        if (checkError(ciErr1, "Creating Program with source", error_ret)) {
            Cleanup(error_ret, EXIT_FAILURE);
            return error_ret;
        }
        output += "*** Got createprogramwithsource\n";
//=========================================================================================
        ciErr1 = clBuildProgram(cpProgram, 0, NULL, NULL, NULL, NULL);
        if (ciErr1 != CL_SUCCESS) {
            char buffer[1000];
            int n;
            std::string error;
            std::string temp;
            n = sprintf(buffer,
                        "Error in clBuildProgram, Line %u in file %s !!! Error code = %d\n\n",
                        __LINE__, __FILE__, ciErr1);
            if (n > 0) {
                temp = buffer;
                error += temp;
            }
            size_t length;
            char buffer2[2048];
            clGetProgramBuildInfo(cpProgram, id, CL_PROGRAM_BUILD_LOG, sizeof(buffer2), buffer2,
                                  &length);
            n = sprintf(buffer, "--- Build log ---\n%s\n", buffer2);
            if (n > 0) {
                temp = buffer;
                error += temp;
            }
            Cleanup(error, EXIT_FAILURE);
            return error_ret;
        }
        output += "*** Got buildprogram\n";
//=========================================================================================
        ckKernel = clCreateKernel(cpProgram, "MatrixMul", &ciErr1);
        if (checkError(ciErr1, "Creating Kernel", error_ret)) {
            Cleanup(error_ret, EXIT_FAILURE);
            return error_ret;
        }
        output += "*** Got createkernel\n";
//=========================================================================================
        cmDevSrcA = clCreateBuffer(ctx, CL_MEM_READ_ONLY, sizeof(cl_float) * szA, NULL,
                                   &ciErr1);
        cmDevSrcB = clCreateBuffer(ctx, CL_MEM_READ_ONLY, sizeof(cl_float) * szB, NULL,
                                   &ciErr2);
        ciErr1 |= ciErr2;
        cmDevDst = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(cl_float) * szC, NULL,
                                  &ciErr2);
        ciErr1 |= ciErr2;
        if (checkError(ciErr1, "Creating buffer", error_ret)) {
            Cleanup(error_ret, EXIT_FAILURE);
            return error_ret;
        }
        output += "*** Got createbuffer\n";
//=========================================================================================
        ciErr1 = clSetKernelArg(ckKernel, 0, sizeof(cl_int), (void *) &N);
        ciErr1 |= clSetKernelArg(ckKernel, 1, sizeof(cl_int), (void *) &M);
        ciErr1 |= clSetKernelArg(ckKernel, 2, sizeof(cl_int), (void *) &P);
        ciErr1 |= clSetKernelArg(ckKernel, 3, sizeof(cl_mem), (void *) &cmDevSrcA);
        ciErr1 |= clSetKernelArg(ckKernel, 4, sizeof(cl_mem), (void *) &cmDevSrcB);
        ciErr1 |= clSetKernelArg(ckKernel, 5, sizeof(cl_mem), (void *) &cmDevDst);
        if (checkError(ciErr1, "Setting kernel arguments", error_ret)) {
            Cleanup(error_ret, EXIT_FAILURE);
            return error_ret;
        }
        output += "*** Got setkernelarg\n";
//=========================================================================================
//        Event evWrite("Write Buffer");
//        ciErr1 = clEnqueueWriteBuffer(queue, cmDevSrcA, CL_FALSE, 0, sizeof(cl_float) * szA,
//                                      srcA,
//                                      0, NULL, &evWrite.CLEvent());
//        if ( checkError(ciErr1, "Writing buffer", error_ret)) {
//            return error_ret;
//        }
//        ciErr1 = clWaitForEvents(1, &evWrite.CLEvent());
//        if (checkError(ciErr1, "Waiting for event", error_ret)) {
//            return error_ret;
//        }


        for (int i = 0; i < npasses; i++) {
            Event evWrite1("Write Buffer");
            Event evWrite2("Write Buffer");
            ciErr1 = clEnqueueWriteBuffer(queue, cmDevSrcA, CL_FALSE, 0, sizeof(cl_float) * szA,
                                          srcA,
                                          0, NULL, &evWrite1.CLEvent());

//            ciErr1 = clWaitForEvents(1, &evWrite1.CLEvent());
//            if (checkError(ciErr1, "Waiting for event", error_ret)) {
//                return error_ret;
//            }

            ciErr1 |= clEnqueueWriteBuffer(queue, cmDevSrcB, CL_FALSE, 0, sizeof(cl_float) * szB,
                                           srcB,
                                           0, NULL, &evWrite2.CLEvent());
            if (checkError(ciErr1, "Writing buffer", error_ret)) {
                Cleanup(error_ret, EXIT_FAILURE);
                return error_ret + std::to_string(i);
            }


            output += "*** Got enqueuewritebuffer\n";

//=========================================================================================
            Event evMatrixMul("matrixMul");
            output += "*** Set profiling event\n";

            ciErr1 = clEnqueueNDRangeKernel(queue, ckKernel, 2, NULL, szGlobalWorkSize,
                                            NULL, 0, NULL, &evMatrixMul.CLEvent());
            if (checkError(ciErr1, "Enqueueing kernel", error_ret)) {
                Cleanup(error_ret, EXIT_FAILURE);
                return error_ret;
            }


            output += "*** Got enqueuendrangekernel\n";
//=========================================================================================
            Event evRead("Read Buffer");
            ciErr1 = clEnqueueReadBuffer(queue, cmDevDst, CL_TRUE, 0, sizeof(cl_float) * szC, dst,
                                         0,
                                         NULL, &evRead.CLEvent());

            if (checkError(ciErr1, "Reading result buffer", error_ret)) {
                Cleanup(error_ret, EXIT_FAILURE);
                return error_ret;
            }

            output += "*** Got enqueuereadbuffer\n";
//=========================================================================================
            milliseconds start;
            milliseconds end;
            milliseconds cpuRunTime;

            start = duration_cast<milliseconds>(
                    system_clock::now().time_since_epoch()
            );

            MatrixMulHost(M, N, P, (const float *) srcA, (const float *) srcB, (float *) Golden);

            end = duration_cast<milliseconds>(
                    system_clock::now().time_since_epoch()
            );
            cpuRunTime = end - start;
            averageCPU[i] += (double) cpuRunTime.count() / npasses;
            output += "*** Got CPU run time\n";
//=========================================================================================
            output += "*** Got GPU run time\n";

            // Finish and flush everything in buffer
//            ciErr1 = clFinish(queue);
//            if (checkError(ciErr1, "Finish all in queue", error_ret)) {
//                return error_ret;
//            }


            // Get write buffer time
            if (!evWrite1.FillTimingInfo(error_ret))
                return error_ret;
            if (!evWrite2.FillTimingInfo(error_ret))
                return error_ret;
            averageWrite[i] += evWrite1.StartEndRuntime() / 1.e6 / npasses;
            averageWrite[i] += evWrite2.StartEndRuntime() / 1.e6 / npasses;

            // Get Matrix Tim
            if (!evMatrixMul.FillTimingInfo(error_ret))
                return error_ret;
            averageGPU[i] += (double) evMatrixMul.StartEndRuntime() / 1.0e6 / npasses;

            // Get Read time
            if (!evRead.FillTimingInfo(error_ret))
                return error_ret;
            averageRead[i] += evRead.StartEndRuntime() / 1.0e6 / npasses;
        }

//=========================================================================================

        output += "\n\nC[N][M] = A[N][P] * B[P, M]\n";
        output += "Matrix Size: M=" + std::to_string(M) + " N=" + std::to_string(N) + " P=" +
                  std::to_string(P) + "\n";

        output += ("\n");
        output += "*** Got result from GPU \n";
        char buffer[1000];
        int n;
        std::string temp;
        for (int idx = 0; idx < 5; idx++) {
            n = sprintf(buffer, "%f ", dst[idx]);
            if (n > 0) {
                temp = buffer;
                output += temp;
            }
        }
        output += (" ... ...");
//=========================================================================================
        output += "\n\n*** Got result from HOST\n";
        for (int idx = 0; idx < 5; idx++) {
            n = sprintf(buffer, "%f ", Golden[idx]);
            if (n > 0) {
                temp = buffer;
                output += temp;
            }
        }
        output += " ... ...";
//=========================================================================================
        n = sprintf(buffer, "\n\nKernel runtime is: %f ms\n", averageGPU[i]);
        if (n > 0) {
            temp = buffer;
            output += temp;
        }
//=========================================================================================
        n = sprintf(buffer, "CPU  runtime is: %f ms\n\n", averageCPU[i]);
        if (n > 0) {
            temp = buffer;
            output += temp;
        }

//=========================================================================================
        bMatch = true;
        for (int idx = 0; idx < szC; idx++) {
            if (Golden[idx] != dst[idx]) {
                bMatch = false;
                n = sprintf(buffer, "idx %d doesn't match. %f vs %f", idx, Golden[idx], dst[idx]);
                if (n > 0) {
                    temp = buffer;
                    output += temp;
                }
                break;
            }
        }

        output += "\n\n*** Got checkMatch";
        if (bMatch == true)
            output += "\n******* PASSed\n\n\n";
        else
            output += "\n******* FAILed\n\n\n";

        // Add to message
        message += std::to_string(sizes[i]) + "," + std::to_string(averageCPU[i]) + ","
                   + std::to_string(averageGPU[i]) + "," + std::to_string(averageRead[i])
                   + "," + std::to_string(averageWrite[i]) + "\n";
    }
//=========================================================================================
//    // Get a class reference for this object
//    jclass thisClass = env->GetObjectClass(thisObj);
//
//    // Get the Method ID for method "callback", which takes no arg and return void
//    jmethodID midwriteFile = env->GetMethodID(thisClass, "writeFile", "(Ljava/lang/String;Ljava/lang/String;)V");
//    if (NULL == midwriteFile)
//        output += "Java's method not found!\n ";
//    else {
//        // Call back the method (which returns void), baed on the Method ID
//        std::string temp = std::to_string(iNumElements) + "," + std::to_string(averageCpuRunTime) + "," + std::to_string(averageGpuRunTime*1.0e-6) + "\n";
//        jstring message = env->NewStringUTF(temp.c_str());
//        jstring fileName = env->NewStringUTF("graph-data.csv");
//        env->CallVoidMethod(thisObj, midwriteFile, message, fileName);
//        output += "*** In C, call back Java's writeFile()\n";
//    }

//=========================================================================================
    Cleanup(output, (bMatch == true) ? EXIT_SUCCESS : EXIT_FAILURE);
    return output;
}


//Helper functions
void Cleanup(std::string &output, int iExitCode) {
//    if(cPathAndName)free(cPathAndName);
//    if(cSourceCL)free(cSourceCL);
    if (ckKernel)clReleaseKernel(ckKernel);
    if (cpProgram)clReleaseProgram(cpProgram);
//    if(cqCommandQueue)clReleaseCommandQueue(cqCommandQueue);
//    if(cxGPUContext)clReleaseContext(cxGPUContext);
    if (cmDevSrcA)clReleaseMemObject(cmDevSrcA);
    if (cmDevSrcB)clReleaseMemObject(cmDevSrcB);
    if (cmDevDst)clReleaseMemObject(cmDevDst);

    free(srcA);
    free(srcB);
    free(dst);
    free(Golden);

    if (iExitCode == EXIT_FAILURE)
        output += "\n******* FAILed\n\n\n";
}

// Multiply 2 matrix
// C[N][M] = A[N][P] * B[P, M]
void
MatrixMulHost(int M, int N, int P, const float *pfData1, const float *pfData2, float *pfResult) {
    int row, col, k;
    for (row = 0; row < N; row++) {
        for (col = 0; col < M; col++) {
            pfResult[row * N + col] = 0.0f;
            for (k = 0; k < P; k++) {
                // C(i, j) = sum(over K) A(i, k) * B(k, j)
                pfResult[row * M + col] += pfData1[row * P + k] * pfData2[k * M + col];
            }
        }
    }
}