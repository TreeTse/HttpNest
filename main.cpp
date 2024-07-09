#include <signal.h>
#include "config.h"

WebServer *webserver;

void Stop(int sig)
{
    LOG_DEBUG("sig=%d\n", sig);
    webserver->Stop();
    delete webserver;
    LOG_DEBUG("webserver stopped.\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    //command line
    Config config;
    config.ParseArg(argc, argv);

    if (!config.isCloseLog_) {
        Log::GetInstance()->Init(0, "./mylog", ".log", 80000);
    }

    signal(SIGTERM, Stop);
    signal(SIGINT, Stop);

    webserver = new WebServer(config.ip_, config.port_, 10000, config.optLinger_, "root", "xsh", "auroradb",
                     config.sqlNum_, 6, 0);

    webserver->Start();

    return 0;
}
