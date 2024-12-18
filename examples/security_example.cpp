#include "Network.hpp"
#include <iostream>

void printSecurityInfo(const Network::NetworkResponse& response) {
    std::cout << "Status: " << response.status_code << std::endl;
    std::cout << "SSL/TLS Info:" << std::endl;
    for (const auto& [key, value] : response.headers) {
        if (key.find("SSL") != std::string::npos || 
            key.find("TLS") != std::string::npos || 
            key == "Strict-Transport-Security") {
            std::cout << "  " << key << ": " << value << std::endl;
        }
    }
    std::cout << std::endl;
}

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // SSL/TLS with certificate validation
    {
        std::cout << "\n=== SSL/TLS with Certificate Validation ===" << std::endl;
        
        Network::RequestConfig config;
        config.verify_ssl = true;
        config.use_tls12_or_higher = true;
        
        // Test with a valid certificate
        auto response = Network::Get("https://api.github.com", config);
        std::cout << "Valid Certificate Test:" << std::endl;
        printSecurityInfo(response);
        
        // Test with an invalid certificate
        response = Network::Get("https://expired.badssl.com/", config);
        std::cout << "Invalid Certificate Test:" << std::endl;
        printSecurityInfo(response);
    }

    // API Key Authentication
    {
        std::cout << "\n=== API Key Authentication ===" << std::endl;
        
        Network::RequestConfig config;
        config.api_key = "your_api_key_here";
        
        // Bearer token authentication
        config.additional_headers["Authorization"] = "Bearer " + config.api_key;
        
        // Custom header authentication
        config.additional_headers["X-API-Key"] = config.api_key;
        
        auto response = Network::Get("https://httpbin.org/headers", config);
        printSecurityInfo(response);
    }

    // OAuth Token Authentication
    {
        std::cout << "\n=== OAuth Token Authentication ===" << std::endl;
        
        Network::RequestConfig config;
        config.oauth_token = "your_oauth_token_here";
        config.additional_headers["Authorization"] = "Bearer " + config.oauth_token;
        
        auto response = Network::Get("https://httpbin.org/headers", config);
        printSecurityInfo(response);
        
        // Demonstrate token refresh (simulated)
        std::cout << "Simulating token refresh..." << std::endl;
        config.oauth_token = "refreshed_token";
        config.additional_headers["Authorization"] = "Bearer " + config.oauth_token;
        
        response = Network::Get("https://httpbin.org/headers", config);
        printSecurityInfo(response);
    }

    // Advanced Security Features
    {
        std::cout << "\n=== Advanced Security Features ===" << std::endl;
        
        Network::RequestConfig config;
        config.verify_ssl = true;
        config.use_tls12_or_higher = true;
        config.additional_headers["Strict-Transport-Security"] = "max-age=31536000";
        config.additional_headers["X-Content-Type-Options"] = "nosniff";
        config.additional_headers["X-Frame-Options"] = "DENY";
        config.additional_headers["X-XSS-Protection"] = "1; mode=block";
        
        auto response = Network::Get("https://httpbin.org/headers", config);
        printSecurityInfo(response);
    }

    // Basic Authentication
    {
        std::cout << "\n=== Basic Authentication ===" << std::endl;
        
        std::string credentials = Network::Base64Encode("username:password");
        
        Network::RequestConfig config;
        config.additional_headers["Authorization"] = "Basic " + credentials;
        
        auto response = Network::Get("https://httpbin.org/basic-auth/username/password", config);
        std::cout << "Basic Auth Status: " << response.status_code << std::endl;
        std::cout << response.body << std::endl;
    }

    Network::Cleanup();
    return 0;
}
