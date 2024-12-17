/**
 * @file example.cpp
 * @brief Test suite for the Network class library
 * 
 * This file contains a comprehensive test suite for the Network class,
 * which provides HTTP communication capabilities using WinINet.
 * The tests cover basic HTTP methods, security features, error handling,
 * and performance characteristics.
 * 
 * Features tested:
 * - Basic HTTP methods (GET, POST, PUT, PATCH, DELETE)
 * - Security validation (SSL, domain validation)
 * - Error handling (invalid ports, protocols)
 * - Performance (rate limiting, timeouts)
 * - Retry mechanism
 * - Large payload handling
 * 
 * Usage:
 * 1. Build and run the executable
 * 2. Tests will run automatically and generate a detailed report
 * 3. Check the "ISSUES REQUIRING ATTENTION" section for any failures
 * 
 * Test endpoints used:
 * - httpbin.org: For basic HTTP method testing
 * - httpstat.us: For status code testing
 * - postman-echo.com: For payload testing
 * 
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
#include <functional>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <map>

/**
 * @brief Helper struct to store test results
 * 
 * This struct stores all relevant information about a single test execution,
 * including timing information and error messages if any.
 */
struct TestResult {
    std::string name;
    bool success;
    int statusCode;
    std::string error;
    double duration;
    std::string body;
};

// Global test results storage
std::vector<TestResult> test_results;

// Forward declarations
void printTestReport();
void printDetailedResult(const TestResult& result);
TestResult runHttpMethodTest(const std::string& name,
                           Network::Method method,
                           const std::string& url,
                           const std::optional<std::string>& payload = std::nullopt,
                           const std::map<std::string, std::string>& headers = {});

/**
 * @brief Measures the duration of a function execution
 * 
 * @tparam Func Type of the function to measure
 * @param func Function to execute and measure
 * @return std::pair<ReturnType, double> Pair of function result and duration in milliseconds
 */
template<typename Func>
auto measureDuration(Func&& func) {
    auto start = std::chrono::steady_clock::now();
    auto result = std::forward<Func>(func)();
    auto end = std::chrono::steady_clock::now();
    return std::make_pair(
        result,
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
    );
}

/**
 * @brief Prints a formatted test response
 * 
 * @param response Network response from the test
 * @param testName Name of the test
 * @param duration_ms Duration of the test in milliseconds
 */
void printResponse(const Network::NetworkResponse& response, 
                  const std::string& testName, 
                  double duration_ms) {
    std::cout << "\n=== " << testName << " ===\n"
              << "Status Code: " << response.status_code << "\n"
              << "Success: " << (response.success ? "Yes" : "No") << "\n";
    
    if (!response.error_message.empty()) {
        std::cout << "Error: " << response.error_message << "\n";
    }
    
    std::cout << "Duration: " << duration_ms << "ms\n"
              << "Body (first 150 chars): " << response.body.substr(0, 150) << "...\n"
              << "Headers:\n================\n\n";

    // Store test result
    test_results.push_back({
        testName,
        response.success,
        response.status_code,
        response.error_message,
        duration_ms,
        response.body
    });
}

/**
 * @brief Tests basic HTTP methods
 * 
 * Tests GET, POST, PUT, PATCH, and DELETE methods to ensure proper
 * request formatting and response handling.
 */
void testHttpMethods() {
    std::cout << "Testing all HTTP methods...\n";

    // GET Request
    runHttpMethodTest("GET Request", 
                     Network::Method::HTTP_GET,
                     "https://httpbin.org/get");

    // POST Request
    runHttpMethodTest("POST Request",
                     Network::Method::HTTP_POST,
                     "https://reqres.in/api/users",
                     R"({"name": "test", "job": "developer"})",
                     {{"Content-Type", "application/json"}});

    // PUT Request
    runHttpMethodTest("PUT Request",
                     Network::Method::HTTP_PUT,
                     "https://reqres.in/api/users/2",
                     R"({"name": "updated", "job": "developer"})",
                     {{"Content-Type", "application/json"}});

    // PATCH Request
    runHttpMethodTest("PATCH Request",
                     Network::Method::HTTP_PATCH,
                     "https://reqres.in/api/users/2",
                     R"({"name": "patched"})",
                     {{"Content-Type", "application/json"}});

    // DELETE Request
    runHttpMethodTest("DELETE Request",
                     Network::Method::HTTP_DELETE,
                     "https://reqres.in/api/users/2");
}

