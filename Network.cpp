#include "Network.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#pragma warning(disable : 4996)

// Implementation of methods (simplified for brevity)
bool Network::Initialize() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void Network::Cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
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

    // Create socket
    int sock = CreateSocket(host, port);
    if (sock < 0) {
        response.success = false;
        response.error_message = "Socket creation failed";
        return response;
    }

    // Construct request based on method
    std::ostringstream request;
    std::string method_str;
    switch (method) {
    case Method::HTTP_GET: method_str = "GET"; break;
    case Method::HTTP_POST: method_str = "POST"; break;
    case Method::HTTP_PUT: method_str = "PUT"; break;
    case Method::HTTP_DELETE: method_str = "DELETE"; break;
    case Method::HTTP_PATCH: method_str = "PATCH"; break;
    }

    request << method_str << " " << path << " HTTP/1.1\r\n"
        << "Host: " << host << "\r\n";

    // Add additional headers
    for (const auto& header : config.additional_headers) {
        request << header.first << ": " << header.second << "\r\n";
    }

    // Handle payload for methods that support it
    if (payload && (method == Method::HTTP_POST || method == Method::HTTP_PUT || method == Method::HTTP_PATCH)) {
        request << "Content-Type: application/json\r\n"
            << "Content-Length: " << payload->length() << "\r\n";
    }

    request << "Connection: close\r\n\r\n";

    if (payload) {
        request << *payload;
    }

    std::string request_str = request.str();

    // Send request
    if (send(sock, request_str.c_str(), request_str.length(), 0) < 0) {
        response.success = false;
        response.error_message = "Send failed";
        CloseSocket(sock);
        return response;
    }

    // Receive response
    std::string raw_response;
    char buffer[4096];
    int bytes_read;
    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        raw_response += buffer;
    }

    // Close socket
    CloseSocket(sock);

    // Parse response (simplified)
    size_t header_end = raw_response.find("\r\n\r\n");
    if (header_end != std::string::npos) {
        std::string headers_str = raw_response.substr(0, header_end);
        response.body = raw_response.substr(header_end + 4);

        // Parse status code (simplified)
        size_t status_start = headers_str.find("HTTP/1.1 ");
        if (status_start != std::string::npos) {
            response.status_code = std::stoi(headers_str.substr(status_start + 9, 3));
        }

        response.success = (response.status_code >= 200 && response.status_code < 300);
    }

    return response;
}

Network::NetworkResponse Network::Get(
    const std::string& url,
    const RequestConfig& config
) {
    return Request(Method::HTTP_GET, url, std::nullopt, config);
}

Network::NetworkResponse Network::Post(
    const std::string& url,
    const std::string& payload,
    const std::string& content_type,
    const RequestConfig& config
) {
    RequestConfig modified_config = config;
    modified_config.additional_headers["Content-Type"] = content_type;
    return Request(Method::HTTP_POST, url, payload, modified_config);
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

int Network::CreateSocket(const std::string& host, int port) {
    // Socket creation logic (simplified)
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
#endif

    struct sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    struct hostent* server = gethostbyname(host.c_str());
    if (!server) return -1;

    std::memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        CloseSocket(sock);
        return -1;
    }

    return sock;
}

void Network::CloseSocket(int socket) {
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}