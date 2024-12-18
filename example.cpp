/**
 * @file main.cpp
 * @brief Comprehensive test suite and validation framework for the Network library
 * 
 * This file contains an extensive test suite that validates all aspects of the Network library,
 * ensuring its reliability, security, and performance. The tests are designed to catch common
 * issues and edge cases that users might encounter in production environments.
 * 
 * Test Categories:
 * 1. Basic HTTP Methods (GET, POST, PUT, PATCH, DELETE)
 * 2. Security Features (SSL/TLS, API Keys, Certificate Validation)
 * 3. Performance (Rate Limiting, Timeouts, Connection Pooling)
 * 4. Error Handling (Invalid URLs, Network Issues, Timeouts)
 * 5. Advanced Features (Async Operations, WebSocket, Compression)
 * 6. Edge Cases (Large Payloads, Special Characters, Unicode)
 * 
 * Each test provides detailed output and metrics, making it easy to identify:
 * - Functionality issues
 * - Performance bottlenecks
 * - Security vulnerabilities
 * - Compatibility problems
 * 
 * Usage:
 * 1. Build the project in Release mode
 * 2. Run the executable
 * 3. Check the test report for any failures
 * 4. Review performance metrics
 * 
 * @note This test suite requires internet connectivity and access to test endpoints
 * @version 1.1.0
 * @author Jxint
 * @date December 2024
 */

/**
 * Network Class Test Suite
 * 
 * This file contains comprehensive tests for the Network class, testing various
 * HTTP methods, security features, performance characteristics, and error handling.
 * 
 * Test Categories:
 * 1. Basic HTTP Methods (GET, POST, PUT, PATCH, DELETE)
 * 2. Security Features (SSL/TLS)
 * 3. Performance (Rate Limiting, Timeouts)
 * 4. Error Handling
 * 5. Edge Cases (Large Payloads, Custom Headers)
 */

#include "net/Network.hpp"
#include "net/WebSocket.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <future>

// Helper function to print section headers
void printSection(const std::string& section) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "  " << section << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

// Helper function to print test results
void printTestResult(const std::string& test, bool success, const std::string& details = "") {
    std::cout << std::setw(40) << std::left << test << ": "
              << (success ? "\033[32mPASSED\033[0m" : "\033[31mFAILED\033[0m");
    if (!details.empty()) {
        std::cout << " - " << details;
    }
    std::cout << std::endl;
}

class NetworkTester {
public:
    static void runAllTests() {
        if (!Network::Initialize()) {
            std::cerr << "Failed to initialize network" << std::endl;
            return;
        }

        printSection("Network Library Comprehensive Test Report");
        std::cout << "Testing all features of the Network Library...\n" << std::endl;

        testBasicHTTP();
        testHTTPS();
        testWebSocket();
        testAsyncRequests();
        testSecurity();
        testEncoding();
        testHeaders();
        testConnectionManagement();
        testErrorHandling();
        testRateLimiting();
        testPerformanceMetrics();
        testConcurrentRequests();
        testContentTypes();
        testCompressionHandling();
        testLoadBalancing();

        printSection("Test Summary");
        std::cout << "Total Tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
        std::cout << "Success Rate: " << (passedTests * 100.0 / totalTests) << "%" << std::endl;

        Network::Cleanup();
    }

private:
    static void testBasicHTTP() {
        printSection("Basic HTTP Operations");

        // GET request
        {
            auto response = Network::Get("http://example.com");
            bool success = response.status_code == 200;
            recordTest("GET Request", success);
        }

        // POST request
        {
            std::string payload = "name=test&value=123";
            auto response = Network::Post(
                "http://httpbin.org/post",
                payload,
                "application/x-www-form-urlencoded"
            );
            bool success = response.status_code == 200;
            recordTest("POST Request", success);
        }

        // PUT request
        {
            std::string payload = R"({"name": "test"})";
            auto response = Network::Put(
                "http://httpbin.org/put",
                payload,
                "application/json"
            );
            bool success = response.status_code == 200;
            recordTest("PUT Request", success);
        }
    }

