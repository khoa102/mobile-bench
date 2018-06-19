//
// Created by Khoa Tran on 5/29/18.
//
#include <string>
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/opencl.h>
#include <CL/err_code.h>
#include <stdio.h>
#include <Event.h>

#ifndef MYFIRSTAPP_BUSSPEEDDOWNLOAD_H
#define MYFIRSTAPP_BUSSPEEDDOWNLOAD_H

std::string BusSpeedDownload(cl_device_id id,
                         cl_context ctx,
                         cl_command_queue queue, std::string& message, bool pinned);

#endif //MYFIRSTAPP_BUSSPEEDDOWNLOAD_H
