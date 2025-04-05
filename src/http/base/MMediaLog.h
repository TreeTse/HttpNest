#pragma once 
#include "base/LogStream.h"
#include <iostream>
using namespace nest::base;

#define HTTP_DEBUG_ON 1

#ifdef HTTP_DEBUG_ON
#define HTTP_TRACE LOG_TRACE << "HTTP::"
#define HTTP_DEBUG LOG_DEBUG << "HTTP::"
#define HTTP_INFO LOG_INFO << "HTTP::"
#elif
#define HTTP_TRACE if(0) LOG_TRACE
#define HTTP_DEBUG if(0) LOG_DEBUG
#define HTTP_INFO if(0) LOG_INFO
#endif

#define HTTP_WARN LOG_WARN
#define HTTP_ERROR LOG_ERROR