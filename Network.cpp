/**
 * @file Network.cpp
 * @brief Implementation of the Network class
 * 
 * This file contains the implementation of the Network class methods,
 * providing HTTP communication capabilities using the WinHTTP API.
 */

#include "Network.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <winhttp.h>
#include <mutex>
#include <map>
#include <string>
#include <thread>
#include <future>
#include <chrono>

#pragma comment(lib, "winhttp.lib")

// Define HTTP/2 flag if not available in older Windows SDK
#ifndef WINHTTP_FLAG_HTTP2
#define WINHTTP_FLAG_HTTP2 0x00000020
#endif

// Define TLS 1.2 flag if not available
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000800
#endif

// Initialize static members
HINTERNET Network::hSession = NULL;
std::mutex Network::sessionMutex;
std::mutex Network::requestMutex;
std::mutex Network::rateLimitMutex;
std::map<std::string, Network::RateLimitInfo> Network::rateLimitMap;

/**
 * @brief Initializes the WinHTTP API
 * 
 * This method initializes the WinHTTP API with default browser settings.
 * 
 * @return true if initialization is successful, false otherwise
 */
bool Network::Initialize() {
    std::lock_guard<std::mutex> lock(sessionMutex);
    
    if (hSession) {
        return true;  // Already initialized
    }

    // Initialize WinHTTP with default browser settings
    hSession = WinHttpOpen(
        L"Network Library/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC  // Enable async for connection pooling
    );

    if (!hSession) {
        DWORD error = GetLastError();
        printf("[Network] Failed to initialize. Error code: %lu\n", error);
        return false;
    }

    // Configure connection pooling
    DWORD maxConnections = 128;  // Maximum number of connections in the pool
    WinHttpSetOption(hSession, WINHTTP_OPTION_MAX_CONNS_PER_SERVER, &maxConnections, sizeof(maxConnections));

    // Configure default timeouts (in milliseconds)
    DWORD resolveTimeout = 15000;   // DNS resolution timeout
    DWORD connectTimeout = 30000;   // Connection timeout
    DWORD sendTimeout = 30000;      // Send timeout
    DWORD receiveTimeout = 30000;   // Receive timeout

    WinHttpSetTimeouts(hSession, resolveTimeout, connectTimeout, sendTimeout, receiveTimeout);

    return true;
}

/**
 * @brief Cleans up the WinHTTP API
 * 
 * This method closes the WinHTTP handle and sets it to NULL.
 */
void Network::Cleanup() {
    std::lock_guard<std::mutex> lock(sessionMutex);
    
    if (hSession) {
        WinHttpCloseHandle(hSession);
        hSession = NULL;
    }
}

/**
 * @brief Applies rate limiting for a given host
 * 
 * This method checks if the rate limit for a given host has been exceeded.
 * 
 * @param host The host to check the rate limit for
 * @param rateLimit The rate limit per minute
 * @return true if the rate limit has not been exceeded, false otherwise
 */
