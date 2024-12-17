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
// Initialize the library
if (!Network::Initialize()) {
    std::cerr << "Failed to initialize network\n";
    return 1;
}

// Make a GET request
auto response = Network::Get("https://httpbin.org/get");
if (response.success) {
    std::cout << "Response: " << response.body << "\n";
}

// Make a POST request with custom headers
Network::RequestConfig config;
config.additional_headers["Content-Type"] = "application/json";
config.timeout_seconds = 30;

auto response = Network::Post(
    "https://postman-echo.com/post",
    "{\"key\": \"value\"}",
    "application/json",
    config
);

// Cleanup
Network::Cleanup();
```

## Advanced Usage

### Custom Configuration

```cpp
Network::RequestConfig config;

// Security settings
config.verify_ssl = true;
config.use_tls12_or_higher = true;

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

The library comes with a comprehensive test suite that verifies:
- Basic HTTP methods functionality
- Security features (SSL/TLS)
- Error handling
- Performance characteristics
- Retry mechanism
- Rate limiting

Run the tests:
```bash
./NetworkClient.exe
```

## Known Issues

1. SSL Certificate Validation
   - Some self-signed certificates may not be recognized
   - Working on adding custom certificate support

2. Domain Validation
   - Certain special domain formats may not be properly validated
   - Improvements planned for future releases

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Credits

- Author: Jxint
- Version: 1.1.0
- Last Updated: December 2024

## Support

For support, please:
1. Check the [Issues](https://github.com/JMJAJ/NetworkClient/issues) page
2. Create a new issue if your problem isn't already listed
3. Provide as much detail as possible about your environment and the issue

## Roadmap

Future improvements planned:
- [ ] WebSocket support
- [ ] HTTP/2 support
- [ ] Async request handling
- [ ] Custom certificate handling
- [ ] Proxy support
- [ ] Cookie management
- [ ] Request/Response compression
- [ ] Better timeout granularity
