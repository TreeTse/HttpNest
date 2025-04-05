#include "HttpContext.h"
#include "http/base/MMediaLog.h"

using namespace nest::mm;

namespace
{
    static std::string CHUNK_EOF = "0\r\n\r\n";
}

HttpContext::HttpContext(const TcpConnectionPtr &conn, HttpHandler *handler)
: connection_(conn), handler_(handler)
{}

int32_t HttpContext::Parse(MsgBuffer &buf)
{
    while (buf.ReadableBytes() > 1) {
        auto state = httpParser_.Parse(buf);
        if (state == kExpectHttpComplete || state == kExpectChunkComplete) {
            if (handler_) {
                handler_->OnRequest(connection_, httpParser_.GetHttpRequest(), httpParser_.Chunk());
            }
        } else if(state == kExpectError) {
            connection_->ForceClose();
        }
    }
    return 1;
}

bool HttpContext::PostRequest(HttpRequestPtr &request)
{
    if(request->IsChunked()) {
        PostChunkHeader(request->MakeHeaders());
    } else if(request->IsStream()) {
        PostStreamHeader(request->MakeHeaders());
    } else {
        PostRequest(request->AppendToBuffer());
    }
    return true;
}

bool HttpContext::PostRequest(const std::string &header_and_body)
{
    if (postState_ != kHttpContextPostInit) {
        return false;
    }
    header_ = header_and_body;
    postState_ = kHttpContextPostHttp;
    connection_->Send(header_.c_str(), header_.size());
    return true;
}

bool HttpContext::PostStreamHeader(const std::string &header)
{
    if(postState_ != kHttpContextPostInit) {
        return false;
    }
    header_ = header;
    postState_ = kHttpContextPostInit;
    connection_->Send(header_.c_str(), header_.size());
    headerSent_ = true;
    return true;
}

bool HttpContext::PostChunkHeader(const std::string &header)
{
    if(postState_ != kHttpContextPostInit) {
        return false;
    }
    header_ = header;
    postState_ = kHttpContextPostInit;
    connection_->Send(header_.c_str(), header_.size());
    headerSent_ = true;
    return true;
}

bool HttpContext::PostRequest(const std::string &header, PacketPtr &packet)
{
    if(postState_ != kHttpContextPostInit) {
        return false;
    }

    header_ = header;
    outPakcet_ = packet;
    postState_ = kHttpContextPostHttpHeader;
    connection_->Send(header_.c_str(), header_.size());
    return true;
}

void HttpContext::PostChunk(PacketPtr &chunk)
{
    outPakcet_ = chunk;
    if(!headerSent_) {
        postState_ = kHttpContextPostChunkHeader;
        connection_->Send(header_.c_str(), header_.size());
        headerSent_ = true;
    } else {
        postState_ = kHttpContextPostChunkLen;
        char buf[32] ={0,};
        sprintf(buf, "%X\r\n", outPakcet_->PacketSize());
        header_ = std::string(buf);
        connection_->Send(header_.c_str(), header_.size());
    }
}

void HttpContext::PostEofChunk()
{
    postState_ = kHttpContextPostChunkEOF;
    connection_->Send(CHUNK_EOF.c_str(), CHUNK_EOF.size());
}

bool HttpContext::PostStreamChunk(PacketPtr &packet)
{
    if (postState_ == kHttpContextPostInit) {
        outPakcet_ = packet;
        if(!headerSent_) {
            postState_ = kHttpContextPostHttpStreamHeader;
            connection_->Send(header_.c_str(), header_.size());
            headerSent_ = true;
        } else {
            postState_ = kHttpContextPostHttpStreamChunk;
            connection_->Send(outPakcet_->Data(), outPakcet_->PacketSize());
        }
        return true;
    }
    return false;
}

void HttpContext::WriteComplete(const TcpConnectionPtr &conn)
{
    switch(postState_) {
        case kHttpContextPostInit: {
            break;
        }
        case kHttpContextPostHttp: {
            postState_ = kHttpContextPostInit;
            handler_->OnSent(conn);
            break;
        }
        case kHttpContextPostHttpHeader: {
            postState_ = kHttpContextPostHttpBody;
            connection_->Send(outPakcet_->Data(), outPakcet_->PacketSize());
            break;
        }
        case kHttpContextPostHttpBody: {
            postState_ = kHttpContextPostInit;
            handler_->OnSent(conn);
            break;
        }
        case kHttpContextPostChunkHeader: {
            postState_ = kHttpContextPostChunkLen;
            handler_->OnSentNextChunk(conn);
            break;
        }
        case kHttpContextPostChunkLen: {
            postState_ = kHttpContextPostChunkBody;
            connection_->Send(outPakcet_->Data(), outPakcet_->PacketSize());
            break;
        }
        case kHttpContextPostChunkBody: {
            postState_ = kHttpContextPostInit;
            handler_->OnSentNextChunk(conn);
            break;
        }
        case kHttpContextPostChunkEOF: {
            postState_ = kHttpContextPostInit;
            handler_->OnSent(conn);
            break;
        }
        case kHttpContextPostHttpStreamHeader: {
            postState_ = kHttpContextPostInit;
            handler_->OnSentNextChunk(conn);
            break;
        }
        case kHttpContextPostHttpStreamChunk: {
            postState_ = kHttpContextPostInit;
            handler_->OnSentNextChunk(conn);
            break;
        }
    }
}