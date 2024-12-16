# NetworkClient

**NetworkClient** is a lightweight and portable C++ library for making HTTP requests, encoding data, and parsing URLs. It is implemented as header (`.hpp`) and source (`.cpp`) files, making it easy to integrate directly into your project without pre-compiling a library.

## Features
- Cross-platform compatibility
- Supports multiple HTTP methods (GET, POST, PUT, DELETE, PATCH)
- HTTPS support with secure connections
- URL parsing and validation
- Base64 encoding and URL encoding utilities
- Configurable request options (timeouts, redirects, custom headers)
- Comprehensive test suite for HTTP and HTTPS functionality
- No pre-built library needed—just include the `.hpp` and `.cpp` files

## Getting Started

### Prerequisites
- C++17 or later
- A C++ compiler compatible with your operating system
- For Windows: WinINet library (wininet.lib)

### Installation
Clone the repository:
```bash
git clone https://github.com/JMJAJ/NetworkClient.git
cd NetworkClient
```

Copy the following files into your project:
- `net/Network.hpp`
- `net/Network.cpp`
- `net/SimpleNetwork.hpp`
- `net/SimpleNetwork.cpp`

### Testing
The project includes a comprehensive test suite in `main.cpp` that verifies:
- HTTP GET/POST requests
- HTTPS GET/POST requests
- Custom header handling
- Redirect following
- Timeout behavior

To run the tests:
```cpp
// Compile and run main.cpp
```

### Usage
Include `Network.hpp` in your project and use the `Network` class to make HTTP requests:
```cpp
#include "net/Network.hpp"
#include <iostream>

int main() {
    // Initialize network (important for Windows)
    if (!Network::Initialize()) {
        std::cerr << "Network initialization failed!" << std::endl;
        return 1;
    }

    try {
        // Configure request options
        Network::RequestConfig config;
        config.timeout_seconds = 30;
        config.follow_redirects = true;
        config.additional_headers["User-Agent"] = "NetworkClient/1.0";

        // Make HTTPS GET request
        auto response = Network::Get("https://api.example.com/data", config);
        if (response.success) {
            std::cout << "Status Code: " << response.status_code << std::endl;
            std::cout << "Response: " << response.body << std::endl;
        }

        // Make POST request with JSON data
        std::string json_payload = "{\"key\":\"value\"}";
        response = Network::Post(
            "https://api.example.com/update",
            json_payload,
            "application/json",
            config
        );
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // Always cleanup
    Network::Cleanup();
    return 0;
}
```

Check more in `example.cpp`

## API Reference

### Core Methods
- `Network::Initialize()` - Must be called before using any network methods
- `Network::Cleanup()` - Must be called to release network resources
- `Network::Get(url, config)` - Perform HTTP/HTTPS GET request
- `Network::Post(url, payload, content_type, config)` - Perform HTTP/HTTPS POST request
- `Network::Put(url, payload, content_type, config)` - Perform HTTP/HTTPS PUT request
- `Network::Delete(url, config)` - Perform HTTP/HTTPS DELETE request

### Configuration Options
- `timeout_seconds` - Request timeout in seconds
- `follow_redirects` - Whether to follow HTTP redirects
- `max_redirects` - Maximum number of redirects to follow
- `additional_headers` - Custom HTTP headers

## Contribution
Contributions are welcome! Please fork the repository, create a new branch, and submit a pull request.

## License
This project is licensed under the MIT License - see the LICENSE file for details.
