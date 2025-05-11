# Network Class Library

A modern C++ HTTP networking library built on Windows Internet (WinINet) API, providing a simple yet powerful interface for making HTTP/HTTPS requests.

## Features

- **HTTP Methods Support**
  - GET, POST, PUT, PATCH, DELETE
  - Configurable request headers
  - Form data and JSON payload support
  - Response headers and body handling

- **Security**
  - SSL/TLS support with certificate validation
  - TLS 1.2+ enforcement option
  - API key and OAuth token support
  - Custom security flags configuration

- **Advanced Features**
  - Automatic retry mechanism with exponential backoff
  - Rate limiting with precise timing
  - Timeout handling at multiple levels
  - Redirect following with max limit
  - URL encoding and Base64 encoding utilities
  - Asynchronous request support
  - HTTP/2 support

- **Error Handling**
  - Detailed error messages
  - Status code validation
  - Connection error recovery
  - Invalid URL/protocol detection

## Requirements

- Windows OS
- C++17 or later
- Visual Studio 2019 or later
- WinINet library (included in Windows SDK)

## Installation

1. Clone the repository:
```bash
git clone https://github.com/JMJAJ/NetworkClient.git
```

2. Include the library in your project:
```cpp
#include "Network.hpp"
```

3. Link against WinINet:
```cpp
#pragma comment(lib, "wininet.lib")
```

## Quick Start

```cpp
#include "Network.hpp"
#include <iostream>

int main() {

    // Initialize the library
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network\n";
        return 1;
    }

    // Make a GET request
    auto response_get = Network::Get("https://httpbin.org/get");
    if (response_get.success) {
        std::cout << "Response: " << response_get.body << "\n";
    }

    // Make a POST request with custom headers
    Network::RequestConfig config;
    config.additional_headers["Content-Type"] = "application/json";
    config.timeout_seconds = 30;

    auto response_post = Network::Post(
        "https://postman-echo.com/post",
        "{\"key\": \"value\"}",
        "application/json",
        config
    );

    if (response_post.success) {
        std::cout << "Response: " << response_post.body << "\n";
    }

    // Cleanup
    Network::Cleanup();

    return 0;
}
```

## Advanced Usage

### Custom Configuration

```cpp
Network::RequestConfig config;

// Security settings
config.verify_ssl = true;
config.use_tls12_or_higher = true;
config.use_http2 = true;  // Enable HTTP/2 support

// Retry settings
config.max_retries = 3;
config.retry_delay_ms = 1000;

// Rate limiting
config.rate_limit_per_minute = 60;

// Authentication
config.api_key = "your-api-key";
config.oauth_token = "your-oauth-token";

// Custom headers
config.additional_headers["User-Agent"] = "MyApp/1.0";
config.additional_headers["Accept"] = "application/json";
```

### Asynchronous Requests

```cpp
// Async GET request
std::atomic<bool> get_completed{false};
Network::GetAsync("https://httpbin.org/get",
    [&get_completed](const Network::NetworkResponse& response) {
        if (response.success) {
            std::cout << "Async GET completed: " << response.body << "\n";
        }
        get_completed = true;
    }
);

// Async POST request
Network::RequestConfig config;
config.timeout_seconds = 30;

std::atomic<bool> post_completed{false};
Network::PostAsync(
    "https://httpbin.org/post",
    "{\"key\": \"value\"}",
    "application/json",
    [&post_completed](const Network::NetworkResponse& response) {
        if (response.success) {
            std::cout << "Async POST completed: " << response.body << "\n";
        }
        post_completed = true;
    },
    config
);

// Continue execution while requests are processing
std::cout << "Requests are processing asynchronously...\n";

// Wait for both requests to complete (with timeout)
const int MAX_WAIT_SECONDS = 10;
int waited_seconds = 0;
while ((!get_completed || !post_completed) && waited_seconds < MAX_WAIT_SECONDS) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    waited_seconds++;
    std::cout << "Waiting for requests to complete... " << waited_seconds << "s\n";
}

if (!get_completed || !post_completed) {
    std::cout << "Some requests did not complete within timeout period.\n";
}
```

### HTTP/2 Example

```cpp
Network::RequestConfig config;
config.use_http2 = true;  // Enable HTTP/2
config.verify_ssl = true; // Required for HTTP/2

// Make HTTP/2 request
auto response = Network::Get("https://http2.github.io/", config);
if (response.success) {
    std::cout << "HTTP/2 Request successful!\n";
    std::cout << "Protocol: " << response.headers["protocol"] << "\n";
    std::cout << "Response size: " << response.body.size() << " bytes\n";
}
```

### Error Handling