    static void testHTTPS() {
        printSection("HTTPS and SSL/TLS");

        Network::RequestConfig config;
        config.verify_ssl = true;
        config.use_tls12_or_higher = true;

        auto response = Network::Get("https://httpbin.org/get", config);
        bool success = response.status_code == 200;
        recordTest("HTTPS with TLS 1.2+", success);
    }

    static void testWebSocket() {
        printSection("WebSocket Communication");

        // Note: WebSocket implementation is currently in progress
        recordTest("WebSocket Connection", false);
        recordTest("WebSocket Message Exchange", false);
        
        std::cout << "\nNote: WebSocket functionality is currently under development." << std::endl;
        std::cout << "The following features are planned:" << std::endl;
        std::cout << "- Secure WebSocket (WSS) support" << std::endl;
        std::cout << "- Automatic reconnection" << std::endl;
        std::cout << "- Message compression" << std::endl;
        std::cout << "- Binary message support" << std::endl;
    }

    static void testAsyncRequests() {
        printSection("Asynchronous Operations");

        std::atomic<bool> callbackCalled{false};
        Network::GetAsync("https://httpbin.org/get",
            [&callbackCalled](const Network::NetworkResponse& response) {
                callbackCalled = true;
            }
        );

        // Wait for callback
        std::this_thread::sleep_for(std::chrono::seconds(2));
        recordTest("Async Request", callbackCalled);
    }

    static void testSecurity() {
        printSection("Security Features");

        // API Key test
        {
            Network::RequestConfig config;
            config.api_key = "test_key";
            config.additional_headers["Authorization"] = "Bearer " + config.api_key;
            
            auto response = Network::Get("https://httpbin.org/headers", config);
            bool success = response.status_code == 200;
            recordTest("API Key Authentication", success);
        }

        // SSL Certificate validation
        {
            Network::RequestConfig config;
            config.verify_ssl = true;
            auto response = Network::Get("https://expired.badssl.com/", config);
            bool success = !response.success; // Should fail for invalid cert
            recordTest("SSL Certificate Validation", success);
        }
    }

    static void testEncoding() {
        printSection("Encoding Utilities");

        // URL Encoding
        {
            std::string encoded = Network::UrlEncode("Hello World!");
            bool success = encoded == "Hello%20World%21";
            recordTest("URL Encoding", success);
        }

        // Base64 Encoding
        {
            std::string encoded = Network::Base64Encode("Hello World!");
            bool success = !encoded.empty();
            recordTest("Base64 Encoding", success);
        }
    }

    static void testHeaders() {
        printSection("Header Management");

        Network::RequestConfig config;
        config.additional_headers = {
            {"User-Agent", "TestClient/1.0"},
            {"Accept", "application/json"}
        };

        auto response = Network::Get("https://httpbin.org/headers", config);
        bool success = response.status_code == 200;
        recordTest("Custom Headers", success);
    }

    static void testConnectionManagement() {
        printSection("Connection Management");

        // Connection pooling test
        {
            std::vector<double> times;
            for (int i = 0; i < 3; i++) {
                auto start = std::chrono::high_resolution_clock::now();
                auto response = Network::Get("https://httpbin.org/get");
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration<double>(end - start).count();
                times.push_back(duration);
            }

            // Second request should be faster due to connection reuse
            bool success = times[1] < times[0];
            recordTest("Connection Pooling", success);
        }

        // Timeout test
        {
            Network::RequestConfig config;
            config.timeout_seconds = 1;
            auto response = Network::Get("https://httpbin.org/delay/2", config);
            bool success = !response.success; // Should timeout
            recordTest("Connection Timeout", success);
        }
    }

    static void testErrorHandling() {
        printSection("Error Handling");

        // Invalid URL
        {
            auto response = Network::Get("not_a_valid_url");
            bool success = !response.success;
            recordTest("Invalid URL Handling", success);
        }

        // Non-existent host
        {
            auto response = Network::Get("http://this-domain-does-not-exist.com");
            bool success = !response.success;
            recordTest("Non-existent Host Handling", success);
        }
    }

