//
// Created by Khoa Tran on 6/5/18.
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
#include "Event.h"

#ifndef MYFIRSTAPP_MATRIXMUL_H
#define MYFIRSTAPP_MATRIXMUL_H

std::string matrixMul(cl_device_id id,
                             cl_context ctx,
                             cl_command_queue queue, std::string& message);

#endif //MYFIRSTAPP_MATRIXMUL_H