```cpp
auto response = Network::Request(
    Network::Method::HTTP_GET,
    "https://httpbin.org/status/404"  // Will return 404 Not Found
);

if (!response.success) {
    std::cerr << "Error: " << response.error_message << "\n";
    std::cerr << "Status Code: " << response.status_code << "\n";
}
```

### Rate Limiting Example

```cpp
Network::RequestConfig config;
config.rate_limit_per_minute = 30;  // 30 requests per minute

// These requests will be automatically rate-limited
for (int i = 0; i < 5; ++i) {
    auto response = Network::Get("https://postman-echo.com/get", config);
    std::cout << "Request " << i + 1 << " status: " << response.status_code << "\n";
}
```

### Different HTTP Methods Example

```cpp
// GET request
auto get_response = Network::Get("https://httpbin.org/get");

// POST request with JSON
auto post_response = Network::Post(
    "https://httpbin.org/post",
    "{\"name\": \"test\", \"value\": 123}",
    "application/json"
);

// PUT request
auto put_response = Network::Put(
    "https://httpbin.org/put",
    "updated_data=hello",
    "application/x-www-form-urlencoded"
);

// DELETE request
auto delete_response = Network::Delete("https://httpbin.org/delete");

// PATCH request
Network::RequestConfig config;
config.additional_headers["Content-Type"] = "application/json";
auto patch_response = Network::Request(
    Network::Method::HTTP_PATCH,
    "https://httpbin.org/patch",
    "{\"updated\": true}"
);
```

### Retry Mechanism Example

```cpp
Network::RequestConfig config;
config.max_retries = 3;
config.retry_delay_ms = 1000;

// This will retry up to 3 times on 500 error
auto response = Network::Get(
    "https://httpbin.org/status/500",
    config
);

std::cout << "Final status after retries: " << response.status_code << "\n";
```

### Large Payload Example

```cpp
// Get a large response (5000 bytes of random data)
auto response = Network::Get("https://httpbin.org/bytes/5000");
std::cout << "Received " << response.body.length() << " bytes\n";
```

## Testing

The library includes a comprehensive test suite (`example.cpp`) that thoroughly validates all aspects of the library:

### Test Categories
1. **Basic HTTP Methods**
   - GET, POST, PUT, PATCH, DELETE operations
   - Request/response validation
   - Header handling

2. **Security Features**
   - SSL/TLS configuration
   - Certificate validation
   - API key authentication
   - OAuth token handling

3. **Performance**
   - Rate limiting behavior
   - Timeout handling
   - Connection pooling
   - Load distribution

4. **Error Handling**
   - Invalid URL scenarios
   - Network issues
   - Timeout conditions
   - Status code validation

5. **Advanced Features**
   - Asynchronous operations
   - WebSocket functionality (beta)
   - Compression handling
   - Content type validation

6. **Edge Cases**
   - Large payloads
   - Special characters
   - Unicode support
   - Concurrent requests

### Running Tests
# Build in Release mode
Build it in VS in Release mode

```bash
# Run the test suite
./NetworkTest
```

The test suite provides detailed output including:
- Success/failure status for each test
- Performance metrics
- Latency measurements
- Server distribution statistics

## Documentation

### Available Documentation
1. **README.md** (this file)
   - Feature overview
   - Installation instructions
   - Quick start guide
   - Advanced usage examples

2. **CHANGELOG.md**
   - Version history
   - Feature additions
   - Bug fixes
   - Breaking changes

3. **SECURITY.md**
   - Security policies
   - SSL/TLS configuration
   - Best practices for API keys
   - Known security limitations

4. **CONTRIBUTING.md**
   - Development guidelines
   - Code style requirements
   - Pull request process
   - Testing requirements

5. **Example Code**
   - `examples/` demonstrates:
     - Basic request operations
     - Security configurations
     - Error handling patterns
     - Rate limiting usage
     - Advanced features

### API Documentation
The codebase includes comprehensive documentation with:
- Detailed function descriptions
- Parameter explanations
- Return value specifications
- Usage examples
- Error handling guidance

## Known Issues

1. SSL Certificate Validation
   - Some self-signed certificates may not be recognized
   - Working on adding custom certificate support

2. Domain Validation
   - Certain special domain formats may not be properly validated
   - Improvements planned for future releases

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Credits

- Author: Jxint
- Version: 1.1.0
- Last Updated: December 2024

## Roadmap

Future improvements planned:
- [ ] WebSocket support (beta)
- [x] HTTP/2 support
- [x] Async request handling
- [ ] Custom certificate handling
- [ ] Proxy support
- [ ] Cookie management
- [ ] Request/Response compression
- [ ] Better timeout granularity
