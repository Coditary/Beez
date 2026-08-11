#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace beez::test
{

class MockHttpServer
{
  public:
    MockHttpServer()
    {
        listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listenFd_ < 0)
        {
            throw std::runtime_error("failed to create mock http server socket");
        }

        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;

        if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
        {
            ::close(listenFd_);
            throw std::runtime_error("failed to bind mock http server socket");
        }

        socklen_t addressLength = sizeof(address);
        if (::getsockname(listenFd_, reinterpret_cast<sockaddr*>(&address), &addressLength) != 0)
        {
            ::close(listenFd_);
            throw std::runtime_error("failed to read mock http server port");
        }

        port_ = ntohs(address.sin_port);
        if (::listen(listenFd_, 4) != 0)
        {
            ::close(listenFd_);
            throw std::runtime_error("failed to listen on mock http server socket");
        }

        worker_ = std::thread([this]() { serveLoop(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    MockHttpServer(const MockHttpServer&) = delete;
    MockHttpServer& operator=(const MockHttpServer&) = delete;

    ~MockHttpServer()
    {
        stop_.store(true);
        if (listenFd_ >= 0)
        {
            ::shutdown(listenFd_, SHUT_RDWR);
            ::close(listenFd_);
            listenFd_ = -1;
        }
        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    [[nodiscard]] std::uint16_t port() const
    {
        return port_;
    }

    [[nodiscard]] std::string baseUrl() const
    {
        return "http://127.0.0.1:" + std::to_string(port_);
    }

    void setFixedResponse(int statusCode, std::string body, std::string contentType = "text/plain")
    {
        statusCode_ = statusCode;
        body_ = std::move(body);
        contentType_ = std::move(contentType);
    }

    [[nodiscard]] std::string lastMethod() const
    {
        return lastMethod_;
    }

    [[nodiscard]] std::string lastBody() const
    {
        return lastBody_;
    }

  private:
    static constexpr std::size_t ReadBufferSize = 4096U;

    void serveLoop()
    {
        while (!stop_.load())
        {
            const int ClientFd = ::accept(listenFd_, nullptr, nullptr);
            if (ClientFd < 0)
            {
                if (stop_.load())
                {
                    return;
                }
                continue;
            }

            handleClient(ClientFd);
            ::close(ClientFd);
        }
    }

    void handleClient(int clientFd)
    {
        std::string request;
        std::array<char, ReadBufferSize> buffer {};
        while (!request.contains("\r\n\r\n"))
        {
            const ssize_t Read = ::recv(clientFd, buffer.data(), buffer.size(), 0);
            if (Read <= 0)
            {
                return;
            }
            request.append(buffer.data(), static_cast<std::size_t>(Read));
        }

        const std::size_t LineEnd = request.find("\r\n");
        const std::string RequestLine = request.substr(0, LineEnd);
        const std::size_t FirstSpace = RequestLine.find(' ');
        const std::size_t SecondSpace = RequestLine.find(' ', FirstSpace + 1);
        if (FirstSpace != std::string::npos && SecondSpace != std::string::npos)
        {
            lastMethod_ = RequestLine.substr(0, FirstSpace);
        }

        std::size_t contentLength = 0;
        std::size_t searchPos = 0;
        while (true)
        {
            const std::size_t HeaderLineEnd = request.find("\r\n", searchPos);
            if (HeaderLineEnd == std::string::npos)
            {
                break;
            }
            const std::string HeaderLine = request.substr(searchPos, HeaderLineEnd - searchPos);
            searchPos = HeaderLineEnd + 2;
            if (HeaderLine.empty())
            {
                break;
            }
            constexpr std::string_view Prefix = "Content-Length:";
            if (HeaderLine.starts_with(Prefix))
            {
                contentLength =
                    static_cast<std::size_t>(std::stoul(HeaderLine.substr(Prefix.size())));
            }
        }

        const std::size_t BodyStart = request.find("\r\n\r\n");
        if (BodyStart != std::string::npos)
        {
            lastBody_ = request.substr(BodyStart + 4);
            while (lastBody_.size() < contentLength)
            {
                const ssize_t Read = ::recv(clientFd, buffer.data(), buffer.size(), 0);
                if (Read <= 0)
                {
                    break;
                }
                lastBody_.append(buffer.data(), static_cast<std::size_t>(Read));
            }
            if (lastBody_.size() > contentLength)
            {
                lastBody_.resize(contentLength);
            }
        }

        const std::string Response = "HTTP/1.1 " + std::to_string(statusCode_) + " OK\r\n" +
                                     "Content-Type: " + contentType_ + "\r\n" +
                                     "Content-Length: " + std::to_string(body_.size()) + "\r\n" +
                                     "Connection: close\r\n\r\n" + body_;
        (void)::send(clientFd, Response.data(), Response.size(), 0);
    }

    int listenFd_ = -1;
    std::uint16_t port_ = 0;
    std::thread worker_;
    std::atomic<bool> stop_ {false};
    int statusCode_ = 200;
    std::string body_ = "ok";
    std::string contentType_ = "text/plain";
    std::string lastMethod_;
    std::string lastBody_;
};

}  // namespace beez::test
