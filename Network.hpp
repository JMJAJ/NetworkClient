/**
 * @file Network.hpp
 * @brief HTTP Network Communication Library using WinINet
 * 
 * This library provides a modern C++ wrapper around the Windows Internet (WinINet) API,
 * offering a simple interface for making HTTP/HTTPS requests while handling common
 * scenarios like SSL, timeouts, retries, and rate limiting.
 * 
 * Features:
 * - Support for all major HTTP methods (GET, POST, PUT, PATCH, DELETE)
 * - SSL/TLS support with certificate validation
 * - Automatic retry mechanism with configurable delays
 * - Rate limiting capabilities
 * - Timeout handling at multiple levels
 * - Custom header support
 * - Authentication support (API Key, OAuth)
 * - Asynchronous request support
 * - HTTP/2 support
 * 
 * @author Jxint
 * @date December 2024
 */

#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <string>
#include <map>
#include <optional>
#include <chrono>
#include <functional>
#include <mutex> // Added mutex header

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif

/**
 * @brief Main networking class providing HTTP communication capabilities
 * 
 * This class provides a static interface for making HTTP requests using WinINet.
 * It handles connection management, request configuration, and response processing.
 */
class Network {
public:
    /**
     * @brief Supported HTTP methods
     */
    enum class Method {
        HTTP_GET,    ///< HTTP GET method
        HTTP_POST,   ///< HTTP POST method
        HTTP_PUT,    ///< HTTP PUT method
        HTTP_PATCH,  ///< HTTP PATCH method
        HTTP_DELETE  ///< HTTP DELETE method
    };

    /**
     * @brief Configuration options for HTTP requests
     */
    struct RequestConfig {
        int timeout_seconds = 30;     ///< Request timeout in seconds
        bool follow_redirects = true; ///< Whether to follow HTTP redirects
        int max_redirects = 5;        ///< Maximum number of redirects to follow
        std::map<std::string, std::string> additional_headers; ///< Custom headers
        bool verify_ssl = true;       ///< Enable SSL certificate verification
        bool use_tls12_or_higher = true; ///< Enforce TLS 1.2 or higher
        int max_retries = 3;          ///< Number of retry attempts
        int retry_delay_ms = 1000;    ///< Delay between retries in milliseconds
        std::string api_key;          ///< API key for authentication
        std::string oauth_token;      ///< OAuth token for authentication
        int rate_limit_per_minute = 0;///< Rate limiting (0 = disabled)
        bool use_http2 = true;        ///< Use HTTP/2 if available
        bool async_request = false;   ///< Make request asynchronously
    };

    /**
     * @brief HTTP response structure
     */
    struct NetworkResponse {
        int status_code = 0;          ///< HTTP status code
        std::string body;             ///< Response body
        std::map<std::string, std::string> headers; ///< Response headers
        bool success = false;         ///< Whether request was successful
        std::string error_message;    ///< Error message if request failed
    };

    /**
     * @brief Initialize the network library
     * @return true if initialization successful, false otherwise
     */
    static bool Initialize();

    /**
     * @brief Clean up network resources
     */
    static void Cleanup();

    /**
     * @brief Make an HTTP request
     * @param method HTTP method to use
     * @param url Target URL
     * @param payload Optional request body
     * @param config Request configuration
     * @return NetworkResponse containing the response or error
     */
    static NetworkResponse Request(
        Method method,
        const std::string& url,
        const std::optional<std::string>& payload = std::nullopt,
        const RequestConfig& config = RequestConfig()
    );

    /**
     * @brief Make a GET request
     * @param url Target URL
     * @param config Request configuration
     * @return NetworkResponse containing the response or error
     */
    static NetworkResponse Get(
        const std::string& url,
        const RequestConfig& config = RequestConfig()
    );

    /**
     * @brief Make a POST request
     * @param url Target URL
     * @param payload Request body
     * @param content_type Content type of the payload
     * @param config Request configuration
     * @return NetworkResponse containing the response or error
     */
    static NetworkResponse Post(
        const std::string& url,
        const std::string& payload,
        const std::string& content_type = "application/x-www-form-urlencoded",
        const RequestConfig& config = RequestConfig()
    );

    /**
     * @brief Make a PUT request
     * @param url Target URL
     * @param payload Request body
     * @param content_type Content type of the payload
     * @param config Request configuration
     * @return NetworkResponse containing the response or error
     */
    static NetworkResponse Put(
        const std::string& url,
        const std::string& payload,
        const std::string& content_type = "application/x-www-form-urlencoded",
        const RequestConfig& config = RequestConfig()
    );

    /**
     * @brief Make a DELETE request
     * @param url Target URL
     * @param config Request configuration
     * @return NetworkResponse containing the response or error
     */
    static NetworkResponse Delete(
        const std::string& url,
        const RequestConfig& config = RequestConfig()
    );

    /**
     * @brief Make an asynchronous HTTP request
     * @param method HTTP method to use
     * @param url Target URL
     * @param callback Callback function to handle the response
     * @param payload Optional request body
     * @param config Request configuration
     */
    static void RequestAsync(
        Method method,
        const std::string& url,
        std::function<void(NetworkResponse)> callback,
        const std::optional<std::string>& payload = std::nullopt,
        const RequestConfig& config = RequestConfig()
    );

    /**
     * @brief Make an asynchronous GET request
     * @param url Target URL
     * @param callback Callback function to handle the response
     * @param config Request configuration
     */
    static void GetAsync(
        const std::string& url,
        std::function<void(NetworkResponse)> callback,
        const RequestConfig& config = RequestConfig()
    );

    /**
     * @brief Make an asynchronous POST request
     * @param url Target URL
     * @param payload Request body
     * @param content_type Content type of the payload
     * @param callback Callback function to handle the response
     * @param config Request configuration
     */
    static void PostAsync(
        const std::string& url,
        const std::string& payload,
        const std::string& content_type,
        std::function<void(NetworkResponse)> callback,
        const RequestConfig& config = RequestConfig()
    );
    /**
     * @brief URL encode a string
     * @param input String to encode
     * @return URL encoded string
     */
    static std::string UrlEncode(const std::string& input);

    /**
     * @brief Base64 encode a string
     * @param input String to encode
     * @return Base64 encoded string
     */
    static std::string Base64Encode(const std::string& input);

private:
    /**
     * @brief Parse a URL into its components
     * @param url URL to parse
     * @param protocol Output protocol (http/https)
     * @param host Output hostname
     * @param path Output path
     * @param port Output port number
     * @return true if parsing successful, false otherwise
     */
    static bool ParseUrl(
        const std::string& url,
        std::string& protocol,
        std::string& host,
        std::string& path,
        int& port
    );
    
    static HINTERNET hSession;        ///< Global WinHTTP session handle
    static std::mutex sessionMutex;   ///< Mutex for session handle access
    static std::mutex requestMutex;   ///< Mutex for request synchronization
    
    // Rate limiting support
    struct RateLimitInfo {
        std::chrono::steady_clock::time_point lastRequest;
        int requestCount;
    };
    static std::map<std::string, RateLimitInfo> rateLimitMap;  ///< Rate limit tracking per host
    static std::mutex rateLimitMutex;  ///< Mutex for rate limit map access

    /**
     * @brief Applies rate limiting for a host
     * @param host The host to rate limit
     * @param rateLimit The maximum number of requests per minute
     * @return true if request can proceed, false if rate limit exceeded
     */
    static bool ApplyRateLimit(const std::string& host, int rateLimit);
};

#endif // NETWORK_HPP