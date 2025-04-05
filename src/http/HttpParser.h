#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <cstdint>
#include "HttpTypes.h"
#include "http/base/Packet.h"
#include "network/base/MsgBuffer.h"
#include "HttpRequest.h"

namespace nest
{
    namespace mm
    {
        using namespace nest::net;

        enum HttpParserState
        {
            kExpectHeaders,
            kExpectNormalBody,
            kExpectStreamBody,
            kExpectHttpComplete,
            kExpectChunkLen,
            kExpectChunkBody,
            kExpectChunkComplete,
            kExpectLastEmptyChunk,

            kExpectContinue,
            kExpectError,
        };
        using HttpRequestPtr = std::shared_ptr<HttpRequest>;

        class HttpParser
        {
        public:
            HttpParser() = default;
            ~HttpParser() = default;

            HttpParserState Parse(MsgBuffer &buf);
            const PacketPtr &Chunk() const;
            HttpStatusCode Reason() const;
            void ClearForNextHttp();
            void ClearForNextChunk();
            HttpRequestPtr GetHttpRequest() const 
            {
                return req_;
            }
        private:
            void ParseStream(MsgBuffer &buf);
            void ParseNormalBody(MsgBuffer &buf);
            void ParseChunk(MsgBuffer &buf);
            void ParseHeaders();
            void ProcessMethodLine(const std::string &line);

            HttpParserState state_{kExpectHeaders};
            int32_t curChunkLength_{0};
            int32_t curContentLength_{0};
            bool isStream_{false};
            bool isChunked_{false};
            bool isRequest_{true};
            HttpStatusCode reason_{kUnknown};
            std::string header_;
            PacketPtr chunk_;
            HttpRequestPtr req_;
        };
    }
}