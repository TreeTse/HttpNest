#include "config.h"

int main(int argc, char *argv[])
{
    //database info
    string userName = "root";
    string passwd = "xsh";
    string databasename = "auroradb";

    //command line
    Config config;
    config.ParseArg(argc, argv);

    WebServer server;

    server.Init(config.port_, userName, passwd, databasename, config.isAsyncWrite_, 
                config.optLinger_, config.trigMode_,  config.sqlNum_,  config.threadNum_, 
                config.isCloseLog_, config.actorModel_);
    
    server.InitLogger();

    server.InitSqlPool();

    server.InitThreadPool();

    server.TrigMode();

    server.EventListen();

    server.EventLoop();

    return 0;
}
