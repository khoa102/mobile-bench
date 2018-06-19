#include <string>
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/opencl.h>
#include <CL/err_code.h>
#include <stdio.h>
#include <Event.h>
#include <BusSpeedReadback.h>

// Modifications:
//    Khoa Tran, Fri Jun  1  2018
//    Adapt for mobile GPU
std::string BusSpeedReadback(cl_device_id id,
                         cl_context ctx,
                         cl_command_queue queue, std::string& message, bool pinned)
{
    std::string output = "";
    int  npasses = 10;
    std::string error_ret;          // Store the error message returned from check_error
    double avgReadbackSpeed[20];
    double avgReadbackDelay[20];
    double avgReadbackTime[20];
    double avgStartEndTime[20];

    const bool waitForEvents = true;

    // Initialize the result array
    for (int i =0; i < 20; i++){
        avgReadbackDelay[i] = avgReadbackSpeed[i] = avgReadbackTime[i] = avgStartEndTime[i] = 0;
    }

    // Sizes are in kb
    int nSizes  = 20;
    int sizes[20] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,
		     32768,65536,131072,262144,524288};

    // Max sure we don't surpass the OpenCL limit.
    output += "*** getting maximum allocate size\n";
    cl_long maxAllocSizeBytes = 0;
    clGetDeviceInfo(id, CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                    sizeof(cl_long), &maxAllocSizeBytes, NULL);
    while (sizes[nSizes-1]*1024 > 0.90 * maxAllocSizeBytes)
    {
        --nSizes;
        output += " - dropping allocation size to keep under reported limit.\n";
        if (nSizes < 1)
        {
            output = "Error: OpenCL reported a max allocation size less than 1kB.\b";
            return output;
        }
    }

    // Create some host memory pattern
    output += "*** creating host mem pattern\n";
    int err;
    float *hostMem1;
    float *hostMem2;
    cl_mem hostMemObj1;
    cl_mem hostMemObj2;
    long long numMaxFloats = 1024 * (sizes[nSizes-1]) / 4;
    if (pinned)
    {
	    int err1, err2;
        hostMemObj1 = clCreateBuffer(ctx,
                                     CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                     sizeof(float)*numMaxFloats, NULL, &err1);
        if (err1 == CL_SUCCESS)
        {
            hostMem1 = (float*)clEnqueueMapBuffer(queue, hostMemObj1, true,
                                                  CL_MAP_READ|CL_MAP_WRITE,
                                                  0,sizeof(float)*numMaxFloats,0,
                                                  NULL,NULL,&err1);
        }
        hostMemObj2 = clCreateBuffer(ctx,
                                     CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                     sizeof(float)*numMaxFloats, NULL, &err2);
        if (err2 == CL_SUCCESS)
        {
            hostMem2 = (float*)clEnqueueMapBuffer(queue, hostMemObj2, true,
                                                  CL_MAP_READ|CL_MAP_WRITE,
                                                  0,sizeof(float)*numMaxFloats,0,
                                                  NULL,NULL,&err2);
        }
	    while (err1 != CL_SUCCESS || err2 != CL_SUCCESS)
	    {
	        // free the first buffer if only the second failed      ???? WHat if the first failed???
            // If first failed, then second failed so no need to free? if first succeeded but second failed then
            // need to release first
	        if (err1 == CL_SUCCESS)
		        clReleaseMemObject(hostMemObj1);

	        // drop the size and try again
	        output += " - dropping size allocating pinned mem\n";
	        --nSizes;
	        if (nSizes < 1)
	        {
		        output += "Error: Couldn't allocated any pinned buffer\n";
		        return output;
	        }
	        numMaxFloats = 1024 * (sizes[nSizes-1]) / 4;
	        hostMemObj1 = clCreateBuffer(ctx,
					 CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
					 sizeof(float)*numMaxFloats, NULL, &err1);
            if (err1 == CL_SUCCESS)
            {
                hostMem1 = (float*)clEnqueueMapBuffer(queue, hostMemObj1, true,
                                                      CL_MAP_READ|CL_MAP_WRITE,
                                                      0,sizeof(float)*numMaxFloats,0,
                                                      NULL,NULL,&err1);
            }
	        hostMemObj2 = clCreateBuffer(ctx,
					 CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
					 sizeof(float)*numMaxFloats, NULL, &err2);
            if (err2 == CL_SUCCESS)
            {
                hostMem2 = (float*)clEnqueueMapBuffer(queue, hostMemObj2, true,
                                                      CL_MAP_READ|CL_MAP_WRITE,
                                                      0,sizeof(float)*numMaxFloats,0,
                                                      NULL,NULL,&err2);
            }
	    }
    }
    else
    {
        hostMem1 = new float[numMaxFloats];
        hostMem2 = new float[numMaxFloats];
    }

    for (int i=0; i<numMaxFloats; i++) {
        hostMem1[i] = i % 77;
        hostMem2[i] = -1;
    }

    // Allocate some device memory
    output += "*** allocating device mem\n";
    cl_mem mem1 = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
                                 sizeof(float)*numMaxFloats, NULL, &err);
    while (err != CL_SUCCESS)
    {
	    // drop the size and try again
	    output += " - dropping size allocating device mem\n";
        --nSizes;
        if (nSizes < 1)
        {
            output = "Error: Couldn't allocated any device buffer\n";
            return output;
        }
        numMaxFloats = 1024 * (sizes[nSizes-1]) / 4;
        mem1 = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
                      sizeof(float)*numMaxFloats, NULL, &err);
    }
    output += "*** filling device mem to force allocation\n";
    Event evDownloadPrime("DownloadPrime");
    err = clEnqueueWriteBuffer(queue, mem1, false, 0,
                               numMaxFloats*sizeof(float), hostMem1,
                               0, NULL, &evDownloadPrime.CLEvent());
    if ( checkError(err, "Writing buffer", error_ret)) {
        return error_ret;
    }
    output += "*** waiting for download to finish\n";
    err = clWaitForEvents(1, &evDownloadPrime.CLEvent());
    if (checkError(err, "Waiting for event", error_ret)) {
        return error_ret;
    }

    // n passes, forward and backward both
    for (int pass = 0; pass < npasses; pass++)
    {
        // store the times temporarily to estimate latency
        //float times[nSizes];
        // Step through sizes forward on even passes and backward on odd
        for (int i = 0; i < nSizes; i++)
        {
            int sizeIndex;
            if ((pass%2) == 0)
                sizeIndex = i;
            else
                sizeIndex = (nSizes-1) - i;

            // Read memory back from the device
            output += "*** reading from device " + std::to_string(sizes[sizeIndex]) + "kB\n";
            Event evReadback("Readback");
            err = clEnqueueReadBuffer(queue, mem1, false, 0,
                                       sizes[sizeIndex]*1024, hostMem2,
                                       0, NULL, &evReadback.CLEvent());
            if (checkError(err, "Reading from buffer", error_ret)) {
                return error_ret;
            }

            // Wait for event to finish
            output += ">> waiting for readback to finish\n";
            err = clWaitForEvents(1, &evReadback.CLEvent());
            if ( checkError(err, "Waiting for event", error_ret)) {
                return error_ret;
            }

            // Get timings
            err = clFlush(queue);
            if ( checkError(err, "Flushing command queue", error_ret)) {
                return error_ret;
            }
            if (!evReadback.FillTimingInfo(error_ret))
                return error_ret;

            double t = evReadback.SubmitEndRuntime() / 1.e6; // in ms
            //times[sizeIndex] = t;

            // Add timings to output
            double speed = (double(sizes[sizeIndex] * 1024.) /  (1000.*1000.)) / t;
            char sizeStr[256];
            sprintf(sizeStr, "% 7dkB", sizes[sizeIndex]);
            std::string temp = sizeStr;
            output += "ReadbackSpeed for " + temp +"is " + std::to_string(speed) + " GB/sec\n";

            // Add timings to output
            double delay = evReadback.SubmitStartDelay() / 1.e6;
            output += "ReadbackDelay for " + temp + " is " + std::to_string(delay) + " ms\n";
            output += "ReadbackTime for " + temp + " is " + std::to_string(t) + " ms\n";

            // Store result into array
            avgReadbackDelay[sizeIndex] = delay/npasses;
            avgReadbackSpeed[sizeIndex] = speed/npasses;
            avgReadbackTime[sizeIndex] = t/npasses;
            avgStartEndTime[sizeIndex] = evReadback.StartEndRuntime() /1.e6/npasses;
        }
	//resultDB.AddResult("ReadbackLatencyEstimate", "1-2kb", "ms", times[0]-(times[1]-times[0])/1.);
	//resultDB.AddResult("ReadbackLatencyEstimate", "1-4kb", "ms", times[0]-(times[2]-times[0])/3.);
	//resultDB.AddResult("ReadbackLatencyEstimate", "2-4kb", "ms", times[1]-(times[2]-times[1])/1.);
    }
    // Generating message to write to file
    message = "FileSize,AverageReadbackSpeed,AverageReadbackDelay,AverageReadbackTime,AverageStartEndTime,Pinned=" + std::to_string(pinned) + "\n";
    for (int i = 0; i < nSizes; i++){
        message += std::to_string(sizes[i]) + "kB," + std::to_string(avgReadbackSpeed[i]) + ","
                   + std::to_string(avgReadbackDelay[i]) + "," + std::to_string(avgReadbackTime[i])
                   + "," + std::to_string(avgStartEndTime[i]) + "\n";
    }

    // Cleanup
    err = clReleaseMemObject(mem1);
    if (checkError(err, "Release Memory Object", error_ret)) {
        return error_ret;
    }
    if (pinned)
    {
        err = clReleaseMemObject(hostMemObj1);
        if (checkError(err, "Release Memory Object", error_ret)) {
            return error_ret;
        }
        err = clReleaseMemObject(hostMemObj2);
        if (checkError(err, "Release Memory Object", error_ret)) {
            return error_ret;
        }
    }
    else
    {
        delete[] hostMem1;
        delete[] hostMem2;
    }
    return output;
}
