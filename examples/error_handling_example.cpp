#include "Network.hpp"
#include <iostream>

void printResponse(const Network::NetworkResponse& response) {
    std::cout << "Success: " << (response.success ? "Yes" : "No") << std::endl;
    std::cout << "Status Code: " << response.status_code << std::endl;
    if (!response.success) {
        std::cout << "Error Message: " << response.error_message << std::endl;
    }
    std::cout << "Headers:" << std::endl;
    for (const auto& [key, value] : response.headers) {
        std::cout << "  " << key << ": " << value << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // Invalid URL format
    {
        std::cout << "=== Testing Invalid URL ===" << std::endl;
        auto response = Network::Get("not_a_valid_url");
        printResponse(response);
    }

    // Non-existent domain
    {
        std::cout << "=== Testing Non-existent Domain ===" << std::endl;
        auto response = Network::Get("http://this-domain-definitely-does-not-exist.com");
        printResponse(response);
    }

    // Invalid SSL certificate
    {
        std::cout << "=== Testing Invalid SSL Certificate ===" << std::endl;
        Network::RequestConfig config;
        config.verify_ssl = true;
        auto response = Network::Get("https://expired.badssl.com/", config);
        printResponse(response);
    }

    // 404 Not Found
    {
        std::cout << "=== Testing 404 Not Found ===" << std::endl;
        auto response = Network::Get("https://api.github.com/non_existent_endpoint");
        printResponse(response);
    }

    // 403 Forbidden
    {
        std::cout << "=== Testing 403 Forbidden ===" << std::endl;
        auto response = Network::Get("https://api.github.com/user/repos");  // Requires authentication
        printResponse(response);
    }

    // Network timeout
    {
        std::cout << "=== Testing Network Timeout ===" << std::endl;
        Network::RequestConfig config;
        config.timeout_seconds = 1;  // Very short timeout
        auto response = Network::Get("https://httpbin.org/delay/5", config);
        printResponse(response);
    }

    // Invalid request body
    {
        std::cout << "=== Testing Invalid Request Body ===" << std::endl;
        std::string invalid_json = "{not valid json}";
        auto response = Network::Post(
            "https://api.github.com/repos/octocat/Hello-World/issues",
            invalid_json,
            "application/json"
        );
        printResponse(response);
    }

    // Too many redirects
    {
        std::cout << "=== Testing Too Many Redirects ===" << std::endl;
        Network::RequestConfig config;
        config.max_redirects = 2;  // Only allow 2 redirects
        auto response = Network::Get("https://httpbin.org/redirect/5", config);
        printResponse(response);
    }

    Network::Cleanup();
    return 0;
}
