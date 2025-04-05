#include <stdio.h>
#include <iostream>
#include <thread>
#include "base/Config.h"
#include "base/FileMgr.h"
#include "base/LogStream.h"
#include "base/TaskMgr.h"
#include "http/sample/HttpService.h"

using namespace nest::base;
using namespace nest::mm;

int main() {
    g_logger = new Logger(nullptr);
    g_logger->SetLogLevel(kTrace);
    if(!sConfigMgr->LoadConfig("../config/config.json")) {
        std::cerr << "Load config file failed." << std::endl;
        return -1;
    }
    ConfigPtr config = sConfigMgr->GetConfig();
    LogInfoPtr logInfo = config->GetLogInfo();
    std::cout << "log level:" << logInfo->level
        << " path:" << logInfo->path
        << " name:" << logInfo->name 
        << std::endl;
    
    FileLogPtr log = sFileMgr->GetFileLog(logInfo->path + logInfo->name);
    if(!log) {
        std::cerr << "log can't open." << std::endl;
        return -1;
    }
    log->SetRotate(logInfo->rotate_type);
    g_logger = new Logger(log);
    g_logger->SetLogLevel(logInfo->level);

    TaskPtr task1 = std::make_shared<Task>([](const TaskPtr &task){
        sFileMgr->OnCheck();
        task->Restart();
    }, 1000);
    sTaskMgr->Add(task1);
    sMyService->Start();
    while(1) {
        sTaskMgr->OnWork();
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    return 0;
}