/**
 * @brief Tests SSL/TLS security features
 * 
 * Verifies that the Network class properly handles SSL/TLS connections
 * and validates certificates.
 */
void testSecurity() {
    std::cout << "Testing SSL/TLS security features...\n";

    Network::RequestConfig config;
    config.verify_ssl = true;
    config.use_tls12_or_higher = true;

    auto [response, duration] = measureDuration([&]() {
        return Network::Request(
            Network::Method::HTTP_GET,
            "https://expired.badssl.com/",
            std::nullopt,
            config
        );
    });

    printResponse(response, "Strict SSL Test (Should Fail)", duration);
    std::cout << (response.success ? "✗ SSL verification failed to detect invalid certificate\n" 
                                 : "✓ SSL verification working correctly!\n");
}

/**
 * @brief Tests rate limiting
 * 
 * Verifies that the Network class properly enforces rate limiting
 * and handles requests accordingly.
 */
void testRateLimiting() {
    std::cout << "Testing rate limiting...\n";
    
    Network::RequestConfig config;
    config.rate_limit_per_minute = 30;  // 1 request per 2 seconds

    for (int i = 1; i <= 3; i++) {
        auto [response, duration] = measureDuration([&]() {
            return Network::Request(
                Network::Method::HTTP_GET,
                "https://httpbin.org/get?test=" + std::to_string(i),
                std::nullopt,
                config
            );
        });

        printResponse(response, "Rate Limited Request #" + std::to_string(i), duration);

        if (i > 1) {
            std::cout << "\nTime between requests: " << duration << "ms "
                     << "(Target: 2000ms, Deviation: " 
                     << std::abs(2000 - duration) << "ms)\n";
        }
    }
}

/**
 * @brief Tests timeout handling
 * 
 * Verifies that the Network class properly handles timeouts
 * and reports errors accordingly.
 */
TestResult testTimeout() {
    TestResult result;
    result.name = "Timeout Test (Should Fail)";
    
    auto [response, duration] = measureDuration([&]() {
        Network::RequestConfig config;
        config.timeout_seconds = 1;
        config.max_retries = 0;
        return Network::Request(
            Network::Method::HTTP_GET,
            "https://httpstat.us/200?sleep=5000",
            std::nullopt,
            config
        );
    });

    result.duration = duration;
    result.success = (duration <= 1500);
    result.statusCode = response.status_code;
    result.error = response.success ? "" : "Request timed out";
    result.body = response.body;

    if (duration > 1500) {
        std::cout << "\n✗ Timeout took too long: " << duration << "ms (should be ~1000ms)\n";
    } else if (duration < 800) {
        std::cout << "\n✗ Timeout too quick: " << duration << "ms (should be ~1000ms)\n";
    } else {
        std::cout << "\n✓ Timeout occurred as expected around 1000ms\n";
    }

    return result;
}

/**
 * @brief Tests error handling
 * 
 * Verifies that the Network class properly handles various errors
 * such as invalid domains, ports, and protocols.
 */
void testErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    // Invalid Domain
    runHttpMethodTest("Invalid Domain Test",
                     Network::Method::HTTP_GET,
                     "https://this-domain-definitely-does-not-exist-123.com");

    // Invalid Port - Using port 0 which is reserved and should always fail
    {
        Network::RequestConfig config;
        config.timeout_seconds = 5;  // Longer timeout for connection attempts
        
        auto [response, duration] = measureDuration([&]() {
            return Network::Request(
                Network::Method::HTTP_GET,
                "https://httpbin.org:0/get",  // Port 0 is reserved and should fail
                std::nullopt,
                config
            );
        });

        printResponse(response, "Invalid Port Test", duration);
        
        if (response.success || response.status_code != 0) {
            std::cout << "✗ Invalid port test unexpectedly succeeded\n";
        } else {
            std::cout << "✓ Invalid port test failed as expected\n";
        }
    }

    // Invalid Protocol
    runHttpMethodTest("Invalid Protocol Test",
                     Network::Method::HTTP_GET,
                     "invalid://httpbin.org/get");
}

/**
 * @brief Tests custom headers
 * 
 * Verifies that the Network class properly handles custom headers
 * in requests.
 */
void testCustomHeaders() {
    std::cout << "Testing custom headers...\n";

    Network::RequestConfig config;
    config.additional_headers = {
        {"X-Custom-Header", "test-value"},
        {"Accept", "application/json"}
    };

    runHttpMethodTest("Custom Headers Test",
                     Network::Method::HTTP_GET,
                     "https://jsonplaceholder.typicode.com/todos/1",
                     std::nullopt,
                     config.additional_headers);
}

