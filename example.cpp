#include <iostream>
#include <cassert>
#include <string>
#include "net/Network.hpp"

// Test function declarations
void testHttpGet();
void testHttpsGet();
void testHttpPost();
void testHttpsPost();
void testCustomHeaders();
void testRedirects();
void testTimeout();
void testHttpsSecurity();

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network!" << std::endl;
        return 1;
    }

    try {
        std::cout << "Running Network Tests...\n" << std::endl;
        
        std::cout << "1. Testing HTTP GET..." << std::endl;
        testHttpGet();
        
        std::cout << "\n2. Testing HTTPS GET..." << std::endl;
        testHttpsGet();
        
        std::cout << "\n3. Testing HTTP POST..." << std::endl;
        testHttpPost();
        
        std::cout << "\n4. Testing HTTPS POST..." << std::endl;
        testHttpsPost();
        
        std::cout << "\n5. Testing Custom Headers..." << std::endl;
        testCustomHeaders();
        
        std::cout << "\n6. Testing Redirects..." << std::endl;
        testRedirects();
        
        std::cout << "\n7. Testing Timeout..." << std::endl;
        testTimeout();
        
        std::cout << "\n8. Testing HTTPS Security..." << std::endl;
        testHttpsSecurity();

        std::cout << "\nAll tests completed successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        Network::Cleanup();
        return 1;
    }

    Network::Cleanup();
    return 0;
}

void testHttpGet() {
    auto response = Network::Get("http://httpbin.org/get");
    assert(response.success && "HTTP GET request failed");
    assert(response.status_code == 200 && "Expected status code 200");
    assert(!response.body.empty() && "Response body should not be empty");
    std::cout << "HTTP GET test passed!" << std::endl;
}

void testHttpsGet() {
    // Test basic HTTPS GET
    auto response = Network::Get("https://httpbin.org/get");
    assert(response.success && "HTTPS GET request failed");
    assert(response.status_code == 200 && "Expected status code 200");
    assert(!response.body.empty() && "Response body should not be empty");
    
    // Test HTTPS with query parameters
    response = Network::Get("https://httpbin.org/get?param1=test&param2=value");
    assert(response.success && "HTTPS GET with query params failed");
    assert(response.body.find("param1") != std::string::npos && "Query parameter not found in response");
    
    // Test HTTPS with different ports
    response = Network::Get("https://httpbin.org:443/get");
    assert(response.success && "HTTPS GET with explicit port failed");
    
    // Test HTTPS with SSL certificate validation
    response = Network::Get("https://expired.badssl.com/");
    assert(!response.success && "SSL certificate validation failed - accepted invalid certificate");
    
    std::cout << "HTTPS GET tests passed!" << std::endl;
}

void testHttpPost() {
    std::string payload = "test=data&key=value";
    auto response = Network::Post("http://httpbin.org/post", payload);
    assert(response.success && "HTTP POST request failed");
    assert(response.status_code == 200 && "Expected status code 200");
    assert(response.body.find("test") != std::string::npos && "POST data not found in response");
    std::cout << "HTTP POST test passed!" << std::endl;
}

void testHttpsPost() {
    // Test HTTPS POST with JSON
    std::string json_payload = "{\"test\":\"data\",\"secure\":true}";
    Network::RequestConfig config;
    config.additional_headers["Content-Type"] = "application/json";
    config.additional_headers["Accept"] = "application/json";
    
    auto response = Network::Post("https://httpbin.org/post", json_payload, "application/json", config);
    assert(response.success && "HTTPS POST request failed");
    assert(response.status_code == 200 && "Expected status code 200");
    assert(response.body.find("test") != std::string::npos && "POST data not found in response");
    
    // Test HTTPS POST with form data
    std::string form_payload = "field1=value1&field2=value2";
    config.additional_headers["Content-Type"] = "application/x-www-form-urlencoded";
    response = Network::Post("https://httpbin.org/post", form_payload, "application/x-www-form-urlencoded", config);
    assert(response.success && "HTTPS POST with form data failed");
    assert(response.body.find("field1") != std::string::npos && "Form data not found in response");
    
    // Test HTTPS POST with large payload
    std::string large_payload(50000, 'X'); // 50KB payload
    response = Network::Post("https://httpbin.org/post", large_payload, "text/plain", config);
    assert(response.success && "HTTPS POST with large payload failed");
    
    std::cout << "HTTPS POST tests passed!" << std::endl;
}

void testCustomHeaders() {
    Network::RequestConfig config;
    config.additional_headers["X-Custom-Header"] = "test-value";
    
    auto response = Network::Get("https://httpbin.org/headers", config);
    assert(response.success && "Custom headers request failed");
    assert(response.body.find("X-Custom-Header") != std::string::npos && "Custom header not found in response");
    std::cout << "Custom headers test passed!" << std::endl;
}

void testRedirects() {
    Network::RequestConfig config;
    config.follow_redirects = true;
    config.max_redirects = 5;
    
    auto response = Network::Get("https://httpbin.org/redirect/2", config);
    assert(response.success && "Redirect request failed");
    assert(response.status_code == 200 && "Expected status code 200 after redirect");
    std::cout << "Redirects test passed!" << std::endl;
}

void testTimeout() {
    Network::RequestConfig config;
    config.timeout_seconds = 1; // Short timeout
    
    auto response = Network::Get("https://httpbin.org/delay/2", config);
    assert(!response.success && "Expected timeout failure");
    std::cout << "Timeout test passed!" << std::endl;
}

void testHttpsSecurity() {
    Network::RequestConfig config;
    
    // Test HTTPS upgrade (HTTP to HTTPS redirect)
    auto response = Network::Get("http://httpbin.org/redirect-to?url=https://httpbin.org/get");
    assert(response.success && "HTTPS upgrade redirect failed");
    assert(response.status_code == 200 && "Expected status code 200 after HTTPS upgrade");
    
    // Test HSTS (HTTP Strict Transport Security)
    response = Network::Get("https://httpbin.org/response-headers?Strict-Transport-Security=max-age=31536000");
    assert(response.success && "HSTS header test failed");
    assert(response.headers.find("Strict-Transport-Security") != response.headers.end() && "HSTS header not found");
    
    std::cout << "HTTPS security tests passed!" << std::endl;
}