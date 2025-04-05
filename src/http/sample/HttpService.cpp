#include "HttpService.h"
#include "base/StringUtils.h"
#include "base/Config.h"
#include "base/Time.h"
#include "http/HttpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpUtils.h"
#include "http/HttpContext.h"
#include "http/base/MMediaLog.h"

using namespace nest::mm;

void HttpService::OnNewConnection(const TcpConnectionPtr &conn)
{}

void HttpService::OnConnectionDestroy(const TcpConnectionPtr &conn)
{}

void HttpService::OnActive(const ConnectionPtr &conn)
{}

void HttpService::OnRecv(const TcpConnectionPtr &conn, PacketPtr &&data)
{}

void HttpService::OnSent(const TcpConnectionPtr &conn)
{}

bool HttpService::OnSentNextChunk(const TcpConnectionPtr &conn)
{
    return false;
}

void HttpService::OnRequest(const TcpConnectionPtr &conn, const HttpRequestPtr &req, const PacketPtr &packet)
{
    if (req->IsRequest()) {
        HTTP_DEBUG << "req method:" << req->Method() << " path:" << req->Path();
    } else {
        HTTP_DEBUG << "req code:" << req->GetStatusCode() << " msg:" << HttpUtils::ParseStatusMessage(req->GetStatusCode());
    }
    auto headers = req->Headers();
    for(auto const &h:headers) {
        HTTP_DEBUG << h.first << ":" << h.second;
    }
    if (req->IsRequest()) {
        HttpRequestPtr res = std::make_shared<HttpRequest>(false);
        res->SetStatusCode(200);
        res->AddHeader("server", "nest");
        res->AddHeader("content-length", "0");

        auto cxt = conn->GetContext<HttpContext>(kHttpContext);
        if(cxt) {
            res->SetIsStream(false);
            cxt->PostRequest(res);
        }
    }
}

void HttpService::Start()
{
    ConfigPtr config = sConfigMgr->GetConfig();
    pool_ = new EventLoopThreadPool(config->threadNums_, config->cpuStart_, config->cpus_);
    pool_->Start();

    auto services = config->GetServiceInfos();
    auto eventloops = pool_->GetLoops();
    HTTP_TRACE << "eventloops size: " << eventloops.size();
    for (auto &el:eventloops) {
        for (auto &s:services) {
            if (s->protocol == "HTTP" || s->protocol == "http") {
                InetAddress local(s->addr, s->port);
                TcpServer *server = new HttpServer(el, local, this);
                servers_.push_back(server);
                servers_.back()->Start();
            }
        }
    }
}

void HttpService::Stop()
{}
