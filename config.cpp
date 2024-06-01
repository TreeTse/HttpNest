#include "config.h"

Config::Config()
{
    //default 9002
    port_ = 9002;

    //default sync
    isAsyncWrite_ = 0;

    //default listenfd LT + connfd LT
    trigMode_ = 0;

    //listenfd default ET
    listenTrigMode_ = 1;

    //connfd default ET
    connTrigMode_ = 1;

    //default enable
    optLinger_ = 1;

    //default 8
    sqlNum_ = 8;

    //default 8
    threadNum_ = 8;

    //default close
    isCloseLog_ = 1;

    //default proactor(0)
    actorModel_ = 0;
}

void Config::ParseArg(int argc, char*argv[])
{
    int opt;
    const char *str = "p:l:m:o:s:t:c:a:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 'p': {
                port_ = atoi(optarg);
                break;
            }
            case 'l': {
                isAsyncWrite_ = atoi(optarg);
                break;
            }
            case 'm': {
                trigMode_ = atoi(optarg);
                break;
            }
            case 'o': {
                optLinger_ = atoi(optarg);
                break;
            }
            case 's': {
                sqlNum_ = atoi(optarg);
                break;
            }
            case 't': {
                threadNum_ = atoi(optarg);
                break;
            }
            case 'c': {
                isCloseLog_ = atoi(optarg);
                break;
            }
            case 'a': {
                actorModel_ = atoi(optarg);
                break;
            }
            default:
                break;
        }
    }
}
