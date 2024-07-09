#include "config.h"

Config::Config()
{
    ip_ = "127.0.0.1";

    //default 9002
    port_ = 9002;

    //default enable
    optLinger_ = 1;

    //default 8
    sqlNum_ = 8;

    //default 8
    threadNum_ = 8;

    //default close
    isCloseLog_ = 1;
}

void Config::ParseArg(int argc, char*argv[])
{
    int opt;
    const char *str = "p:o:s:t:c:a:i:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 'p': {
                port_ = atoi(optarg);
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
            case 'i': {
                ip_ = optarg;
                break;
            }
            default:
                break;
        }
    }
}
