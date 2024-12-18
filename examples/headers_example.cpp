#include "Network.hpp"
#include <iostream>
#include <iomanip>

void printHeaders(const std::map<std::string, std::string>& headers) {
    for (const auto& [key, value] : headers) {
        std::cout << std::setw(30) << std::left << key << ": " << value << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // Standard Headers Example
    {
        std::cout << "=== Standard Headers Example ===" << std::endl;
        
        Network::RequestConfig config;
        config.additional_headers = {
            {"User-Agent", "CustomClient/1.0"},
            {"Accept", "application/json"},
            {"Accept-Language", "en-US,en;q=0.9"},
            {"Accept-Encoding", "gzip, deflate"},
            {"Cache-Control", "no-cache"}
        };
        
        auto response = Network::Get("https://httpbin.org/headers", config);
        std::cout << "Request Headers:" << std::endl;
        printHeaders(config.additional_headers);
        std::cout << "Response:" << std::endl;
        std::cout << response.body << std::endl;
    }

    // Custom Headers Example
    {
        std::cout << "\n=== Custom Headers Example ===" << std::endl;
        
        Network::RequestConfig config;
        config.additional_headers = {
            {"X-Custom-Header", "CustomValue"},
            {"X-Request-ID", "123456789"},
            {"X-API-Version", "2.0"},
            {"X-Client-Name", "NetworkLibrary"},
            {"X-Debug-Mode", "true"}
        };
        
        auto response = Network::Get("https://httpbin.org/headers", config);
        std::cout << "Custom Headers Sent:" << std::endl;
        printHeaders(config.additional_headers);
        std::cout << "Response:" << std::endl;
        std::cout << response.body << std::endl;
    }

    // Content Negotiation Example
    {
        std::cout << "\n=== Content Negotiation Example ===" << std::endl;
        
        // JSON request
        {
            Network::RequestConfig config;
            config.additional_headers = {
                {"Accept", "application/json"},
                {"Content-Type", "application/json"}
            };
            
            auto response = Network::Get("https://httpbin.org/anything", config);
            std::cout << "JSON Negotiation Response Headers:" << std::endl;
            printHeaders(response.headers);
        }
        
        // XML request
        {
            Network::RequestConfig config;
            config.additional_headers = {
                {"Accept", "application/xml"},
                {"Content-Type", "application/xml"}
            };
            
            auto response = Network::Get("https://httpbin.org/anything", config);
            std::cout << "XML Negotiation Response Headers:" << std::endl;
            printHeaders(response.headers);
        }
    }

    // Conditional Requests Example
    {
        std::cout << "\n=== Conditional Requests Example ===" << std::endl;
        
        // First request to get ETag
        auto initial_response = Network::Get("https://api.github.com/repos/octocat/Hello-World");
        std::string etag = initial_response.headers["ETag"];
        
        // Conditional request using ETag
        Network::RequestConfig config;
        config.additional_headers = {
            {"If-None-Match", etag}
        };
        
        auto conditional_response = Network::Get("https://api.github.com/repos/octocat/Hello-World", config);
        std::cout << "Conditional Request Status: " << conditional_response.status_code << std::endl;
        printHeaders(conditional_response.headers);
    }

    // CORS Headers Example
    {
        std::cout << "\n=== CORS Headers Example ===" << std::endl;
        
        Network::RequestConfig config;
        config.additional_headers = {
            {"Origin", "http://example.com"},
            {"Access-Control-Request-Method", "POST"},
            {"Access-Control-Request-Headers", "Content-Type"}
        };
        
        Network::NetworkResponse response = Network::Request(
            Network::Method::HTTP_GET,
            "https://httpbin.org/headers",
            std::nullopt,
            config
        );
        
        std::cout << "CORS Preflight Response Headers:" << std::endl;
        printHeaders(response.headers);
    }

    // Response Headers Processing Example
    {
        std::cout << "\n=== Response Headers Processing Example ===" << std::endl;
        
        auto response = Network::Get("https://api.github.com");
        
        std::cout << "Rate Limiting Info:" << std::endl;
        std::cout << "Rate Limit: " << response.headers["X-RateLimit-Limit"] << std::endl;
        std::cout << "Remaining: " << response.headers["X-RateLimit-Remaining"] << std::endl;
        std::cout << "Reset Time: " << response.headers["X-RateLimit-Reset"] << std::endl;
        
        std::cout << "\nServer Info:" << std::endl;
        std::cout << "Server: " << response.headers["Server"] << std::endl;
        std::cout << "Protocol Version: " << response.headers["HTTP"] << std::endl;
        
        if (response.headers.count("Content-Encoding")) {
            std::cout << "Content Encoding: " << response.headers["Content-Encoding"] << std::endl;
        }
        
        if (response.headers.count("Content-Type")) {
            std::cout << "Content Type: " << response.headers["Content-Type"] << std::endl;
        }
    }

    Network::Cleanup();
    return 0;
}
