#include "Network.hpp"
#include <iostream>

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // Basic HTTPS GET request
    {
        auto response = Network::Get("https://api.github.com");
        std::cout << "\n=== Basic HTTPS GET Request ===" << std::endl;
        std::cout << "Status: " << response.status_code << std::endl;
        std::cout << "Body: " << response.body << std::endl;
    }

    // HTTPS with custom SSL configuration
    {
        Network::RequestConfig config;
        config.verify_ssl = true;  // Enable SSL verification
        config.use_tls12_or_higher = true;  // Enforce TLS 1.2 or higher
        
        auto response = Network::Get("https://api.github.com/zen", config);
        std::cout << "\n=== HTTPS with Custom SSL Config ===" << std::endl;
        std::cout << "Status: " << response.status_code << std::endl;
        std::cout << "Body: " << response.body << std::endl;
    }

    // HTTPS POST with OAuth token
    {
        Network::RequestConfig config;
        config.oauth_token = "your_oauth_token_here";  // Replace with actual token
        
        std::string json_payload = R"({
            "name": "test-repo",
            "description": "This is a test repository",
            "private": true
        })";
        
        auto response = Network::Post(
            "https://api.github.com/user/repos",
            json_payload,
            "application/json",
            config
        );
        std::cout << "\n=== HTTPS POST with OAuth ===" << std::endl;
        std::cout << "Status: " << response.status_code << std::endl;
    }

    // HTTPS with API key in header
    {
        Network::RequestConfig config;
        config.api_key = "your_api_key_here";  // Replace with actual API key
        config.additional_headers["Authorization"] = "Bearer " + config.api_key;
        
        auto response = Network::Get("https://api.example.com/secure", config);
        std::cout << "\n=== HTTPS with API Key ===" << std::endl;
        std::cout << "Status: " << response.status_code << std::endl;
    }

    Network::Cleanup();
    return 0;
}
