#ifndef CONFIG_H
#define CONFIG_H

#include "webserver.h"

using namespace std;

class Config
{
public:
    Config();
    ~Config(){};
    void ParseArg(int argc, char*argv[]);

    int port_;
    int isAsyncWrite_;
    int trigMode_;
    int listenTrigMode_;
    int connTrigMode_;
    int optLinger_;
    int sqlNum_;
    int threadNum_;
    int isCloseLog_;
    int actorModel_;
};

#endif