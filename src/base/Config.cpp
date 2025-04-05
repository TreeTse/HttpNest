#include <fstream>
#include "Config.h"
#include "LogStream.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

using namespace nest::base;

namespace
{
    static ServiceInfoPtr service_info_nullptr;
}

bool Config::LoadConfig(const std::string &file)
{
    LOG_DEBUG << "load config file: " << file;
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::ifstream in(file);
    std::string err;
    auto res = Json::parseFromStream(reader, in, &root, &err);
    if (!res) {
        LOG_ERROR << "config file:" << file << " parse error,err:" << err;
        return false;
    }
    Json::Value nameObj = root["name"];
    if (!nameObj.isNull()) {
        name_ = nameObj.asString();
    }
    Json::Value cpusObj = root["cpu_start"];
    if (!cpusObj.isNull()) {
        cpuStart_ = cpusObj.asInt();
    }
    Json::Value cpus1Obj = root["cpus"];
    if (!cpus1Obj.isNull()) {
        cpus_ = cpus1Obj.asInt();
    }
    Json::Value threadsObj = root["threads"];
    if (!threadsObj.isNull()) {
        threadNums_ = threadsObj.asInt();
    }
    Json::Value logObj = root["log"];
    if (!logObj.isNull()) {
        ParseLogInfo(logObj);
    }
    if (!ParseServiceInfo(root["services"])) {
        return false;
    }
    
    return true;
}

LogInfoPtr& Config::GetLogInfo()
{
    return logInfo_;
}

bool Config::ParseLogInfo(const Json::Value &root)
{
    logInfo_ = std::make_shared<LogInfo>();
    Json::Value levelObj = root["level"];
    if (!levelObj.isNull()) {
        std::string level = levelObj.asString();
        if (level == "TRACE") {
            logInfo_->level = kTrace;
        } else if (level == "DEBUG") {
            logInfo_->level = kDebug;
        } else if (level == "INFO") {
            logInfo_->level = kInfo;
        } else if (level == "WARN") {
            logInfo_->level = kWarn;
        } else if (level == "ERROR") {
            logInfo_->level = kError;
        }
    }
    Json::Value pathObj = root["path"];
    if(!pathObj.isNull()) {
        logInfo_->path = pathObj.asString();
    }
    Json::Value nameObj = root["name"];
    if(!nameObj.isNull()) {
        logInfo_->name = nameObj.asString();
    }   
    Json::Value rtObj = root["rotate"];
    if(!rtObj.isNull()) {
        std::string rt = rtObj.asString();
        if(rt == "DAY") {
            logInfo_->rotate_type = kRotateDay;
        }
        else if(rt == "HOUR") {
            logInfo_->rotate_type = kRotateHour;
        }
    }   
    return true;
}

bool ConfigMgr::LoadConfig(const std::string &file)
{
    ConfigPtr config = std::make_shared<Config>();
    if (config->LoadConfig(file)) {
        std::lock_guard<std::mutex> lk(mtx_);
        config_ = config;
        return true;
    }
    return false;
}

ConfigPtr ConfigMgr::GetConfig()
{
    std::lock_guard<std::mutex> lk(mtx_);
    return config_;
}

const std::vector<ServiceInfoPtr> &Config::GetServiceInfos()
{
    return services_;
}

const ServiceInfoPtr &Config::GetServiceInfo(const std::string &protocol, const std::string &transport)
{
    for (auto &s:services_) {
        if (s->protocol == protocol && s->transport == transport) {
            return s;
        }
    }
    return service_info_nullptr;
}

bool Config::ParseServiceInfo(const Json::Value &serviceObj)
{
    if (serviceObj.isNull()) {
        LOG_ERROR << " config no service section!";
        return false;
    }
    if(!serviceObj.isArray()) {
        LOG_ERROR << " service section type is not array!";
        return false;
    }
    for(auto const &s:serviceObj) {
        ServiceInfoPtr sinfo = std::make_shared<ServiceInfo>();

        sinfo->addr = s.get("addr", "0.0.0.0").asString();
        sinfo->port = s.get("port", "0").asInt();
        sinfo->protocol = s.get("protocol", "rtmp").asString();
        sinfo->transport = s.get("transport", "tcp").asString();

        LOG_INFO << "service info addr:" << sinfo->addr
                 << " port:" << sinfo->port
                 << " protocol:" << sinfo->protocol
                 << " transport:" << sinfo->transport;
        services_.emplace_back(sinfo);
    }
    return true;
}