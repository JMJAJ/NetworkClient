#ifndef SIMPLE_NETWORK_HPP
#define SIMPLE_NETWORK_HPP

#include <string>
#include <map>
#include <optional>
#include <chrono>

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
        const std::string& content_type = "application/json",
        const RequestConfig& config = RequestConfig()
    );

    // Utility methods
    static std::string UrlEncode(const std::string& input);
    static std::string Base64Encode(const std::string& input);

private:
    // Internal URL parsing method
    static bool ParseUrl(
        const std::string& url,
        std::string& protocol,
        std::string& host,
        std::string& path,
        int& port
    );

    // Low-level socket operations
    static int CreateSocket(const std::string& host, int port);
    static void CloseSocket(int socket);
};

#endif // SIMPLE_NETWORK_HPP