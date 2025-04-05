#include "HttpClient.h"
#include "http/base/MMediaLog.h"
#include "http/HttpContext.h"
#include "network/DnsService.h"

using namespace nest::mm;
using namespace nest::net;

HttpClient::HttpClient(EventLoop *loop, HttpHandler *handler)
: loop_(loop), handler_(handler)
{}

HttpClient::~HttpClient()
{}

void HttpClient::SetCloseCallback(const CloseConnectionCallback &cb)
{
    closeCb_ = cb;
}

void HttpClient::SetCloseCallback(CloseConnectionCallback &&cb)
{
    closeCb_ = std::move(cb);
}

void HttpClient::Get(const std::string &url)
{
    isPost_ = false;
    url_ = url;
    CreateTcpClient();
}

void HttpClient::Post(const std::string &url, const PacketPtr &packet)
{
    isPost_ = true;
    url_ = url;
    outPacket_ = packet;
    CreateTcpClient();
}

void HttpClient::OnWriteComplete(const TcpConnectionPtr &conn)
{
    auto context = conn->GetContext<HttpContext>(kHttpContext);
    if(context) {
        context->WriteComplete(conn);
    }
}

void HttpClient::OnConnection(const TcpConnectionPtr& conn, bool connected)
{
    if(connected) {
        auto context = std::make_shared<HttpContext>(conn, handler_);
        conn->SetContext(kHttpContext, context);
        if(isPost_) {
            context->PostRequest(request_->MakeHeaders(), outPacket_);
        } else {
            context->PostRequest(request_->MakeHeaders());
        }
    }
}

void HttpClient::OnMessage(const TcpConnectionPtr& conn, MsgBuffer &buf)
{
    auto context = conn->GetContext<HttpContext>(kHttpContext);
    if(context) {
        auto ret = context->Parse(buf);
        if(ret == -1) {
            HTTP_DEBUG << "message parse error.";
            conn->ForceClose();
        }
    }
}

bool HttpClient::ParseUrl(const std::string &url)
{
    if(url.size() > 7) {//http://domain
        uint16_t port = 80;
        auto pos = url.find_first_of("/", 7);
        if(pos != std::string::npos) {
            const std::string &path = url.substr(pos);
            auto pos1 = path.find_first_of("?");
            if(pos1 != std::string::npos) {
                request_->SetPath(path.substr(0, pos1));
                request_->SetQuery(path.substr(pos1 + 1));
            } else {
                request_->SetPath(path);
            }
            std::string domain = url.substr(7, pos - 7);
            request_->AddHeader("Host", domain);
            auto pos2 = domain.find_first_of(":");
            if(pos2 != std::string::npos) {
                addr_.SetAddr(domain.substr(0, pos2));
                addr_.SetPort(std::atoi(url.substr(pos2 + 1).c_str()));
            } else {
                addr_.SetAddr(domain);
                addr_.SetPort(port);
            }
        } else {
            request_->SetPath("/");
            std::string domain = url.substr(7);
            request_->AddHeader("Host", domain);
            addr_.SetAddr(domain);
            addr_.SetPort(port);            
        }

        auto list = sDnsService->GetHostAddress(addr_.ip());
        if(list.size() > 0) {
            for(auto const &l:list) {
                if(!l->IsIpV6()) {
                    addr_.SetAddr(l->ip());
                    break;
                }
            }
        } else {
            sDnsService->AddHost(addr_.ip());
            std::vector<InetAddressPtr> list2;
            sDnsService->GetHostInfo(addr_.ip(), list2);
            if(list2.size() > 0) {
                for(auto const &l:list2) {
                    if(!l->IsIpV6()) {
                        addr_.SetAddr(l->ip());
                        break;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

void HttpClient::CreateTcpClient()
{
    request_.reset();
    request_ = std::make_shared<HttpRequest>(true);
    if(isPost_) {
        request_->SetMethod(kPost);
    } else {
        request_->SetMethod(kGet);
    }
    request_->AddHeader("User-Agent", "curl/7.61.1");
    request_->AddHeader("Accept", "*/*");
    auto ret = ParseUrl(url_);
    if(!ret) {
        HTTP_ERROR << "invalid url:" << url_;
        if(closeCb_) {
            closeCb_(nullptr);
        }
        return;
    }
    tcpClient_ = std::make_shared<TcpClient>(loop_, addr_);
    tcpClient_->SetWriteCompleteCallback(std::bind(&HttpClient::OnWriteComplete, this, std::placeholders::_1));
    tcpClient_->SetRecvMsgCallback(std::bind(&HttpClient::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpClient_->SetCloseCallback(closeCb_);
    tcpClient_->SetConnectCallback(std::bind(&HttpClient::OnConnection, this, std::placeholders::_1, std::placeholders::_2));
    tcpClient_->Connect();
}