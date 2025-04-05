#pragma once

#include "HttpParser.h"
#include "HttpRequest.h"
#include "http/base/Packet.h"
#include "HttpHandler.h"
#include <string>

namespace nest
{
    namespace mm
    {
        using namespace nest::net;
        enum HttpContextPostState
        {
            kHttpContextPostInit,
            kHttpContextPostHttp,
            kHttpContextPostHttpHeader,
            kHttpContextPostHttpBody,
            kHttpContextPostHttpStreamHeader,
            kHttpContextPostHttpStreamChunk,
            kHttpContextPostChunkHeader,
            kHttpContextPostChunkLen,
            kHttpContextPostChunkBody,
            kHttpContextPostChunkEOF
        };

        class HttpContext
        {
        public:
            HttpContext(const TcpConnectionPtr &conn, HttpHandler *handler);
            ~HttpContext() = default;

            int32_t Parse(MsgBuffer &buf);
            bool PostRequest(HttpRequestPtr &request);
            bool PostRequest(const std::string &header_and_body);
            bool PostStreamHeader(const std::string &header);
            bool PostChunkHeader(const std::string &header);
            bool PostRequest(const std::string &header, PacketPtr &packet);
            void PostChunk(PacketPtr &chunk);
            void PostEofChunk();
            bool PostStreamChunk(PacketPtr &packet);
            void WriteComplete(const TcpConnectionPtr &);

        private:
            TcpConnectionPtr connection_;
            HttpParser httpParser_;
            std::string header_;
            PacketPtr outPakcet_;
            HttpContextPostState postState_{kHttpContextPostInit};
            bool headerSent_;
            HttpHandler *handler_{nullptr};
        };
    }
}