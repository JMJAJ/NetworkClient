# NetworkClient

**NetworkClient** is a lightweight and portable C++ library for making HTTP requests, encoding data, and parsing URLs. It is implemented as header (`.hpp`) and source (`.cpp`) files, making it easy to integrate directly into your project without pre-compiling a library.

## Features
- Cross-platform compatibility.
- Supports multiple HTTP methods (GET, POST, PUT, DELETE, PATCH).
- URL parsing and validation.
- Base64 encoding and URL encoding utilities.
- No pre-built library needed—just include the `.hpp` and `.cpp` files.

## Getting Started

### Prerequisites
- C++17 or later.
- A C++ compiler compatible with your operating system.
- For Windows: Ensure `ws2_32.lib` is available.

### Installation
Clone the repository:
```bash
git clone https://github.com/yourusername/NetworkClient.git
cd NetworkClient
```

Copy the following files into your project:
- `Network.hpp`
- `Network.cpp`

### Usage
Include `Network.hpp` in your project and use the `Network` class to make HTTP requests:
```cpp
#include "Network.hpp"
#include <iostream>

int main() {
    // Initialize network (important for Windows)
    if (!Network::Initialize()) {
        std::cerr << "Network initialization failed!" << std::endl;
        return 1;
    }

    try {
        // 1. Simple GET request to a public API
        std::cout << "=== Testing GET Request ===" << std::endl;
        auto get_response = Network::Get("http://httpbin.org/get");
        if (get_response.success) {
            std::cout << "GET Request Successful" << std::endl;
            std::cout << "Status Code: " << get_response.status_code << std::endl;
            std::cout << "Response Body (first 200 chars): "
                << get_response.body.substr(0, 200) << std::endl;
        }
        else {
            std::cerr << "GET Request Failed: " << get_response.error_message << std::endl;
        }

        // 2. POST request with JSON payload
        std::cout << "\n=== Testing POST Request ===" << std::endl;
        Network::RequestConfig post_config;
        post_config.additional_headers["Content-Type"] = "application/json";
        post_config.timeout_seconds = 10;

        auto post_response = Network::Post(
            "http://httpbin.org/post",
            R"({"name":"John Doe","age":30})",
            "application/json",
            post_config
        );

        if (post_response.success) {
            std::cout << "POST Request Successful" << std::endl;
            std::cout << "Status Code: " << post_response.status_code << std::endl;
            std::cout << "Response Body (first 200 chars): "
                << post_response.body.substr(0, 200) << std::endl;
        }
        else {
            std::cerr << "POST Request Failed: " << post_response.error_message << std::endl;
        }

        // 3. PUT Request
        std::cout << "\n=== Testing PUT Request ===" << std::endl;
        auto put_response = Network::Request(
            Network::Method::HTTP_PUT,
            "http://httpbin.org/put",
            R"({"updated":"data"})"
        );

        if (put_response.success) {
            std::cout << "PUT Request Successful" << std::endl;
            std::cout << "Status Code: " << put_response.status_code << std::endl;
        }
        else {
            std::cerr << "PUT Request Failed: " << put_response.error_message << std::endl;
        }

        // 4. PATCH Request
        std::cout << "\n=== Testing PATCH Request ===" << std::endl;
        auto patch_response = Network::Request(
            Network::Method::HTTP_PATCH,
            "http://httpbin.org/patch",
            R"({"partial":"update"})"
        );

        if (patch_response.success) {
            std::cout << "PATCH Request Successful" << std::endl;
            std::cout << "Status Code: " << patch_response.status_code << std::endl;
        }
        else {
            std::cerr << "PATCH Request Failed: " << patch_response.error_message << std::endl;
        }

        // 5. DELETE Request
        std::cout << "\n=== Testing DELETE Request ===" << std::endl;
        auto delete_response = Network::Request(
            Network::Method::HTTP_DELETE,
            "http://httpbin.org/delete"
        );

        if (delete_response.success) {
            std::cout << "DELETE Request Successful" << std::endl;
            std::cout << "Status Code: " << delete_response.status_code << std::endl;
        }
        else {
            std::cerr << "DELETE Request Failed: " << delete_response.error_message << std::endl;
        }

        // 6. Failed Request Test (non-existent domain)
        std::cout << "\n=== Testing Failed Request ===" << std::endl;
        auto failed_response = Network::Get("http://nonexistentdomain123456.com");
        if (!failed_response.success) {
            std::cout << "Failed Request Test Passed" << std::endl;
            std::cout << "Error Message: " << failed_response.error_message << std::endl;
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
    }

    // Cleanup (important for Windows)
    Network::Cleanup();

    // Prevent console from closing immediately
    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}
```

### Build
Compile your project along with `Network.cpp`. For example, using `g++`:
```bash
g++ -o main main.cpp Network.cpp -std=c++17
```

On Windows, remember to link with the Winsock library:
```bash
g++ -o main main.cpp Network.cpp -std=c++17 -lws2_32
```

### API Overview
Here’s a quick overview of the provided methods:

#### HTTP Methods
- `Network::Get(url, config)`
- `Network::Post(url, payload, content_type, config)`
- `Network::Put(url, payload, config)`
- `Network::Delete(url, config)`

#### Utility Methods
- `Network::UrlEncode(const std::string&)`
- `Network::Base64Encode(const std::string&)`

#### Initialization and Cleanup
- `Network::Initialize()` - Must be called before using any network methods.
- `Network::Cleanup()` - Must be called to release network resources.

## Contribution
Contributions are welcome! Please fork the repository, create a new branch, and submit a pull request.

## License
This project is licensed under the MIT License.
