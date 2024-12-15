#include "Network.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

HINTERNET Network::hInternet = NULL;

bool Network::Initialize() {
    hInternet = InternetOpen(
        L"Network/1.0",
        INTERNET_OPEN_TYPE_DIRECT,
        NULL,
        NULL,
        0
    );
    return hInternet != NULL;
}

void Network::Cleanup() {
    if (hInternet) {
        InternetCloseHandle(hInternet);
        hInternet = NULL;
    }
}

Network::NetworkResponse Network::Request(
    Method method,
    const std::string& url,
    const std::optional<std::string>& payload,
    const RequestConfig& config
) {
    NetworkResponse response;
    std::string protocol, host, path;
    int port;

    if (!ParseUrl(url, protocol, host, path, port)) {
        response.success = false;
        response.error_message = "Invalid URL";
        return response;
    }

    // Convert host and path to wide strings
    std::wstring whost(host.begin(), host.end());
    std::wstring wpath(path.begin(), path.end());

    // Connect to server
    DWORD flags = (protocol == "https") ? INTERNET_FLAG_SECURE : 0;
    HINTERNET hConnect = InternetConnectA(
        hInternet,
        host.c_str(),
        port,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
    );

    if (!hConnect) {
        response.error_message = "Failed to connect to server";
        return response;
    }

    // Convert method to string
    const char* method_str;
    switch (method) {
        case Method::HTTP_GET: method_str = "GET"; break;
        case Method::HTTP_POST: method_str = "POST"; break;
        case Method::HTTP_PUT: method_str = "PUT"; break;
        case Method::HTTP_PATCH: method_str = "PATCH"; break;
        case Method::HTTP_DELETE: method_str = "DELETE"; break;
        default: method_str = "GET";
    }

    // Open request
    HINTERNET hRequest = HttpOpenRequestA(
        hConnect,
        method_str,
        path.c_str(),
        NULL,
        NULL,
        NULL,
        flags,
        0
    );

    if (!hRequest) {
        InternetCloseHandle(hConnect);
        response.error_message = "Failed to create request";
        return response;
    }

    // Add headers
    std::string headers;
    for (const auto& header : config.additional_headers) {
        headers += header.first + ": " + header.second + "\r\n";
    }

    if (!headers.empty()) {
        HttpAddRequestHeadersA(
            hRequest,
            headers.c_str(),
            -1,
            HTTP_ADDREQ_FLAG_ADD
        );
    }

    // Send request
    BOOL success;
    if (payload) {
        success = HttpSendRequestA(
            hRequest,
            NULL,
            0,
            (LPVOID)payload->c_str(),
            payload->length()
        );
    } else {
        success = HttpSendRequestA(
            hRequest,
            NULL,
            0,
            NULL,
            0
        );
    }

    if (!success) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        response.error_message = "Failed to send request";
        return response;
    }

    // Get status code
    DWORD status_code = 0;
    DWORD size = sizeof(status_code);
    HttpQueryInfoA(
        hRequest,
        HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
        &status_code,
        &size,
        NULL
    );
    response.status_code = status_code;

    // Read response
    std::string response_body;
    char buffer[4096];
    DWORD bytes_read;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
        response_body.append(buffer, bytes_read);
    }

    response.body = response_body;
    response.success = (status_code >= 200 && status_code < 300);

    // Clean up
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);

    return response;
}

Network::NetworkResponse Network::Get(const std::string& url, const RequestConfig& config) {
    return Request(Method::HTTP_GET, url, std::nullopt, config);
}

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

Network::NetworkResponse Network::Delete(const std::string& url, const RequestConfig& config) {
    return Request(Method::HTTP_DELETE, url, std::nullopt, config);
}

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