#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include "webserver.h"

using namespace std;

class Config
{
public:
    Config();
    ~Config(){};
    void ParseArg(int argc, char*argv[]);

    string ip_;
    int port_;
    int optLinger_;
    int sqlNum_;
    int threadNum_;
    int isCloseLog_;
};

#endif