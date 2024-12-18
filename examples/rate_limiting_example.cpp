#include "Network.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // Rate limiting example
    {
        Network::RequestConfig config;
        config.rate_limit_per_minute = 30;  // Limit to 30 requests per minute
        
        std::cout << "\n=== Rate Limiting Example ===" << std::endl;
        for (int i = 0; i < 5; i++) {
            auto start = std::chrono::steady_clock::now();
            
            auto response = Network::Get("https://httpbin.org/get", config);
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            std::cout << "Request " << i + 1 << ":" << std::endl;
            std::cout << "Status: " << response.status_code << std::endl;
            std::cout << "Time taken: " << duration.count() << "ms" << std::endl;
        }
    }

    // Retry mechanism example
    {
        Network::RequestConfig config;
        config.max_retries = 3;
        config.retry_delay_ms = 1000;  // 1 second between retries
        
        std::cout << "\n=== Retry Mechanism Example ===" << std::endl;
        // Using a non-existent endpoint to trigger retries
        auto response = Network::Get("https://httpbin.org/status/500", config);
        std::cout << "Final status after retries: " << response.status_code << std::endl;
    }

    // Timeout example
    {
        Network::RequestConfig config;
        config.timeout_seconds = 5;  // 5 second timeout
        
        std::cout << "\n=== Timeout Example ===" << std::endl;
        // Using a delayed response endpoint
        auto response = Network::Get("https://httpbin.org/delay/10", config);
        std::cout << "Response: " << (response.success ? "Success" : "Timeout") << std::endl;
    }

    // Redirect following example
    {
        Network::RequestConfig config;
        config.follow_redirects = true;
        config.max_redirects = 5;
        
        std::cout << "\n=== Redirect Following Example ===" << std::endl;
        auto response = Network::Get("https://httpbin.org/redirect/3", config);
        std::cout << "Final status: " << response.status_code << std::endl;
    }

    Network::Cleanup();
    return 0;
}
