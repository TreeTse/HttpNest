#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <sstream>
#include "HttpTypes.h"

namespace nest
{
    namespace mm
    {
        class HttpRequest
        {
        public:
            explicit HttpRequest(bool isRequest = true);
            const std::unordered_map<std::string, std::string> &Headers() const;
            void AddHeader(const std::string &field, const std::string &value);
            void AddHeader(std::string &&field, std::string &&value); 
            void RemoveHeader(const std::string &key);
            const std::string &GetHeader(const std::string &field) const;        
            std::string MakeHeaders();
            void SetQuery(const std::string &query);
            void SetQuery(std::string &&query);
            void SetParameter(const std::string &key, const std::string &value);
            void SetParameter(std::string &&key, std::string &&value);
            const std::string &GetParameter(const std::string &key) const;
            const std::string &Query() const ;

            void SetMethod(const std::string &method);
            void SetMethod(HttpMethod method);
            HttpMethod Method() const;
            void SetVersion(Version v);
            void SetVersion(const std::string &version);
            Version GetVersion() const;
            void SetPath(const std::string &path);
            const std::string &Path() const;
            void SetStatusCode(int32_t code);
            uint32_t GetStatusCode() const;
            void SetBody(const std::string &body);
            void SetBody(std::string &&body);
            const std::string &Body() const;
            std::string AppendToBuffer();
            bool IsRequest() const;
            bool IsStream() const;
            bool IsChunked() const;
            void SetIsStream(bool s);
            void SetIsChunked(bool c);

        private:
            void AppendRequestFirstLine(std::stringstream &ss);
            void AppendResponseFirstLine(std::stringstream &ss);

            void ParseParameters();

            HttpMethod method_{kInvalid};
            Version version_{Version::kUnknown};
            std::string path_;
            std::string query_;
            std::unordered_map<std::string, std::string> headers_;
            std::unordered_map<std::string, std::string> parameters_;
            std::string body_;
            uint32_t code_{0};
            bool isRequest_{true};
            bool isStream_{false};
            bool isChunked_{false};            
        };
    }
}