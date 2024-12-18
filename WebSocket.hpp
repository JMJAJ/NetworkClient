#ifndef WEBSOCKET_HPP
#define WEBSOCKET_HPP

#include <string>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

class WebSocket {
public:
    enum class State {
        CLOSED,
        CONNECTING,
        CONNECTED,
        CLOSING
    };

    struct Config {
        bool auto_reconnect = true;
        int reconnect_delay_ms = 5000;
        int ping_interval_ms = 30000;
        bool use_compression = true;
        std::string subprotocol;
    };

    using MessageCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using StateCallback = std::function<void(State)>;

    WebSocket();
    ~WebSocket();

    bool Connect(const std::string& url, const Config& config = Config());
    void Disconnect();
    bool Send(const std::string& message);
    bool SendBinary(const std::vector<uint8_t>& data);
    
    void SetMessageCallback(MessageCallback callback);
    void SetErrorCallback(ErrorCallback callback);
    void SetStateCallback(StateCallback callback);
    
    State GetState() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

#endif // WEBSOCKET_HPP
