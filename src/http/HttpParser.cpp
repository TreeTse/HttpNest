#include "HttpParser.h"
#include "base/StringUtils.h"
#include "http/base/MMediaLog.h"
#include <algorithm>

using namespace nest::mm;

static std::string CRLFCRLF = "\r\n\r\n";
static int32_t kHttpMaxBodySize = 64*1024;

namespace
{
    static std::string string_empty;
}

HttpParserState HttpParser::Parse(MsgBuffer &buf)
{
    if(buf.ReadableBytes() == 0) {
        return state_;
    }

    switch (state_) {
        case kExpectHeaders: {
            if(buf.ReadableBytes() > CRLFCRLF.size()) {
                auto *space = std::search(buf.Peek(), (const char *)buf.BeginWrite(), CRLFCRLF.data(), CRLFCRLF.data() + CRLFCRLF.size());
                if(space != (const char *)buf.BeginWrite()) { //CRLFCRLF found
                    auto size = space - buf.Peek();
                    header_.assign(buf.Peek(), size);
                    buf.Retrieve(size + 4);
                    ParseHeaders();
                    if(state_ == kExpectHttpComplete || state_ == kExpectError) {
                        return state_;
                    }
                } else {
                    if(buf.ReadableBytes() > kHttpMaxBodySize) {
                        reason_ = k400BadRequest;
                        state_ = kExpectError;
                        return state_;
                    }
                    return kExpectContinue;
                }
            } else {
                return kExpectContinue;
            }
            break;
        }
        case kExpectNormalBody: {
            ParseNormalBody(buf);
            break;
        }
        case kExpectStreamBody: {
            ParseStream(buf);
            break;
        }
        case kExpectChunkLen: {
            auto crlf = buf.FindCRLF();
            if(crlf) {
                std::string len(buf.Peek(), crlf);
                char *end;
                curChunkLength_ = std::strtol(len.c_str(), &end, 16);
                HTTP_DEBUG << " chunk len:" << curChunkLength_;
                if(curChunkLength_ > 1024*1024) {
                    HTTP_ERROR << "error chunk len.";
                    state_ = kExpectError;
                    reason_ = k400BadRequest;
                }
                buf.RetrieveUntil(crlf + 2);
                if(curChunkLength_ == 0) {
                    state_ = kExpectLastEmptyChunk;
                } else {
                    state_ = kExpectChunkBody;
                }
            } else {
                if(buf.ReadableBytes() > 32) {
                    buf.RetrieveAll();
                    reason_ = k400BadRequest;
                    state_ = kExpectError;
                    return state_;
                }
            }
            break;
        }
        case kExpectChunkBody: {
            ParseChunk(buf);
            if(state_ == kExpectChunkComplete) {
                return state_;
            }
            break;
        }
        case kExpectLastEmptyChunk: {
            auto crlf = buf.FindCRLF();
            if(crlf) {
                buf.RetrieveUntil(crlf + 2);
                chunk_.reset();
                state_ = kExpectChunkComplete;
                break;
            }
        }
        default:
            break;
    }
    return state_;
}

const PacketPtr &HttpParser::Chunk() const
{
    return chunk_;
}

HttpStatusCode HttpParser::Reason() const
{
    return reason_;
}

void HttpParser::ClearForNextHttp()
{
    state_ = kExpectHeaders;
    header_.clear();
    req_.reset();
    curContentLength_ = -1;
    chunk_.reset();
}

void HttpParser::ClearForNextChunk()
{
    if(isChunked_) {
        state_ = kExpectChunkLen;
        curChunkLength_ = -1;
    } else {
        if(isStream_) {
            state_ = kExpectStreamBody;
        } else {
            state_ = kExpectHeaders;
            curChunkLength_ = -1;
        }
    }
    chunk_.reset();
}

void HttpParser::ParseStream(MsgBuffer &buf)
{
    if(!chunk_) {
        chunk_ = Packet::NewPacket(kHttpMaxBodySize);
    }
    auto size = std::min((int)buf.ReadableBytes(), chunk_->Space());
    memcpy(chunk_->Data() + chunk_->PacketSize(), buf.Peek(), size);
    chunk_->UpdatePacketSize(size);
    buf.Retrieve(size);

    if(chunk_->Space() == 0) {
        state_ = kExpectChunkComplete;
    }
}

