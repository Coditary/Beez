#pragma once

#include "beez/plugin/lua/api/net/detail/headers.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

// NOLINTBEGIN(misc-include-cleaner,misc-non-private-member-variables-in-classes,cppcoreguidelines-special-member-functions,modernize-use-equals-delete)
namespace beez::plugin::lua::net_detail
{

constexpr long HttpStatusOkMin = 200;
constexpr long HttpStatusOkExclusive = 300;
constexpr long HttpStatusClientErrorExclusive = 400;
constexpr long DefaultRequestTimeoutSeconds = 30;
constexpr long DefaultDownloadTimeoutSeconds = 120;
constexpr long DefaultPingTimeoutSeconds = 10;
constexpr long DefaultOnlineCheckTimeoutSeconds = 3;

struct HttpResponse
{
    long statusCode = 0;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::string error;

    [[nodiscard]] bool ok() const
    {
        return statusCode >= HttpStatusOkMin && statusCode < HttpStatusOkExclusive;
    }
};

struct RequestOptions
{
    std::string method = "GET";
    std::string url;
    std::string body;
    HeaderList headers;
    long timeoutSeconds = DefaultRequestTimeoutSeconds;
    bool followRedirects = true;
};

struct DownloadOptions
{
    HeaderList headers;
    long timeoutSeconds = DefaultDownloadTimeoutSeconds;
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
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    [[nodiscard]] static HttpClient& instance();

    void setProxy(std::string proxyUrl);
    void clearProxy();

    [[nodiscard]] HttpResponse perform(const RequestOptions& options);
    [[nodiscard]] HttpResponse uploadFile(const std::string& url,
                                          const std::filesystem::path& filePath,
                                          const HeaderList& headers = {});
    [[nodiscard]] std::uintmax_t download(const std::string& url,
                                          const std::filesystem::path& destination,
                                          const DownloadOptions& options = {});

    [[nodiscard]] PingResult ping(const std::string& url,
                                  long timeoutSeconds = DefaultPingTimeoutSeconds);
    [[nodiscard]] bool isOnline(long timeoutSeconds = DefaultOnlineCheckTimeoutSeconds);

  private:
    HttpClient();
    ~HttpClient();

    std::string proxyUrl_;
};

}  // namespace beez::plugin::lua::net_detail
// NOLINTEND(misc-include-cleaner,misc-non-private-member-variables-in-classes,cppcoreguidelines-special-member-functions,modernize-use-equals-delete)