bool Network::ApplyRateLimit(const std::string& host, int rateLimit) {
    if (rateLimit <= 0) {
        return true;  // Rate limiting disabled
    }

    std::lock_guard<std::mutex> lock(rateLimitMutex);
    auto now = std::chrono::steady_clock::now();
    auto& info = rateLimitMap[host];

    // If this is the first request, initialize
    if (info.requestCount == 0) {
        info.lastRequest = now;
        info.requestCount = 1;
        return true;
    }

    // Calculate time since window start
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.lastRequest).count();
    
    // If window has passed, reset counter
    if (elapsed >= 60000) {  // 60 seconds = 1 minute
        info.lastRequest = now;
        info.requestCount = 1;
        return true;
    }

    // Check if we've exceeded rate limit
    if (info.requestCount >= rateLimit) {
        // Calculate remaining time in window
        auto remainingTime = 60000 - elapsed;
        return false;  // Rate limit exceeded
    }

    // Increment counter and allow request
    info.requestCount++;
    return true;
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
    std::lock_guard<std::mutex> lock(requestMutex);
    NetworkResponse response;
    
    // Parse URL
    std::string protocol, host, path;
    int port;
    if (!ParseUrl(url, protocol, host, path, port)) {
        response.success = false;
        response.error_message = "Invalid URL";
        return response;
    }

    // Apply rate limiting
    if (config.rate_limit_per_minute > 0 && !ApplyRateLimit(host, config.rate_limit_per_minute)) {
        response.success = false;
        response.error_message = "Rate limit exceeded for host: " + host + ". Please wait before retrying.";
        response.status_code = 429;  // HTTP 429 Too Many Requests
        return response;
    }

    // Convert strings to wide strings
    std::wstring whost(host.begin(), host.end());
    std::wstring wpath(path.begin(), path.end());

    // Create connection handle with connection pooling
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        whost.c_str(),
        static_cast<WORD>(port),
        0
    );

    if (!hConnect) {
        response.error_message = "Failed to connect";
        return response;
    }

    // Create request handle
    LPCWSTR pwszVerb = nullptr;
    switch (method) {
        case Method::HTTP_GET:    pwszVerb = L"GET"; break;
        case Method::HTTP_POST:   pwszVerb = L"POST"; break;
        case Method::HTTP_PUT:    pwszVerb = L"PUT"; break;
        case Method::HTTP_DELETE: pwszVerb = L"DELETE"; break;
        default:                  pwszVerb = L"GET"; break;
    }

    DWORD flags = WINHTTP_FLAG_REFRESH;
    if (protocol == "https") {
        flags |= WINHTTP_FLAG_SECURE;
    }

    // Add timeout flag if timeout is configured
    if (config.timeout_seconds > 0) {
        flags |= WINHTTP_FLAG_REFRESH;  // Force refresh to ensure timeout works
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        pwszVerb,
        wpath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        response.error_message = "Failed to create request";
        return response;
    }

    // Set timeouts
    if (config.timeout_seconds > 0) {
        DWORD timeout = config.timeout_seconds * 1000;
        DWORD resolveTimeout = (timeout / 4 > 15000) ? 15000 : timeout / 4;  // Max 15s for DNS
        DWORD connectTimeout = (timeout / 2 > 30000) ? 30000 : timeout / 2;  // Max 30s for connect
        DWORD sendTimeout = timeout;
        DWORD receiveTimeout = timeout;

        WinHttpSetTimeouts(hRequest, resolveTimeout, connectTimeout, sendTimeout, receiveTimeout);

        // Set additional timeout options
        DWORD option = WINHTTP_OPTION_CONNECT_RETRIES;
        DWORD retries = 3;
        WinHttpSetOption(hRequest, option, &retries, sizeof(retries));
    }

    // Add custom headers
    std::wstring headers;
    for (const auto& [key, value] : config.additional_headers) {
        headers += std::wstring(key.begin(), key.end()) + L": " +
                  std::wstring(value.begin(), value.end()) + L"\r\n";
    }
    
    if (!headers.empty()) {
        WinHttpAddRequestHeaders(
            hRequest,
            headers.c_str(),
            static_cast<DWORD>(-1L),
            WINHTTP_ADDREQ_FLAG_ADD
        );
    }

    // Send request
    BOOL bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        payload ? const_cast<LPVOID>(static_cast<LPCVOID>(payload->c_str())) : WINHTTP_NO_REQUEST_DATA,
        payload ? static_cast<DWORD>(payload->size()) : 0,
        payload ? static_cast<DWORD>(payload->size()) : 0,
        0
    );

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }

    if (!bResults) {
        DWORD error = GetLastError();
        switch (error) {
            case ERROR_WINHTTP_TIMEOUT:
                response.error_message = "Request timed out";
                break;
            case ERROR_WINHTTP_NAME_NOT_RESOLVED:
                response.error_message = "DNS name resolution failed";
                break;
            case ERROR_WINHTTP_CANNOT_CONNECT:
                response.error_message = "Failed to connect to server";
                break;
            case ERROR_WINHTTP_CONNECTION_ERROR:
                response.error_message = "Connection was terminated";
                break;
            default:
                char errorMsg[256];
                sprintf_s(errorMsg, "Request failed with error code: %lu", error);
                response.error_message = errorMsg;
        }
        response.success = false;
        response.status_code = 0;
        
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return response;
    }

    // Get status code
    DWORD statusCode = 0;
    DWORD size = sizeof(DWORD);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &size,
        WINHTTP_NO_HEADER_INDEX
    );
    response.status_code = statusCode;

    // Get headers
    DWORD headerSize = 0;
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_RAW_HEADERS_CRLF,
        WINHTTP_HEADER_NAME_BY_INDEX,
        NULL,
        &headerSize,
        WINHTTP_NO_HEADER_INDEX
    );

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        std::vector<wchar_t> headerBuffer(headerSize / sizeof(wchar_t));
        WinHttpQueryHeaders(
            hRequest,
            WINHTTP_QUERY_RAW_HEADERS_CRLF,
            WINHTTP_HEADER_NAME_BY_INDEX,
            headerBuffer.data(),
            &headerSize,
            WINHTTP_NO_HEADER_INDEX
        );

        std::wstring headerStr(headerBuffer.data());
        std::wistringstream headerStream(headerStr);
        std::wstring line;

        while (std::getline(headerStream, line)) {
            if (line.empty() || line == L"\r") continue;
            
            size_t colonPos = line.find(L':');
            if (colonPos != std::wstring::npos) {
                std::wstring key = line.substr(0, colonPos);
                std::wstring value = line.substr(colonPos + 2); // Skip ": "
                if (!value.empty() && value.back() == L'\r') {
                    value.pop_back(); // Remove trailing \r
                }
                
                // Convert to narrow string
                std::string keyStr(key.begin(), key.end());
                std::string valueStr(value.begin(), value.end());
                response.headers[keyStr] = valueStr;
            }
        }
    }

    // Get response body
    std::string responseBody;
    DWORD bytesAvailable;
    do {
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            break;
        }

        if (bytesAvailable == 0) {
            break;
        }

        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
            responseBody.append(buffer.data(), bytesRead);
        }
    } while (bytesAvailable > 0);

    response.body = responseBody;
    response.success = (statusCode >= 200 && statusCode < 300);
    
    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);

    return response;
}

