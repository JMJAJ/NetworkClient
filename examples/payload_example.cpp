#include "Network.hpp"
#include <iostream>
#include <map>

// Helper function to create URL-encoded form data
std::string createFormData(const std::map<std::string, std::string>& data) {
    std::string result;
    for (const auto& [key, value] : data) {
        if (!result.empty()) result += "&";
        result += Network::UrlEncode(key) + "=" + Network::UrlEncode(value);
    }
    return result;
}

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // Form data example
    {
        std::cout << "\n=== Form Data Example ===" << std::endl;
        
        // Create form data
        std::map<std::string, std::string> formData = {
            {"username", "john_doe"},
            {"email", "john@example.com"},
            {"message", "Hello, World!"}
        };
        
        std::string payload = createFormData(formData);
        
        auto response = Network::Post(
            "https://httpbin.org/post",
            payload,
            "application/x-www-form-urlencoded"
        );
        
        std::cout << "Form submission status: " << response.status_code << std::endl;
        std::cout << "Response: " << response.body << std::endl;
    }

    // JSON payload example
    {
        std::cout << "\n=== JSON Payload Example ===" << std::endl;
        
        // Create JSON payload
        std::string jsonPayload = R"({
            "user": {
                "name": "John Doe",
                "age": 30,
                "email": "john@example.com",
                "preferences": {
                    "newsletter": true,
                    "theme": "dark"
                },
                "interests": ["programming", "networking", "security"]
            }
        })";
        
        auto response = Network::Post(
            "https://httpbin.org/post",
            jsonPayload,
            "application/json"
        );
        
        std::cout << "JSON submission status: " << response.status_code << std::endl;
        std::cout << "Response: " << response.body << std::endl;
    }

    // File upload example
    {
        std::cout << "\n=== File Upload Example ===" << std::endl;
        
        // Simulate file content
        std::string fileContent = "This is the content of the file\nLine 2\nLine 3";
        
        // Create multipart form data
        std::string boundary = "------------------------boundary123456789";
        std::string multipartPayload = 
            "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n" +
            fileContent + "\r\n"
            "--" + boundary + "--\r\n";
        
        Network::RequestConfig config;
        config.additional_headers["Content-Type"] = "multipart/form-data; boundary=" + boundary;
        
        auto response = Network::Post(
            "https://httpbin.org/post",
            multipartPayload,
            "",  // Content-Type is set in headers
            config
        );
        
        std::cout << "File upload status: " << response.status_code << std::endl;
        std::cout << "Response: " << response.body << std::endl;
    }

    // PATCH with JSON example
    {
        std::cout << "\n=== PATCH with JSON Example ===" << std::endl;
        
        std::string patchPayload = R"({
            "op": "replace",
            "path": "/user/name",
            "value": "John Smith"
        })";
        
        Network::RequestConfig config;
        config.additional_headers["Content-Type"] = "application/json-patch+json";
        
        auto response = Network::Request(
            Network::Method::HTTP_PATCH,
            "https://httpbin.org/patch",
            patchPayload,
            config
        );
        
        std::cout << "PATCH status: " << response.status_code << std::endl;
        std::cout << "Response: " << response.body << std::endl;
    }

    Network::Cleanup();
    return 0;
}
