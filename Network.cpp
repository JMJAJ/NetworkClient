/**
 * @file Network.cpp
 * @brief Implementation of the Network class
 * 
 * This file contains the implementation of the Network class methods,
 * providing HTTP communication capabilities using the WinINet API.
 */

#include "Network.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <wininet.h>
#include <mutex>
#include <chrono>
#include <map>
#include <string>
#include <thread>

#pragma comment(lib, "wininet.lib")

// Initialize static member
HINTERNET Network::hInternet = NULL;

/**
 * @brief Initializes the WinINet API
 * 
 * This method initializes the WinINet API with default browser settings.
 * 
 * @return true if initialization is successful, false otherwise
 */
bool Network::Initialize() {
    if (hInternet) {
        return true;  // Already initialized
    }

    // Initialize WinINet with default browser settings
    hInternet = InternetOpenW(
        L"Mozilla/5.0",
        INTERNET_OPEN_TYPE_PRECONFIG,  // Use system's internet settings
        NULL,
        NULL,
        0
    );

    if (!hInternet) {
        DWORD error = GetLastError();
        printf("[Network] Failed to initialize. Error code: %lu\n", error);
        return false;
    }

    // Set default timeouts for better responsiveness
    DWORD timeout = 5000;  // 5 seconds
    InternetSetOptionW(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    return true;
}

/**
 * @brief Cleans up the WinINet API
 * 
 * This method closes the WinINet handle and sets it to NULL.
 */
void Network::Cleanup() {
    if (hInternet) {
        InternetCloseHandle(hInternet);
        hInternet = NULL;
    }
}

/**
 * @brief Sends an HTTP request
 * 
 * This method sends an HTTP request to the specified URL with the given method,
 * payload, and configuration.
 * 
 * @param method The HTTP method to use (e.g. GET, POST, PUT, DELETE)
 * @param url The URL to send the request to
 * @param payload The payload to send with the request (optional)
 * @param config The request configuration
 * @return The response from the server
 */
Network::NetworkResponse Network::Request(
    Method method,
    const std::string& url,
    const std::optional<std::string>& payload,
    const RequestConfig& config
) {
    NetworkResponse response;
    std::string protocol, host, path;
    int port;

    // Parse and validate URL
    if (!ParseUrl(url, protocol, host, path, port)) {
        response.success = false;
        response.error_message = "Invalid URL";
        return response;
    }

    // Validate protocol
    if (protocol != "http" && protocol != "https") {
        response.error_message = "Invalid protocol: " + protocol;
        return response;
    }

    // Validate port range (1-65535)
    if (port <= 0 || port > 65535) {
        response.error_message = "Invalid port number";
        return response;
    }

    // Convert strings to wide strings for WinINet
    std::wstring whost(host.begin(), host.end());
    std::wstring wpath(path.begin(), path.end());

    // Set up SSL/TLS flags
    DWORD flags = (protocol == "https") ?
        (INTERNET_FLAG_SECURE |
            INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
            INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
            INTERNET_FLAG_NO_CACHE_WRITE) : 0;

    // Connect to server
    HINTERNET hConnect = InternetConnectW(
        hInternet,
        whost.c_str(),
        static_cast<WORD>(port),
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
    );

    if (!hConnect) {
        DWORD error = GetLastError();
        char errorMsg[256];
        sprintf_s(errorMsg, "Failed to connect to server. Error code: %lu", error);
        response.error_message = errorMsg;
        return response;
    }

    // Configure SSL/TLS security
    DWORD securityFlags = 0;
    if (protocol == "https") {
        securityFlags = INTERNET_FLAG_SECURE;
        if (config.verify_ssl) {
            // Strict SSL mode
            securityFlags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP;
        } else {
            // Relaxed SSL mode
            securityFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
                           INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
        }
    }

    // Set up timeouts for better responsiveness
    if (config.timeout_seconds > 0) {
        DWORD timeout = (config.timeout_seconds * 1000 > 1000) ? 1000 : config.timeout_seconds * 1000;
        DWORD resolveTimeout = 250;  // DNS timeout
        DWORD connectTimeout = 500;  // Connect timeout
        
        // Set timeouts at all levels
        InternetSetOptionW(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &resolveTimeout, sizeof(resolveTimeout));
        InternetSetOptionW(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionW(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionW(hInternet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionW(hInternet, INTERNET_OPTION_DATA_SEND_TIMEOUT, &timeout, sizeof(timeout));
        
        InternetSetOptionW(hConnect, INTERNET_OPTION_CONNECT_TIMEOUT, &connectTimeout, sizeof(connectTimeout));
        InternetSetOptionW(hConnect, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionW(hConnect, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionW(hConnect, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionW(hConnect, INTERNET_OPTION_DATA_SEND_TIMEOUT, &timeout, sizeof(timeout));
        
        // Add flags for faster timeout handling
        securityFlags |= INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_PRAGMA_NOCACHE | 
                        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_AUTH |
                        INTERNET_FLAG_NO_UI | INTERNET_FLAG_DONT_CACHE;
    }

    // Improved rate limiting with precise timing
    if (config.rate_limit_per_minute > 0) {
        static std::mutex rateLimitMutex;
        static std::map<std::string, std::chrono::steady_clock::time_point> lastRequestTime;
        
        const int minTimeBetweenRequests = (60 * 1000) / config.rate_limit_per_minute;
        
        std::lock_guard<std::mutex> lock(rateLimitMutex);
        auto now = std::chrono::steady_clock::now();
        
        auto it = lastRequestTime.find(url);
        if (it != lastRequestTime.end()) {
            auto timeSinceLastRequest = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - it->second).count();
            
            if (timeSinceLastRequest < minTimeBetweenRequests) {
                auto sleepTime = minTimeBetweenRequests - timeSinceLastRequest;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
                now = std::chrono::steady_clock::now(); // Update current time after sleep
            }
        }
        
        lastRequestTime[url] = now;
    }

    // Error handling with better timeout detection
    response.success = false;
    
    try {
        // Add authentication headers if provided
        auto headers = config.additional_headers;  // Create a local copy
        if (!config.api_key.empty()) {
            headers["Authorization"] = "Bearer " + config.api_key;
        }
        if (!config.oauth_token.empty()) {
            headers["Authorization"] = "Bearer " + config.oauth_token;
        }

        // Convert method to string
        std::string method_str;
        switch (method) {
        case Method::HTTP_GET: method_str = "GET"; break;
        case Method::HTTP_POST: method_str = "POST"; break;
        case Method::HTTP_PUT: method_str = "PUT"; break;
        case Method::HTTP_PATCH: method_str = "PATCH"; break;
        case Method::HTTP_DELETE: method_str = "DELETE"; break;
        default: method_str = "GET";
        }

        // Create the request with all flags
        HINTERNET hRequest = HttpOpenRequestW(
            hConnect,
            std::wstring(method_str.begin(), method_str.end()).c_str(),
            wpath.c_str(),
            NULL,
            NULL,
            NULL,
            securityFlags,
            0
        );

        if (!hRequest) {
            DWORD error = GetLastError();
            switch (error) {
                case ERROR_INTERNET_TIMEOUT:
                    response.error_message = "Connection timed out";
                    break;
                case ERROR_INTERNET_INVALID_CA:
                case ERROR_INTERNET_SEC_CERT_CN_INVALID:
                case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
                    response.error_message = "SSL certificate validation failed";
                    break;
                case ERROR_INTERNET_NAME_NOT_RESOLVED:
                    response.error_message = "Could not resolve domain name";
                    break;
                default:
                    response.error_message = "Failed to create request";
            }
            return response;
        }

        // Set request timeout and buffer sizes
        if (config.timeout_seconds > 0) {
            // Set aggressive timeouts at 80% of the requested timeout
            DWORD timeout = static_cast<DWORD>(config.timeout_seconds * 800);  // 80% of timeout in milliseconds
            InternetSetOptionW(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
            InternetSetOptionW(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
            InternetSetOptionW(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
            
            // Set even stricter timeouts for the request itself
            DWORD requestTimeout = static_cast<DWORD>(config.timeout_seconds * 600);  // 60% of timeout
            InternetSetOptionW(hRequest, INTERNET_OPTION_CONNECT_TIMEOUT, &requestTimeout, sizeof(requestTimeout));
            InternetSetOptionW(hRequest, INTERNET_OPTION_SEND_TIMEOUT, &requestTimeout, sizeof(requestTimeout));
            InternetSetOptionW(hRequest, INTERNET_OPTION_RECEIVE_TIMEOUT, &requestTimeout, sizeof(requestTimeout));
            
            // Add flags for better timeout control
            securityFlags |= INTERNET_FLAG_NO_CACHE_WRITE | 
                           INTERNET_FLAG_PRAGMA_NOCACHE | 
                           INTERNET_FLAG_RELOAD |
                           INTERNET_FLAG_NO_COOKIES;
            
            // Set up async operation for better timeout control
            INTERNET_BUFFERS BufferIn = { sizeof(INTERNET_BUFFERS) };
            if (!HttpSendRequestEx(hRequest, &BufferIn, NULL, HSR_INITIATE | HSR_SYNC, 0)) {
                response.error_message = "Failed to initiate request";
                return response;
            }
            
            // Set up manual timeout using a separate thread
            std::atomic<bool> requestCompleted{false};
            std::thread timeoutThread([&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
                if (!requestCompleted.load()) {
                    InternetCloseHandle(hRequest);
                    response.error_message = "Request timed out";
                }
            });
            timeoutThread.detach();
        }

        // Add headers
        std::string headers_str;
        for (const auto& header : headers) {
            headers_str += header.first + ": " + header.second + "\r\n";
        }

        if (!headers_str.empty()) {
            HttpAddRequestHeadersW(
                hRequest,
                std::wstring(headers_str.begin(), headers_str.end()).c_str(),
                -1,
                HTTP_ADDREQ_FLAG_ADD
            );
        }

        // Send request with timeout detection
        auto requestStart = std::chrono::steady_clock::now();
        BOOL success = HttpSendRequestW(hRequest, NULL, 0, 
            payload ? (LPVOID)payload->c_str() : NULL,
            payload ? payload->length() : 0
        );

        // Check for timeout or other errors
        if (!success) {
            DWORD error = GetLastError();
            if (error == ERROR_INTERNET_TIMEOUT) {
                response.error_message = "Request timed out";
            } else if (error == ERROR_INTERNET_INVALID_CA || 
                      error == ERROR_INTERNET_SEC_CERT_CN_INVALID ||
                      error == ERROR_INTERNET_SEC_CERT_DATE_INVALID) {
                response.error_message = "SSL certificate validation failed";
            } else {
                response.error_message = "Failed to send request";
            }
            InternetCloseHandle(hRequest);
            return response;
        }

        // Get status code
        DWORD statusCode = 0;
        DWORD size = sizeof(statusCode);
        if (!HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                          &statusCode, &size, NULL)) {
            response.error_message = "Failed to get status code";
            InternetCloseHandle(hRequest);
            return response;
        }
        
        response.status_code = statusCode;

        // Handle server errors with retry
        if (statusCode >= 500 && config.max_retries > 0) {
            InternetCloseHandle(hRequest);
            
            for (int retryCount = 0; retryCount < config.max_retries; retryCount++) {
                // Wait for the configured delay between retries
                if (config.retry_delay_ms > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(config.retry_delay_ms));
                } else {
                    // Default exponential backoff if no delay configured
                    int baseDelay = 100 * (1 << retryCount);
                    int delayMs = (baseDelay > 1000) ? 1000 : baseDelay;  // Cap at 1 second
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                }
                
                // Create new request for retry
                hRequest = HttpOpenRequestW(
                    hConnect,
                    std::wstring(method_str.begin(), method_str.end()).c_str(),
                    wpath.c_str(),
                    NULL,
                    NULL,
                    NULL,
                    securityFlags,
                    0
                );
                
                if (!hRequest) {
                    response.error_message = "Failed to create request for retry";
                    continue;
                }
                
                // Add headers again for retry
                if (!headers_str.empty()) {
                    HttpAddRequestHeadersW(
                        hRequest,
                        std::wstring(headers_str.begin(), headers_str.end()).c_str(),
                        -1,
                        HTTP_ADDREQ_FLAG_ADD
                    );
                }
                
                // Retry the request
                success = HttpSendRequestW(hRequest, NULL, 0,
                    payload ? (LPVOID)payload->c_str() : NULL,
                    payload ? payload->length() : 0
                );
                
                if (!success) {
                    InternetCloseHandle(hRequest);
                    continue;
                }
                
                // Check new status code
                if (!HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                                  &statusCode, &size, NULL)) {
                    InternetCloseHandle(hRequest);
                    continue;
                }
                
                // If status code is now good, break out of retry loop
                if (statusCode < 500) {
                    response.status_code = statusCode;
                    break;
                }
                
                // Close request handle before next retry
                InternetCloseHandle(hRequest);
            }
            
            // If we exhausted all retries, set final error
            if (statusCode >= 500) {
                response.error_message = "Server error (status " + std::to_string(statusCode) + ")";
                return response;
            }
        }

        // Read response with timeout check
        const int maxBufferSize = 8 * 1024; // 8KB chunks for faster timeout detection
        char buffer[maxBufferSize];
        DWORD bytesRead = 0;
        std::string responseData;
        responseData.reserve(64 * 1024); // Pre-allocate 64KB
        
        auto readStart = std::chrono::steady_clock::now();
        bool timedOut = false;
        
        while (InternetReadFile(hRequest, buffer, maxBufferSize, &bytesRead)) {
            if (bytesRead == 0) break;
            
            // Check if we've exceeded the timeout
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - readStart).count();
            if (config.timeout_seconds > 0 && elapsed > config.timeout_seconds * 1000) {
                timedOut = true;
                break;
            }
            
            responseData.append(buffer, bytesRead);
                
            // Add a small delay between reads to allow timeout to trigger
            if (config.timeout_seconds > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        
        if (timedOut) {
            response.error_message = "Request timed out";
            InternetCloseHandle(hRequest);
            return response;
        }

        response.body = responseData;
        response.success = (statusCode >= 200 && statusCode < 300);
        
        if (!response.success) {
            if (statusCode >= 500) {
                response.error_message = "Server error (status " + std::to_string(statusCode) + ")";
            } else if (statusCode >= 400) {
                response.error_message = "Client error (status " + std::to_string(statusCode) + ")";
            }
        }
        
        InternetCloseHandle(hRequest);
        
    } catch (const std::exception& e) {
        response.error_message = e.what();
    }
    
    InternetCloseHandle(hConnect);

    return response;
}

/**
 * @brief Sends a GET request
 * 
 * This method sends a GET request to the specified URL with the given configuration.
 * 
 * @param url The URL to send the request to
 * @param config The request configuration
 * @return The response from the server
 */
Network::NetworkResponse Network::Get(const std::string& url, const RequestConfig& config) {
    return Request(Method::HTTP_GET, url, std::nullopt, config);
}

/**
 * @brief Sends a POST request
 * 
 * This method sends a POST request to the specified URL with the given payload,
 * content type, and configuration.
 * 
 * @param url The URL to send the request to
 * @param payload The payload to send with the request
 * @param content_type The content type of the payload
 * @param config The request configuration
 * @return The response from the server
 */
Network::NetworkResponse Network::Post(
    const std::string& url,
    const std::string& payload,
    const std::string& content_type,
    const RequestConfig& config
) {
    RequestConfig cfg = config;
    cfg.additional_headers["Content-Type"] = content_type;
    return Request(Method::HTTP_POST, url, payload, cfg);
}

/**
 * @brief Sends a PUT request
 * 
 * This method sends a PUT request to the specified URL with the given payload,
 * content type, and configuration.
 * 
 * @param url The URL to send the request to
 * @param payload The payload to send with the request
 * @param content_type The content type of the payload
 * @param config The request configuration
 * @return The response from the server
 */
Network::NetworkResponse Network::Put(
    const std::string& url,
    const std::string& payload,
    const std::string& content_type,
    const RequestConfig& config
) {
    RequestConfig cfg = config;
    cfg.additional_headers["Content-Type"] = content_type;
    return Request(Method::HTTP_PUT, url, payload, cfg);
}

/**
 * @brief Sends a DELETE request
 * 
 * This method sends a DELETE request to the specified URL with the given configuration.
 * 
 * @param url The URL to send the request to
 * @param config The request configuration
 * @return The response from the server
 */
Network::NetworkResponse Network::Delete(const std::string& url, const RequestConfig& config) {
    return Request(Method::HTTP_DELETE, url, std::nullopt, config);
}

/**
 * @brief Parses a URL into its components
 * 
 * This method parses a URL into its protocol, host, path, and port components.
 * 
 * @param url The URL to parse
 * @param protocol The protocol component of the URL
 * @param host The host component of the URL
 * @param path The path component of the URL
 * @param port The port component of the URL
 * @return true if the URL is valid, false otherwise
 */
bool Network::ParseUrl(
    const std::string& url,
    std::string& protocol,
    std::string& host,
    std::string& path,
    int& port
) {
    // Basic URL parsing (simplified)
    size_t protocol_end = url.find("://");
    if (protocol_end == std::string::npos) return false;

    protocol = url.substr(0, protocol_end);
    size_t host_start = protocol_end + 3;
    size_t path_start = url.find('/', host_start);

    if (path_start == std::string::npos) {
        host = url.substr(host_start);
        path = "/";
    }
    else {
        host = url.substr(host_start, path_start - host_start);
        path = url.substr(path_start);
    }

    // Default ports
    port = (protocol == "https") ? 443 : 80;

    // Check for custom port
    size_t port_sep = host.find(':');
    if (port_sep != std::string::npos) {
        port = std::stoi(host.substr(port_sep + 1));
        host = host.substr(0, port_sep);
    }

    return true;
}

/**
 * @brief URL-encodes a string
 * 
 * This method URL-encodes a string by replacing special characters with their
 * corresponding escape sequences.
 * 
 * @param input The string to URL-encode
 * @return The URL-encoded string
 */
std::string Network::UrlEncode(const std::string& input) {
    std::string encoded;
    for (char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        }
        else {
            std::stringstream hex;
            hex << '%' << std::hex << static_cast<int>(static_cast<unsigned char>(c));
            encoded += hex.str();
        }
    }
    return encoded;
}

/**
 * @brief Base64-encodes a string
 * 
 * This method Base64-encodes a string by converting it to a binary format and
 * then encoding it using the Base64 alphabet.
 * 
 * @param input The string to Base64-encode
 * @return The Base64-encoded string
 */
std::string Network::Base64Encode(const std::string& input) {
    const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string encoded;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    for (char c : input) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (int j = 0; j < 4; j++) {
                encoded += base64_chars[char_array_4[j]];
            }
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++) {
            encoded += base64_chars[char_array_4[j]];
        }

        while (i++ < 3) {
            encoded += '=';
        }
    }

    return encoded;
}