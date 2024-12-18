#include "Network.hpp"
#include <iostream>
#include <string>
#include <vector>

void printEncoded(const std::string& original, const std::string& encoded) {
    std::cout << "Original: " << original << std::endl;
    std::cout << "Encoded:  " << encoded << std::endl;
    std::cout << std::endl;
}

int main() {
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // URL Encoding Examples
    {
        std::cout << "=== URL Encoding Examples ===" << std::endl;
        
        // Basic text
        {
            std::string text = "Hello World";
            std::string encoded = Network::UrlEncode(text);
            std::cout << "Basic Text:" << std::endl;
            printEncoded(text, encoded);
        }
        
        // Special characters
        {
            std::string text = "Hello & World @ 2023!";
            std::string encoded = Network::UrlEncode(text);
            std::cout << "Special Characters:" << std::endl;
            printEncoded(text, encoded);
        }
        
        // Unicode characters
        {
            std::string text = "Hello 世界";
            std::string encoded = Network::UrlEncode(text);
            std::cout << "Unicode Characters:" << std::endl;
            printEncoded(text, encoded);
        }
        
        // Query parameters
        {
            std::string text = "key=value&other=data";
            std::string encoded = Network::UrlEncode(text);
            std::cout << "Query Parameters:" << std::endl;
            printEncoded(text, encoded);
        }
        
        // Path segments
        {
            std::string text = "/path/to/file.txt";
            std::string encoded = Network::UrlEncode(text);
            std::cout << "Path Segments:" << std::endl;
            printEncoded(text, encoded);
        }
    }

    // Base64 Encoding Examples
    {
        std::cout << "\n=== Base64 Encoding Examples ===" << std::endl;
        
        // Basic text
        {
            std::string text = "Hello World";
            std::string encoded = Network::Base64Encode(text);
            std::cout << "Basic Text:" << std::endl;
            printEncoded(text, encoded);
        }
        
        // Binary data
        {
            std::vector<uint8_t> binary = {0x00, 0xFF, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
            std::string binaryStr(binary.begin(), binary.end());
            std::string encoded = Network::Base64Encode(binaryStr);
            std::cout << "Binary Data:" << std::endl;
            std::cout << "Original (hex):  ";
            for (uint8_t b : binary) {
                printf("%02X ", b);
            }
            std::cout << "\nBase64 encoded: " << encoded << std::endl;
            std::cout << std::endl;
        }
        
        // Authentication credentials
        {
            std::string credentials = "username:password";
            std::string encoded = Network::Base64Encode(credentials);
            std::cout << "Authentication Credentials:" << std::endl;
            printEncoded(credentials, encoded);
        }
        
        // Long text
        {
            std::string text = "This is a longer text that will be encoded in Base64. "
                              "It contains multiple sentences and should demonstrate "
                              "how Base64 encoding handles longer inputs with various "
                              "characters and spaces.";
            std::string encoded = Network::Base64Encode(text);
            std::cout << "Long Text:" << std::endl;
            printEncoded(text, encoded);
        }
    }

    // Practical Examples
    {
        std::cout << "\n=== Practical Examples ===" << std::endl;
        
        // URL with query parameters
        {
            std::string baseUrl = "https://api.example.com/search";
            std::string query = "C++ Programming";
            std::string category = "tutorials & examples";
            
            std::string fullUrl = baseUrl + "?q=" + Network::UrlEncode(query) + 
                                "&category=" + Network::UrlEncode(category);
            
            std::cout << "URL with Query Parameters:" << std::endl;
            std::cout << "Original parameters:" << std::endl;
            std::cout << "  query: " << query << std::endl;
            std::cout << "  category: " << category << std::endl;
            std::cout << "Full URL: " << fullUrl << std::endl;
            std::cout << std::endl;
        }
        
        // Basic Auth Header
        {
            std::string username = "admin";
            std::string password = "secure&password!123";
            std::string credentials = username + ":" + password;
            std::string encodedAuth = Network::Base64Encode(credentials);
            
            std::cout << "Basic Auth Header:" << std::endl;
            std::cout << "Original credentials: " << credentials << std::endl;
            std::cout << "Auth header value: Basic " << encodedAuth << std::endl;
            std::cout << std::endl;
        }
    }

    Network::Cleanup();
    return 0;
}
