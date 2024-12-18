#include "../net/Network.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

// Helper function to measure request time
template<typename Func>
double measureTime(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // Connection Pooling Example
    {
        std::cout << "=== Connection Pooling Example ===" << std::endl;
        
        // Make multiple requests to the same host
        const int num_requests = 5;
        std::vector<double> times;
        
        for (int i = 0; i < num_requests; i++) {
            double time = measureTime([&]() {
                auto response = Network::Get("https://api.github.com/zen");
                std::cout << "Request " << i + 1 << " status: " << response.status_code << std::endl;
            });
            
            times.push_back(time);
            std::cout << "Request " << i + 1 << " time: " << time << "ms" << std::endl;
        }
        
        // Calculate average time
        double avg_time = 0;
        for (double time : times) {
            avg_time += time;
        }
        avg_time /= times.size();
        
        std::cout << "Average request time: " << avg_time << "ms" << std::endl;
    }

    // Keep-Alive Example
    {
        std::cout << "\n=== Keep-Alive Example ===" << std::endl;
        
        Network::RequestConfig config;
        config.additional_headers["Connection"] = "keep-alive";
        
        // Make multiple requests with keep-alive
        for (int i = 0; i < 3; i++) {
            double time = measureTime([&]() {
                auto response = Network::Get("https://api.github.com/zen", config);
                std::cout << "Request " << i + 1 << " status: " << response.status_code << std::endl;
                std::cout << "Connection header: " << response.headers["Connection"] << std::endl;
            });
            
            std::cout << "Request " << i + 1 << " time: " << time << "ms" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Small delay between requests
        }
    }

    // Connection Timeout Example
    {
        std::cout << "\n=== Connection Timeout Example ===" << std::endl;
        
        Network::RequestConfig config;
        config.timeout_seconds = 5;  // 5 second timeout
        
        // Test with a slow server
        double time = measureTime([&]() {
            auto response = Network::Get("https://httpbin.org/delay/10", config);
            std::cout << "Status: " << response.status_code << std::endl;
            std::cout << "Success: " << (response.success ? "Yes" : "No") << std::endl;
            if (!response.success) {
                std::cout << "Error: " << response.error_message << std::endl;
            }
        });
        
        std::cout << "Request time: " << time << "ms" << std::endl;
    }

    // Connection Recovery Example
    {
        std::cout << "\n=== Connection Recovery Example ===" << std::endl;
        
        Network::RequestConfig config;
        config.max_retries = 3;
        config.retry_delay_ms = 1000;
        
        // Test with an unreliable endpoint
        auto response = Network::Get("https://httpbin.org/status/500", config);
        std::cout << "Final status after retries: " << response.status_code << std::endl;
        std::cout << "Attempts made: " << (response.headers.count("X-Retry-Count") ? 
                                         response.headers.at("X-Retry-Count") : "unknown") 
                  << std::endl;
    }

    // Parallel Connections Example
    {
        std::cout << "\n=== Parallel Connections Example ===" << std::endl;
        
        const int num_parallel = 3;
        std::vector<std::thread> threads;
        std::atomic<int> successful_requests{0};
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Launch parallel requests
        for (int i = 0; i < num_parallel; i++) {
            threads.emplace_back([i, &successful_requests]() {
                auto response = Network::Get("https://api.github.com/zen");
                if (response.success) {
                    std::cout << "Thread " << i << " success: " << response.body << std::endl;
                    successful_requests++;
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        std::cout << "All parallel requests completed in " << duration.count() << "ms" << std::endl;
        std::cout << "Successful requests: " << successful_requests << "/" << num_parallel << std::endl;
    }

    Network::Cleanup();
    return 0;
}
