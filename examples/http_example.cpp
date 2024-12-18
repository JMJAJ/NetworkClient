#include "Network.hpp"
#include <iostream>

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // Basic GET request
    {
        auto response = Network::Get("http://example.com");
        std::cout << "\n=== Basic GET Request ===" << std::endl;
        std::cout << "Status: " << response.status_code << std::endl;
        std::cout << "Body size: " << response.body.size() << " bytes" << std::endl;
    }

    // GET request with custom headers
    {
        Network::RequestConfig config;
        config.additional_headers["User-Agent"] = "MyCustomAgent/1.0";
        config.additional_headers["Accept"] = "text/html";

        auto response = Network::Get("http://httpbin.org/headers", config);
        std::cout << "\n=== GET with Custom Headers ===" << std::endl;
        std::cout << "Response: " << response.body << std::endl;
    }

    // POST with form data
    {
        std::string payload = "name=John&age=30";
        auto response = Network::Post(
            "http://httpbin.org/post",
            payload,
            "application/x-www-form-urlencoded"
        );
        std::cout << "\n=== POST with Form Data ===" << std::endl;
        std::cout << "Response: " << response.body << std::endl;
    }

    // POST with JSON
    {
        std::string json_payload = R"({
            "name": "John",
            "age": 30,
            "city": "New York"
        })";
        
        auto response = Network::Post(
            "http://httpbin.org/post",
            json_payload,
            "application/json"
        );
        std::cout << "\n=== POST with JSON ===" << std::endl;
        std::cout << "Response: " << response.body << std::endl;
    }

    // PUT request
    {
        std::string payload = R"({
            "updated_name": "John Smith"
        })";
        
        auto response = Network::Put(
            "http://httpbin.org/put",
            payload,
            "application/json"
        );
        std::cout << "\n=== PUT Request ===" << std::endl;
        std::cout << "Status: " << response.status_code << std::endl;
    }

    // DELETE request
    {
        auto response = Network::Delete("http://httpbin.org/delete");
        std::cout << "\n=== DELETE Request ===" << std::endl;
        std::cout << "Status: " << response.status_code << std::endl;
    }

    Network::Cleanup();
    return 0;
}