/**
 * @brief Sends an asynchronous HTTP request
 * 
 * This method sends an asynchronous HTTP request to the specified URL with the given method,
 * payload, and configuration.
 * 
 * @param method The HTTP method to use (e.g. GET, POST, PUT, DELETE)
 * @param url The URL to send the request to
 * @param callback The callback function to call with the response
 * @param payload The payload to send with the request (optional)
 * @param config The request configuration
 */
void Network::RequestAsync(Method method, const std::string& url,
    std::function<void(NetworkResponse)> callback,
    const std::optional<std::string>& payload, const RequestConfig& config) {
    
    std::thread([=]() {
        RequestConfig asyncConfig = config;
        asyncConfig.async_request = false;  // Prevent recursive async calls
        NetworkResponse response = Request(method, url, payload, asyncConfig);
        callback(response);
    }).detach();
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
 * @brief Sends an asynchronous GET request
 * 
 * This method sends an asynchronous GET request to the specified URL with the given configuration.
 * 
 * @param url The URL to send the request to
 * @param callback The callback function to call with the response
 * @param config The request configuration
 */
void Network::GetAsync(const std::string& url,
    std::function<void(NetworkResponse)> callback,
    const RequestConfig& config) {
    RequestAsync(Method::HTTP_GET, url, callback, std::nullopt, config);
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
 * @brief Sends an asynchronous POST request
 * 
 * This method sends an asynchronous POST request to the specified URL with the given payload,
 * content type, and configuration.
 * 
 * @param url The URL to send the request to
 * @param payload The payload to send with the request
 * @param content_type The content type of the payload
 * @param callback The callback function to call with the response
 * @param config The request configuration
 */
void Network::PostAsync(const std::string& url,
    const std::string& payload,
    const std::string& content_type,
    std::function<void(NetworkResponse)> callback,
    const RequestConfig& config) {
    
    RequestConfig asyncConfig = config;
    asyncConfig.additional_headers["Content-Type"] = content_type;
    RequestAsync(Method::HTTP_POST, url, callback, payload, asyncConfig);
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