void HttpParser::ParseNormalBody(MsgBuffer &buf)
{
    if(!chunk_) {
        chunk_ = Packet::NewPacket(curContentLength_);
    }
    auto size = std::min((int)buf.ReadableBytes(), chunk_->Space());
    memcpy(chunk_->Data() + chunk_->PacketSize(), buf.Peek(), size);
    chunk_->UpdatePacketSize(size);
    buf.Retrieve(size);
    curContentLength_ -= size;
    if(curContentLength_ == 0) {
        state_ = kExpectHttpComplete;
    }
}

void HttpParser::ParseChunk(MsgBuffer &buf)
{
    if(!chunk_) {
        chunk_ = Packet::NewPacket(curChunkLength_);
    }
    auto size = std::min((int)buf.ReadableBytes(), chunk_->Space());
    memcpy(chunk_->Data() + chunk_->PacketSize(), buf.Peek(), size);
    chunk_->UpdatePacketSize(size);
    buf.Retrieve(size);
    curChunkLength_ -= size;
    if(curChunkLength_ == 0 || chunk_->Space() == 0) {
        state_ = kExpectChunkComplete;
    }
}

void HttpParser::ParseHeaders()
{
    auto list = base::StringUtils::SplitString(header_, "\r\n");
    if(list.size() < 1) {
        reason_ = k400BadRequest;
        state_ = kExpectError;
        return;
    }
    ProcessMethodLine(list[0]);

    for(auto &l:list) {
        auto pos = l.find_first_of(':');
        if(pos != std::string::npos) {
            std::string k = l.substr(0, pos);
            std::string v = l.substr(pos + 1);
            
            HTTP_DEBUG << "parse header k:" << k << " v:" << v;
            req_->AddHeader(std::move(k), std::move(v));
        }
    }
    auto len = req_->GetHeader("content-length");
    if(!len.empty()) {
        HTTP_TRACE << "content-length:" << len;
        try {
            curContentLength_ = std::stoull(len);
        } catch(...) {
            reason_ = k400BadRequest;
            state_ = kExpectError;
            return;
        }

        if(curContentLength_ == 0) {
            state_ = kExpectHttpComplete;
        } else {
            state_ = kExpectNormalBody;
        }
    } else {
        const std::string &chunk = req_->GetHeader("transfer-encoding");
        if(!chunk.empty() && chunk == "chunked") {
            isChunked_ = true;
            req_->SetIsChunked(true);
            state_ = kExpectChunkLen;
        } else {
            if((!isRequest_ && req_->GetStatusCode() != 200)
                || (isRequest_
                    && (req_->Method() == kGet
                        || req_->Method() == kHead 
                        || req_->Method() == kOptions))) {
                curChunkLength_ = 0;
                state_ = kExpectHttpComplete;
            } else {
                curContentLength_ = -1;
                isStream_ = true;
                req_->SetIsStream(true);
                state_ = kExpectStreamBody;
            }
        }
    }
}

void HttpParser::ProcessMethodLine(const std::string &line)
{
    HTTP_DEBUG << "parse method line:" << line;
    auto list = base::StringUtils::SplitString(line, " ");
    if(list.size() != 3) {
        reason_ = k400BadRequest;
        state_ = kExpectError;
        return;
    }
    std::string str = list[0];
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    if(str[0] == 'h' && str[1] == 't' && str[2] == 't' && str[3] == 'p') {
        isRequest_ = false;
    } else {
        isRequest_ = true;
    }
    if(req_) {
        req_.reset();
    }
    req_ = std::make_shared<HttpRequest>(isRequest_);
    if(isRequest_) {
        req_->SetMethod(list[0]);
        const std::string &path = list[1];
        auto pos = path.find_first_of("?");
        if(pos != std::string::npos) {
            req_->SetPath(path.substr(0, pos));
            req_->SetQuery(path.substr(pos + 1));
        } else {
            req_->SetPath(path);
        }
        req_->SetVersion(list[2]);
        HTTP_DEBUG << "http method:" << list[0] 
                << " path:" << req_->Path() 
                << " query:" << req_->Query() 
                << " version:" << list[2];
    } else {
        req_->SetVersion(list[0]);
        req_->SetStatusCode(std::atoi(list[1].c_str()));
        HTTP_DEBUG << "version:" << list[0]
                   << " http code:" << list[1];        
    }
}