/**
 * @brief Tests large payload handling
 * 
 * Verifies that the Network class properly handles large payloads
 * in requests and responses.
 */
void testLargePayload() {
    std::cout << "Testing large payload handling...\n";
    
    runHttpMethodTest("Large Payload Test",
                     Network::Method::HTTP_GET,
                     "https://httpbin.org/bytes/5000");
}

/**
 * @brief Tests retry mechanism
 * 
 * Verifies that the Network class properly handles retries
 * for failed requests.
 */
void testRetryMechanism() {
    std::cout << "Testing retry mechanism...\n";

    Network::RequestConfig config;
    config.max_retries = 3;
    config.timeout_seconds = 10;
    config.retry_delay_ms = 1000;

    auto [response, duration] = measureDuration([&]() {
        return Network::Request(
            Network::Method::HTTP_GET,
            "https://httpbin.org/status/500",  // Using httpbin instead
            std::nullopt,
            config
        );
    });

    // For retry test, we expect a 500 status code
    bool isSuccess = response.status_code == 500 && duration >= 3000.0;

    // Add test result to collection
    test_results.push_back({
        "Retry Mechanism Test",
        isSuccess,  // Success if we got 500 and proper duration
        response.status_code,
        isSuccess ? "" : "Unexpected response or duration",
        static_cast<double>(duration),
        response.body
    });

    // Analyze retry timing
    if (response.status_code == 500) {
        std::cout << "\n✓ Retry mechanism executed with proper status code (500)\n";
        std::cout << "Total duration with retries: " << duration << "ms\n";
        
        // Expected minimum duration with 3 retries and 1000ms delay
        double expected_min_duration = 3000.0; // 3 retries * 1000ms
        if (duration >= expected_min_duration) {
            std::cout << "✓ Duration indicates proper retry delays\n";
        } else {
            std::cout << "✗ Duration too short for proper retry delays\n";
            std::cout << "  Expected at least: " << expected_min_duration << "ms\n";
            std::cout << "  Got: " << duration << "ms\n";
        }
    } else {
        std::cout << "\n✗ Unexpected response from retry test\n"
                 << "   Expected: status 500\n"
                 << "   Got: status " << response.status_code << "\n";
    }
}

/**
 * @brief Prints a detailed test result
 * 
 * @param result The test result to print
 */
void printDetailedResult(const TestResult& result) {
    std::cout << std::setw(40) << std::left << result.name 
              << std::setw(10) << (result.success ? "PASS" : "FAIL")
              << std::setw(15) << result.statusCode
              << std::setw(15) << std::fixed << std::setprecision(3) << result.duration
              << (result.error.empty() ? "" : result.error) << "\n";
}

/**
 * @brief Generates and prints the test report header
 * 
 * @param test_results Vector of all test results
 */
