#include "beez/plugin/lua/api/net/detail/http_client.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace beez::plugin::lua::net_detail
{

namespace
{

class CurlGlobalInit
{
  public:
    CurlGlobalInit()
    {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
        {
            throw std::runtime_error("failed to initialize libcurl");
        }
    }

    ~CurlGlobalInit()
    {
        curl_global_cleanup();
    }
};

[[nodiscard]] CurlGlobalInit& curlGlobalInit()
{
    static CurlGlobalInit Instance;
    return Instance;
}

[[nodiscard]] std::string toLower(std::string value)
{
    std::ranges::transform(value,
                           value.begin(),
                           [](const unsigned char Character)
                           { return static_cast<char>(std::tolower(Character)); });
    return value;
}

[[nodiscard]] std::size_t
appendToString(char* buffer, std::size_t size, std::size_t count, void* userData)
{
    auto* output = static_cast<std::string*>(userData);
    if (size > 0)
    {
        output->append(buffer, count);
    }
    return count;
}

[[nodiscard]] std::size_t
appendHeader(char* buffer, std::size_t size, std::size_t count, void* userData)
{
    auto* headers = static_cast<std::unordered_map<std::string, std::string>*>(userData);
    if (size == 0 || count == 0)
    {
        return count;
    }

    std::string line(buffer, count);
    const std::size_t Separator = line.find(':');
    if (Separator == std::string::npos)
    {
        return count;
    }

    std::string name = toLower(line.substr(0, Separator));
    std::string value = line.substr(Separator + 1);
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
    {
        value.erase(value.begin());
    }
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n'))
    {
        value.pop_back();
    }

    if (!name.empty())
    {
        (*headers)[name] = value;
    }

    return count;
}

struct CurlHandleDeleter
{
    void operator()(CURL* handle) const
    {
        if (handle != nullptr)
        {
            curl_easy_cleanup(handle);
        }
    }
};

using CurlHandle = std::unique_ptr<CURL, CurlHandleDeleter>;

[[nodiscard]] CurlHandle makeCurlHandle()
{
    CurlHandle handle(curl_easy_init());
    if (!handle)
    {
        throw std::runtime_error("failed to create curl handle");
    }
    return handle;
}

void applyCommonOptions(CURL* handle,
                        const std::string& url,
                        const HeaderList& headers,
                        const long TimeoutSeconds,
                        const bool FollowRedirects,
                        const std::string& proxyUrl)
{
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, TimeoutSeconds);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, TimeoutSeconds);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, FollowRedirects ? 1L : 0L);
    curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);

    if (!proxyUrl.empty())
    {
        curl_easy_setopt(handle, CURLOPT_PROXY, proxyUrl.c_str());
    }

    struct curl_slist* headerList = nullptr;
    for (const auto& [name, value] : headers)
    {
        const std::string Line = name + ": " + value;
        headerList = curl_slist_append(headerList, Line.c_str());
    }

    if (headerList != nullptr)
    {
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerList);
    }

    curl_easy_setopt(handle, CURLOPT_PRIVATE, headerList);
}

void freeHeaderList(CURL* handle)
{
    void* privateData = nullptr;
    if (curl_easy_getinfo(handle, CURLINFO_PRIVATE, &privateData) == CURLE_OK &&
        privateData != nullptr)
    {
        curl_slist_free_all(static_cast<curl_slist*>(privateData));
        curl_easy_setopt(handle, CURLOPT_PRIVATE, nullptr);
    }
}

}  // namespace

HttpClient& HttpClient::instance()
{
    (void)curlGlobalInit();
    static HttpClient Client;
    return Client;
}

HttpClient::HttpClient() = default;

HttpClient::~HttpClient() = default;

void HttpClient::setProxy(std::string proxyUrl)
{
    proxyUrl_ = std::move(proxyUrl);
}

void HttpClient::clearProxy()
{
    proxyUrl_.clear();
}

HttpResponse HttpClient::perform(RequestOptions options)
{
    if (options.url.empty())
    {
        throw std::invalid_argument("url must not be empty");
    }

    CurlHandle handle = makeCurlHandle();
    applyCommonOptions(handle.get(),
                       options.url,
                       options.headers,
                       options.timeoutSeconds,
                       options.followRedirects,
                       proxyUrl_);

    HttpResponse response;
    curl_easy_setopt(handle.get(), CURLOPT_CUSTOMREQUEST, options.method.c_str());
    curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, appendToString);
    curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(handle.get(), CURLOPT_HEADERFUNCTION, appendHeader);
    curl_easy_setopt(handle.get(), CURLOPT_HEADERDATA, &response.headers);

    if (!options.body.empty())
    {
        curl_easy_setopt(handle.get(), CURLOPT_POSTFIELDS, options.body.c_str());
        curl_easy_setopt(handle.get(),
                         CURLOPT_POSTFIELDSIZE_LARGE,
                         static_cast<curl_off_t>(options.body.size()));
    }

    const CURLcode Result = curl_easy_perform(handle.get());
    if (Result != CURLE_OK)
    {
        response.error = curl_easy_strerror(Result);
    }

    curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &response.statusCode);
    if (response.statusCode == 0 && response.error.empty())
    {
        response.statusCode = 200;
    }
    freeHeaderList(handle.get());
    return response;
}

