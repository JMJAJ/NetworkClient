#include "Network.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    std::atomic<int> completed_requests{0};

    // Async GET request
    std::cout << "Starting async GET request..." << std::endl;
    Network::GetAsync(
        "https://api.github.com/zen",
        [&completed_requests](const Network::NetworkResponse& response) {
            std::cout << "\n=== Async GET Response ===" << std::endl;
            std::cout << "Status: " << response.status_code << std::endl;
            std::cout << "Body: " << response.body << std::endl;
            completed_requests++;
        }
    );

    // Async POST request
    std::cout << "Starting async POST request..." << std::endl;
    std::string json_payload = R"({
        "title": "Test Issue",
        "body": "This is a test issue created via async API call"
    })";

    Network::PostAsync(
        "https://api.github.com/repos/octocat/Hello-World/issues",
        json_payload,
        "application/json",
        [&completed_requests](const Network::NetworkResponse& response) {
            std::cout << "\n=== Async POST Response ===" << std::endl;
            std::cout << "Status: " << response.status_code << std::endl;
            completed_requests++;
        }
    );

    // Multiple parallel requests
    std::cout << "\nStarting multiple parallel requests..." << std::endl;
    const int num_parallel_requests = 5;
    
    for (int i = 0; i < num_parallel_requests; i++) {
        Network::GetAsync(
            "https://api.github.com/users/octocat",
            [i, &completed_requests](const Network::NetworkResponse& response) {
                std::cout << "\n=== Parallel Request " << i << " Response ===" << std::endl;
                std::cout << "Status: " << response.status_code << std::endl;
                completed_requests++;
            }
        );
    }

    // Wait for all requests to complete
    while (completed_requests < num_parallel_requests + 2) {
        std::cout << "Waiting for requests to complete... " 
                  << completed_requests << " done" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "\nAll requests completed!" << std::endl;

    Network::Cleanup();
    return 0;
}
