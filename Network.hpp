#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <string>
#include <map>
#include <optional>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif

class Network {
public:
    // Enum for HTTP methods
    enum class Method {
        HTTP_GET,
        HTTP_POST,
        HTTP_PUT,
        HTTP_PATCH,
        HTTP_DELETE
    };

    // Configuration struct for advanced options
    struct RequestConfig {
        int timeout_seconds = 30;  // Default timeout
        bool follow_redirects = true;
        int max_redirects = 5;
        std::map<std::string, std::string> additional_headers;
    };

    // Result struct to provide more detailed response information
    struct NetworkResponse {
        int status_code = 0;
        std::string body;
        std::map<std::string, std::string> headers;
        bool success = false;
        std::string error_message;
    };

    // Static initialization and cleanup
    static bool Initialize();
    static void Cleanup();

    // Simplified request method supporting multiple HTTP methods
    static NetworkResponse Request(
        Method method,
        const std::string& url,
        const std::optional<std::string>& payload = std::nullopt,
        const RequestConfig& config = RequestConfig()
    );

    // Specific method shortcuts for convenience
    static NetworkResponse Get(
        const std::string& url,
        const RequestConfig& config = RequestConfig()
    );

    static NetworkResponse Post(
        const std::string& url,
        const std::string& payload,
        const std::string& content_type = "application/x-www-form-urlencoded",
        const RequestConfig& config = RequestConfig()
    );

    static NetworkResponse Put(
        const std::string& url,
        const std::string& payload,
        const std::string& content_type = "application/x-www-form-urlencoded",
        const RequestConfig& config = RequestConfig()
    );

    static NetworkResponse Delete(
        const std::string& url,
        const RequestConfig& config = RequestConfig()
    );

private:
    static std::string UrlEncode(const std::string& input);
    static std::string Base64Encode(const std::string& input);
    static bool ParseUrl(
        const std::string& url,
        std::string& protocol,
        std::string& host,
        std::string& path,
        int& port
    );
    
    // WinINet handling
    static HINTERNET hInternet;
};

#endif // NETWORK_HPP
