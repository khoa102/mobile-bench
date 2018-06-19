#include <string>
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/opencl.h>
#include <CL/err_code.h>
#include <stdio.h>
#include <Event.h>
#include <BusSpeedDownload.h>

std::string BusSpeedDownload(cl_device_id id,
                  cl_context ctx,
                  cl_command_queue queue, std::string& message, bool pinned)
{
	std::string output = "";
	int  npasses = 50;
	std::string error_ret;          // Store the error message returned from check_error
    double avgDownloadSpeed[20];
    double avgDownloadDelay[20];
    double avgDownloadTime[20];
    double avgStartEndTime[20];

    // Initialize the result array
    for (int i =0; i < 20; i++){
        avgDownloadDelay[i] = avgDownloadSpeed[i] = avgDownloadTime[i] = avgStartEndTime[i] = 0;
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
    while (sizes[nSizes-1]*1024 > 0.90 * maxAllocSizeBytes) // Why  90% of maxAllocSizeBytes and 1024 means the unit of the sizes is KB?
    {
        --nSizes;
        output += " -  dropping allocation size to keep under reported limit.\n";
        if (nSizes < 1)
        {
            output =  "Error: OpenCL reported a max allocation size less than 1kB.\b";
            return output;
        }
    }

    // Create some host memory pattern
    output += "*** creating host mem pattern\n";
    int err;
    float *hostMem;
    cl_mem hostMemObj;
    long long numMaxFloats = 1024 * (sizes[nSizes-1]) / 4;  // Why divides by 4 and * by 1024? 1024 for KBs and /4 because the size of float is 4

    if (pinned)
    {
        hostMemObj = clCreateBuffer(ctx,
                                    CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, // Other flag : USE_HOST_PTR
                                    sizeof(float)*numMaxFloats, NULL, &err);
        if (err == CL_SUCCESS)
        {
            hostMem = (float*)clEnqueueMapBuffer(queue, hostMemObj, true,
                                                 CL_MAP_READ|CL_MAP_WRITE,      // Just Read or just  write is faster than read-write
                                                 0,sizeof(float)*numMaxFloats,0,
                                                 NULL,NULL,&err);
        }
        while (err != CL_SUCCESS)
        {
            // drop the size and try again
            output += " - dropping size allocating pinned mem\n";
            --nSizes;
            if (nSizes < 1)
            {
                output = "Error: Couldn't allocated any pinned buffer\n";
                return output;
            }
            numMaxFloats = 1024 * (sizes[nSizes-1]) / 4;
            hostMemObj = clCreateBuffer(ctx,
                                        CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                        sizeof(float)*numMaxFloats, NULL, &err);
            if (err == CL_SUCCESS)
            {
                hostMem = (float*)clEnqueueMapBuffer(queue, hostMemObj, true,
                                                     CL_MAP_READ|CL_MAP_WRITE,
                                                     0,sizeof(float)*numMaxFloats,0,
                                                     NULL,NULL,&err);
            }
        }
    }
    else {
        hostMem = new float[numMaxFloats];
    }

    for (int i=0; i<numMaxFloats; i++)
        hostMem[i] = i % 77;

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
		    output += "Error: Couldn't allocated any device buffer\n";
		    return output;
		}
		numMaxFloats = 1024 * (sizes[nSizes-1]) / 4;
		mem1 = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
				      sizeof(float)*numMaxFloats, NULL, &err);
    }

    output += "*** filling device mem to force allocation\n";
    Event evDownloadPrime("DownloadPrime");
    err = clEnqueueWriteBuffer(queue, mem1, false, 0,
                               numMaxFloats*sizeof(float), hostMem,
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
//            sizeIndex = i;

            // Copy input memory to the device
            output +=  "*** copying to device " + std::to_string(sizes[sizeIndex]) + "kB\n";
            Event evDownload("Download");
            err = clEnqueueWriteBuffer(queue, mem1, false, 0,
                                       sizes[sizeIndex]*1024, hostMem,
                                       0, NULL, &evDownload.CLEvent());
            if (checkError(err, "Writing buffer", error_ret)) {
                return error_ret;
    		}
//
            // Wait for event to finish
//            output +=  "*** waiting for download to finish\n";
            err = clWaitForEvents(1, &evDownload.CLEvent());
            if (checkError(err, "Waiting for event", error_ret)) {
                return error_ret;
    		}

            // Get timings
            err = clFlush(queue);
            if (checkError(err, "Flushing command queue", error_ret)) {
                return error_ret;
    		}
            if (!evDownload.FillTimingInfo(error_ret))
                return error_ret;
            // if (verbose) evDownload.Print(cerr);

            double t = evDownload.SubmitEndRuntime() / 1.e6; // in ms
            // times[sizeIndex] = t;

            // Add timings to output
            double speed = (double(sizes[sizeIndex] * 1024.) /  (1000.*1000.)) / t;
            char sizeStr[256];
            sprintf(sizeStr, "% 7dkB", sizes[sizeIndex]);
            std::string temp = sizeStr;
            output += "DownloadSpeed for " + temp + " is " + std::to_string(speed) + " GB/sec\n";

            // Add timings to output
            double delay = evDownload.SubmitStartDelay() / 1.e6;
            output += "DownloadDelay for " + temp + " is " + std::to_string(delay) + " ms\n";
            output += "DownloadTime for " + temp + " is " + std::to_string(t) + " ms\n";

            // Store result into array
            avgDownloadDelay[sizeIndex] = delay/npasses;
            avgDownloadSpeed[sizeIndex] = speed/npasses;
            avgDownloadTime[sizeIndex] = t/npasses;
            avgStartEndTime[sizeIndex] = evDownload.StartEndRuntime() /1.e6/npasses;
        }
	//resultDB.AddResult("DownloadLatencyEstimate", "1-2kb", "ms", times[0]-(times[1]-times[0])/1.);
	//resultDB.AddResult("DownloadLatencyEstimate", "1-4kb", "ms", times[0]-(times[2]-times[0])/3.);
	//resultDB.AddResult("DownloadLatencyEstimate", "2-4kb", "ms", times[1]-(times[2]-times[1])/1.);
    }
    // Generating message to write to file
    message = "FileSize,AverageDownloadSpeed,AverageDownloadDelay,AverageDownloadTime,AverageStartEndTime,Pinned=" + std::to_string(pinned) + "\n";
    for (int i = 0; i < nSizes; i++){
        message += std::to_string(sizes[i]) + "kB," + std::to_string(avgDownloadSpeed[i]) + ","
                   + std::to_string(avgDownloadDelay[i]) + "," + std::to_string(avgDownloadTime[i])
                   + "," + std::to_string(avgStartEndTime[i]) + "\n";
    }

    // Cleanup
    err = clReleaseMemObject(mem1);
    if (checkError(err, "Release Memory Object", error_ret)) {
        return error_ret;
    }
    if (pinned)
    {
        err = clReleaseMemObject(hostMemObj);
        if (checkError(err, "Release Memory Object", error_ret)) {
            return error_ret;
        }
    }
    else {
        delete[] hostMem;
    }
    return output;
}