HttpResponse HttpClient::uploadFile(const std::string& url,
                                    const std::filesystem::path& filePath,
                                    const HeaderList& headers)
{
    if (url.empty())
    {
        throw std::invalid_argument("url must not be empty");
    }
    if (!std::filesystem::is_regular_file(filePath))
    {
        throw std::invalid_argument("upload file does not exist: " + filePath.string());
    }

    CurlHandle handle = makeCurlHandle();
    applyCommonOptions(handle.get(), url, headers, 120, true, proxyUrl_);

    curl_mime* mime = curl_mime_init(handle.get());
    if (mime == nullptr)
    {
        throw std::runtime_error("failed to create upload mime data");
    }

    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, filePath.string().c_str());
    curl_easy_setopt(handle.get(), CURLOPT_MIMEPOST, mime);

    HttpResponse response;
    curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, appendToString);
    curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(handle.get(), CURLOPT_HEADERFUNCTION, appendHeader);
    curl_easy_setopt(handle.get(), CURLOPT_HEADERDATA, &response.headers);

    const CURLcode Result = curl_easy_perform(handle.get());
    if (Result != CURLE_OK)
    {
        response.error = curl_easy_strerror(Result);
    }

    curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &response.statusCode);
    if (response.statusCode == 0 && response.error.empty())
    {
        response.statusCode = 200;
    }
    curl_mime_free(mime);
    freeHeaderList(handle.get());
    return response;
}

std::uintmax_t HttpClient::download(const std::string& url,
                                    const std::filesystem::path& destination,
                                    DownloadOptions options)
{
    if (url.empty())
    {
        throw std::invalid_argument("url must not be empty");
    }

    std::filesystem::create_directories(destination.parent_path());
    std::FILE* file = std::fopen(destination.string().c_str(), "wb");
    if (file == nullptr)
    {
        throw std::runtime_error("failed to open destination file for writing: " +
                                 destination.string());
    }

    CurlHandle handle = makeCurlHandle();
    applyCommonOptions(handle.get(),
                       url,
                       options.headers,
                       options.timeoutSeconds,
                       options.followRedirects,
                       proxyUrl_);
    curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, file);

    const CURLcode Result = curl_easy_perform(handle.get());
    std::fclose(file);
    freeHeaderList(handle.get());

    if (Result != CURLE_OK)
    {
        std::filesystem::remove(destination);
        throw std::runtime_error(std::string("download failed: ") + curl_easy_strerror(Result));
    }

    long statusCode = 0;
    curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &statusCode);
    if (statusCode != 0 && (statusCode < 200 || statusCode >= 300))
    {
        std::filesystem::remove(destination);
        throw std::runtime_error("download failed with HTTP status " + std::to_string(statusCode));
    }

    return std::filesystem::file_size(destination);
}

PingResult HttpClient::ping(const std::string& url, const long timeoutSeconds)
{
    if (url.empty())
    {
        throw std::invalid_argument("url must not be empty");
    }

    const auto Start = std::chrono::steady_clock::now();
    RequestOptions options;
    options.method = "HEAD";
    options.url = url;
    options.timeoutSeconds = timeoutSeconds;
    options.followRedirects = true;
    const HttpResponse Response = perform(options);
    const auto End = std::chrono::steady_clock::now();

    PingResult result;
    result.statusCode = Response.statusCode;
    result.milliseconds = std::chrono::duration<double, std::milli>(End - Start).count();
    result.error = Response.error;
    result.reachable = Response.error.empty() && Response.statusCode > 0;
    return result;
}

bool HttpClient::isOnline(const long timeoutSeconds)
{
    static constexpr const char* Probes[] = {
        "https://cloudflare.com/cdn-cgi/trace",
        "https://www.google.com/generate_204",
    };

    return std::any_of(std::begin(Probes),
                       std::end(Probes),
                       [&](const char* probe)
                       {
                           const PingResult Result = ping(probe, timeoutSeconds);
                           return Result.reachable && Result.statusCode >= 200 &&
                                  Result.statusCode < 400;
                       });
}

}  // namespace beez::plugin::lua::net_detail
