#pragma once

#include "beez/plugin/lua/api/net/detail/headers.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace beez::plugin::lua::net_detail
{

struct HttpResponse
{
    long statusCode = 0;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::string error;

    [[nodiscard]] bool ok() const
    {
        return statusCode >= 200 && statusCode < 300;
    }
};

struct RequestOptions
{
    std::string method = "GET";
    std::string url;
    std::string body;
    HeaderList headers;
    long timeoutSeconds = 30;
    bool followRedirects = true;
};

struct DownloadOptions
{
    HeaderList headers;
    long timeoutSeconds = 120;
    bool followRedirects = true;
};

struct PingResult
{
    bool reachable = false;
    long statusCode = 0;
    double milliseconds = 0.0;
    std::string error;
};

class HttpClient
{
  public:
    [[nodiscard]] static HttpClient& instance();

    void setProxy(std::string proxyUrl);
    void clearProxy();

    [[nodiscard]] HttpResponse perform(RequestOptions options);
    [[nodiscard]] HttpResponse uploadFile(const std::string& url,
                                          const std::filesystem::path& filePath,
                                          const HeaderList& headers = {});
    [[nodiscard]] std::uintmax_t download(const std::string& url,
                                          const std::filesystem::path& destination,
                                          DownloadOptions options = {});

    [[nodiscard]] PingResult ping(const std::string& url, long timeoutSeconds = 10);
    [[nodiscard]] bool isOnline(long timeoutSeconds = 3);

  private:
    HttpClient();
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    std::string proxyUrl_;
};

}  // namespace beez::plugin::lua::net_detail
