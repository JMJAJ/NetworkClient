# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.1.x   | :white_check_mark: |

## Security Features

### SSL/TLS Configuration
- The library enforces TLS 1.2 or higher by default
- Certificate validation is enabled by default
- Custom certificate validation can be configured via RequestConfig

### Authentication
- Supports API Key and OAuth token authentication
- Keys should be stored securely (not hardcoded)
- Use environment variables or secure configuration management

### Best Practices
1. Always use HTTPS for production environments
2. Keep the library updated to the latest version
3. Enable SSL certificate validation
4. Set appropriate timeouts
5. Use rate limiting to prevent abuse
6. Implement proper error handling
7. Validate all input URLs and data

### Example Secure Configuration
```cpp
Network::RequestConfig config;
config.verify_ssl = true;
config.use_tls12_or_higher = true;
config.timeout_seconds = 30;
config.api_key = getSecureApiKey(); // Get from secure storage
```

## Reporting a Vulnerability

If you discover a security vulnerability, please follow these steps:

1. **Open** a public issue
2. Include:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)