    static void testRateLimiting() {
        printSection("Rate Limiting");

        std::cout << "\nTesting rate limiting:" << std::endl;

        // Configure rate limiting to 5 requests per minute
        Network::RequestConfig rateLimitConfig;
        rateLimitConfig.rate_limit_per_minute = 5;

        // Make 10 requests in quick succession
        int successCount = 0;
        int rateLimitCount = 0;
        
        for (int i = 0; i < 10; i++) {
            auto response = Network::Get("https://httpbin.org/get", rateLimitConfig);
            if (response.success) {
                successCount++;
            } else if (response.status_code == 429) {
                rateLimitCount++;
            }
        }

        // Verify that rate limiting worked
        bool rateLimitPassed = successCount == 5 && rateLimitCount == 5;
        std::cout << "Successful requests: " << successCount << "/5 (rate limit)" << std::endl;
        std::cout << "Rate limited requests: " << rateLimitCount << "/5" << std::endl;
        std::cout << "Rate Limiting                           : " << (rateLimitPassed ? "PASSED" : "FAILED") << std::endl;
        recordTest("Rate Limiting", rateLimitPassed);
    }

    static void testPerformanceMetrics() {
        printSection("Performance Metrics");

        // Test latency across different endpoints
        std::vector<std::string> endpoints = {
            "https://httpbin.org/get",
            "https://httpbin.org/delay/1",
            "https://httpbin.org/bytes/1000",
            "https://httpbin.org/stream-bytes/1000"
        };

        std::cout << "\nLatency Test Results:" << std::endl;
        std::cout << std::setw(40) << std::left << "Endpoint"
                  << std::setw(15) << "Latency(ms)"
                  << std::setw(15) << "Status"
                  << "Response Size" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (const auto& endpoint : endpoints) {
            auto start = std::chrono::high_resolution_clock::now();
            auto response = Network::Get(endpoint);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            std::cout << std::setw(40) << std::left << endpoint
                      << std::setw(15) << duration.count()
                      << std::setw(15) << response.status_code
                      << response.body.size() << " bytes" << std::endl;

            bool success = response.status_code == 200;
            recordTest("Latency Test - " + endpoint, success);
        }
    }

