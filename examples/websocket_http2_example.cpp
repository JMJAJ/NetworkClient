#include "Network.hpp"
#include "WebSocket.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Initialize network
    if (!Network::Initialize()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return 1;
    }

    // Example 1: HTTP/2 Request
    {
        Network::RequestConfig config;
        config.use_http2 = true;  // Enable HTTP/2
        
        auto response = Network::Get("https://http2.github.io/", config);
        if (response.success) {
            std::cout << "HTTP/2 Request successful!" << std::endl;
            std::cout << "Status code: " << response.status_code << std::endl;
            std::cout << "Response size: " << response.body.size() << " bytes" << std::endl;
        }
    }

    // Example 2: Async Request
    {
        Network::RequestConfig config;
        config.use_http2 = true;
        
        std::cout << "\nStarting async request..." << std::endl;
        Network::GetAsync("https://api.github.com/users/octocat",
            [](const Network::NetworkResponse& response) {
                if (response.success) {
                    std::cout << "Async request successful!" << std::endl;
                    std::cout << "Status code: " << response.status_code << std::endl;
                    std::cout << "Response size: " << response.body.size() << " bytes" << std::endl;
                }
            },
            config
        );
        
        // Wait a bit to see the async response
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Example 3: WebSocket
    {
        WebSocket ws;
        WebSocket::Config wsConfig;
        wsConfig.auto_reconnect = true;
        wsConfig.ping_interval_ms = 30000;

        // Set up callbacks
        ws.SetMessageCallback([](const std::string& message) {
            std::cout << "Received message: " << message << std::endl;
        });

        ws.SetErrorCallback([](const std::string& error) {
            std::cerr << "WebSocket error: " << error << std::endl;
        });

        ws.SetStateCallback([](WebSocket::State state) {
            std::cout << "WebSocket state changed to: ";
            switch (state) {
                case WebSocket::State::CLOSED: std::cout << "CLOSED"; break;
                case WebSocket::State::CONNECTING: std::cout << "CONNECTING"; break;
                case WebSocket::State::CONNECTED: std::cout << "CONNECTED"; break;
                case WebSocket::State::CLOSING: std::cout << "CLOSING"; break;
            }
            std::cout << std::endl;
        });

        // Connect to a WebSocket echo server
        std::cout << "\nConnecting to WebSocket server..." << std::endl;
        if (ws.Connect("wss://ws.postman-echo.com/raw", wsConfig)) {
            // Send a test message
            std::cout << "Sending test message..." << std::endl;
            ws.Send("Hello, WebSocket!");
            
            // Wait for response
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // Disconnect
            std::cout << "Disconnecting..." << std::endl;
            ws.Disconnect();
        } else {
            std::cerr << "Failed to connect to WebSocket server" << std::endl;
        }
    }

    Network::Cleanup();
    return 0;
}