void printTestReport() {
    const char* separator = "===========================================";
    const char* title =    "           NETWORK CLASS TEST REPORT           ";
    const char* subtitle = "        Internet Communication Library         ";
    const char* version =  "               Version 1.0.0                  ";
    
    std::cout << "\n" << separator << "\n"
              << title << "\n"
              << subtitle << "\n"
              << version << "\n"
              << separator << "\n\n";

    // Count statistics
    int totalTests = test_results.size();
    int passedTests = std::count_if(test_results.begin(), test_results.end(),
        [](const TestResult& r) { return r.success; });
    int failedTests = totalTests - passedTests;
    
    // Calculate total duration and average
    double totalDuration = std::accumulate(test_results.begin(), test_results.end(), 0.0,
        [](double sum, const TestResult& r) { return sum + r.duration; });
    double avgDuration = totalDuration / totalTests;

    // Print summary with better formatting
    std::cout << "EXECUTION SUMMARY:\n"
              << "─────────────────\n"
              << std::setw(25) << std::left << "Total Tests Executed:" << std::right << totalTests << "\n"
              << std::setw(25) << std::left << "Tests Passed:" << std::right << passedTests 
              << " (" << std::fixed << std::setprecision(1) << (passedTests * 100.0 / totalTests) << "%)\n"
              << std::setw(25) << std::left << "Tests Failed:" << std::right << failedTests 
              << " (" << std::fixed << std::setprecision(1) << (failedTests * 100.0 / totalTests) << "%)\n"
              << std::setw(25) << std::left << "Total Duration:" << std::right << std::fixed 
              << std::setprecision(2) << totalDuration << "ms\n"
              << std::setw(25) << std::left << "Average Duration:" << std::right << std::fixed 
              << std::setprecision(2) << avgDuration << "ms\n\n";

    // Status code distribution
    std::cout << "STATUS CODE DISTRIBUTION:\n"
              << "─────────────────────────\n";
    std::map<int, int> statusCodes;
    for (const auto& result : test_results) {
        statusCodes[result.statusCode]++;
    }
    
    for (const auto& [code, count] : statusCodes) {
        std::cout << std::setw(8) << code << ": " << std::setw(3) << count << " tests"
                  << " (" << std::fixed << std::setprecision(1) 
                  << (count * 100.0 / totalTests) << "%)\n";
    }
    std::cout << "\n";

    // Detailed results
    std::cout << "DETAILED RESULTS:\n"
              << "────────────────\n"
              << std::setw(40) << std::left << "Test Name" 
              << std::setw(10) << "Status"
              << std::setw(15) << "Code"
              << std::setw(15) << "Duration"
              << "Error\n"
              << std::string(100, '─') << "\n\n";

    // Group tests by category
    std::cout << "BASIC HTTP METHODS:\n";
    for (const auto& result : test_results) {
        if (result.name.find("Request") != std::string::npos && 
            result.name.find("Rate Limited") == std::string::npos) {
            printDetailedResult(result);
        }
    }

    std::cout << "\nSECURITY & VALIDATION:\n";
    for (const auto& result : test_results) {
        if (result.name.find("SSL") != std::string::npos ||
            result.name.find("Invalid") != std::string::npos) {
            printDetailedResult(result);
        }
    }

    std::cout << "\nPERFORMANCE TESTS:\n";
    for (const auto& result : test_results) {
        if (result.name.find("Rate Limited") != std::string::npos ||
            result.name.find("Timeout") != std::string::npos ||
            result.name.find("Large Payload") != std::string::npos ||
            result.name.find("Retry Mechanism") != std::string::npos) {
            printDetailedResult(result);
        }
    }

    // Print issues requiring attention
    std::vector<std::string> issues;
    for (const auto& result : test_results) {
        if (!result.success) {
            std::string issue = result.name + ": " + result.error;
            if (result.name.find("Timeout") != std::string::npos) {
                issue += " (took " + std::to_string(static_cast<int>(result.duration)) + "ms)";
            }
            issues.push_back(issue);
        }
    }

    if (!issues.empty()) {
        std::cout << "\nISSUES REQUIRING ATTENTION:\n"
                  << "──────────────────────────\n";
        for (const auto& issue : issues) {
            std::cout << "❌ " << issue << "\n";
        }
    }

    std::cout << "\n" << separator << "\n"
              << "                END OF REPORT                \n"
              << separator << "\n\n";
}

/**
 * @brief Main entry point for the test suite
 * 
 * Initializes the Network class, runs all tests, and generates
 * a comprehensive report of the results.
 */
int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network\n";
        return 1;
    }

    try {
        std::cout << "Starting comprehensive Network class tests...\n\n";

        // Run all tests
        testHttpMethods();
        testSecurity();
        testRateLimiting();
        test_results.push_back(testTimeout());
        testCustomHeaders();
        testErrorHandling();
        testLargePayload();
        testRetryMechanism();

        // Print final report
        printTestReport();
        std::cout << "All network tests completed.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error during testing: " << e.what() << "\n";
        return 1;
    }

    Network::Cleanup();
    return 0;
}

/**
 * @brief Implementation of runHttpMethodTest helper
 * 
 * @param name Name of the test
 * @param method HTTP method to test
 * @param url URL for the test request
 * @param payload Optional payload for the request
 * @param headers Optional headers for the request
 * @return TestResult Result of the test
 */
TestResult runHttpMethodTest(const std::string& name,
                           Network::Method method,
                           const std::string& url,
                           const std::optional<std::string>& payload,
                           const std::map<std::string, std::string>& headers) {
    Network::RequestConfig config;
    config.additional_headers = headers;

    auto [response, duration] = measureDuration([&]() {
        return Network::Request(method, url, payload, config);
    });

    printResponse(response, name, duration);
    return {name, response.success, response.status_code, 
            response.error_message, static_cast<double>(duration), response.body};
}