    static void testConcurrentRequests() {
        printSection("Concurrent Requests");

        const int NUM_REQUESTS = 5;
        std::vector<std::future<Network::NetworkResponse>> futures;
        std::atomic<int> successCount{0};
        
        auto start = std::chrono::high_resolution_clock::now();

        // Launch concurrent requests
        for (int i = 0; i < NUM_REQUESTS; i++) {
            futures.push_back(std::async(std::launch::async, []() {
                return Network::Get("https://httpbin.org/get");
            }));
        }

        // Collect and analyze results
        std::vector<std::chrono::milliseconds> responseTimes;
        for (auto& future : futures) {
            auto response = future.get();
            if (response.success) {
                successCount++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "\nConcurrent Request Results:" << std::endl;
        std::cout << "Total Requests: " << NUM_REQUESTS << std::endl;
        std::cout << "Successful Requests: " << successCount << std::endl;
        std::cout << "Total Time: " << totalDuration.count() << "ms" << std::endl;
        std::cout << "Average Time per Request: " << totalDuration.count() / NUM_REQUESTS << "ms" << std::endl;

        recordTest("Concurrent Requests", successCount == NUM_REQUESTS);
    }

    static void testContentTypes() {
        printSection("Content Type Handling");

        struct ContentTypeTest {
            std::string endpoint;
            std::string expectedType;
            std::string description;
        };

        std::vector<ContentTypeTest> tests = {
            {"https://httpbin.org/json", "application/json", "JSON Response"},
            {"https://httpbin.org/xml", "application/xml", "XML Response"},
            {"https://httpbin.org/html", "text/html", "HTML Response"},
            {"https://httpbin.org/image/jpeg", "image/jpeg", "JPEG Image"},
            {"https://httpbin.org/image/png", "image/png", "PNG Image"}
        };

        for (const auto& test : tests) {
            Network::RequestConfig config;
            config.additional_headers["Accept"] = test.expectedType;
            
            auto response = Network::Get(test.endpoint, config);
            
            // Look for content-type in a case-insensitive way
            std::string actualContentType = "not found";
            for (const auto& header : response.headers) {
                if (iequals(header.first, "content-type")) {
                    actualContentType = header.second;
                    break;
                }
            }

            bool hasCorrectType = actualContentType.find(test.expectedType) != std::string::npos;

            std::cout << "\nTesting " << test.description << ":" << std::endl;
            std::cout << "Endpoint: " << test.endpoint << std::endl;
            std::cout << "Expected Type: " << test.expectedType << std::endl;
            std::cout << "Actual Type: " << actualContentType << std::endl;
            std::cout << "Response Size: " << response.body.size() << " bytes" << std::endl;
            std::cout << "Status Code: " << response.status_code << std::endl;

            recordTest("Content Type - " + test.description, 
                      response.status_code == 200 && hasCorrectType);
        }
    }

    static void testCompressionHandling() {
        printSection("Compression Handling");

        struct CompressionTest {
            std::string encoding;
            std::string endpoint;
        };

        std::vector<CompressionTest> tests = {
            {"gzip", "https://httpbin.org/gzip"},
            {"deflate", "https://httpbin.org/deflate"},
            {"brotli", "https://httpbin.org/brotli"}
        };

        for (const auto& test : tests) {
            Network::RequestConfig config;
            config.additional_headers["Accept-Encoding"] = test.encoding;

            auto response = Network::Get(test.endpoint, config);
            
            std::cout << "\nTesting " << test.encoding << " compression:" << std::endl;
            std::cout << "Response Size: " << response.body.size() << " bytes" << std::endl;
            std::cout << "Status Code: " << response.status_code << std::endl;
            
            auto contentEncoding = response.headers.find("Content-Encoding");
            if (contentEncoding != response.headers.end()) {
                std::cout << "Content-Encoding: " << contentEncoding->second << std::endl;
            }

            bool success = response.status_code == 200;
            recordTest("Compression - " + test.encoding, success);
        }
    }

    static void testLoadBalancing() {
        printSection("Load Balancing and Failover");

        const int NUM_REQUESTS = 10;
        std::map<std::string, int> serverDistribution;
        int totalSuccessful = 0;
        
        std::cout << "\nTesting load distribution across requests:" << std::endl;
        
        for (int i = 0; i < NUM_REQUESTS; i++) {
            try {
                auto response = Network::Get("https://httpbin.org/get");
                if (response.success) {
                    totalSuccessful++;
                    
                    // Look for server header in a case-insensitive way
                    for (const auto& header : response.headers) {
                        if (iequals(header.first, "server")) {
                            serverDistribution[header.second]++;
                            break;
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Request " << i + 1 << " failed: " << e.what() << std::endl;
            }
        }

        std::cout << "\nServer Distribution:" << std::endl;
        for (const auto& [server, count] : serverDistribution) {
            double percentage = (count * 100.0) / totalSuccessful;
            std::cout << server << ": " << count << " requests ("
                      << std::fixed << std::setprecision(1) << percentage << "%)" << std::endl;
        }

        std::cout << "\nTotal successful requests: " << totalSuccessful << "/" << NUM_REQUESTS << std::endl;
        recordTest("Load Distribution", totalSuccessful > 0);
    }

    // Helper function for case-insensitive string comparison
    static bool iequals(const std::string& a, const std::string& b) {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(),
            [](char a, char b) {
                return tolower(a) == tolower(b);
            });
    }

    static void recordTest(const std::string& test, bool success) {
        printTestResult(test, success);
        totalTests++;
        if (success) passedTests++;
    }

    static int totalTests;
    static int passedTests;
};

int NetworkTester::totalTests = 0;
int NetworkTester::passedTests = 0;

int main() {
    NetworkTester::runAllTests();
    return 0;
}
