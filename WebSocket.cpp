#include "WebSocket.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

class WebSocket::Impl {
public:
    MessageCallback messageCallback;
    ErrorCallback errorCallback;
    StateCallback stateCallback;
    State state;
    Config config;
    Impl() : state(State::CLOSED), hSession(nullptr), hConnection(nullptr), hWebSocket(nullptr) {}
    
    ~Impl() {
        Cleanup();
    }

    bool Initialize(const std::string& url, const Config& config) {
        this->config = config;
        state = State::CONNECTING;
        if (stateCallback) stateCallback(state);
        
        // Parse URL
        URL_COMPONENTS urlComp = { 0 };
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.dwSchemeLength = -1;
        urlComp.dwHostNameLength = -1;
        urlComp.dwUrlPathLength = -1;
        urlComp.dwExtraInfoLength = -1;
        
        std::wstring wideUrl(url.begin(), url.end());
        if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.length()), 0, &urlComp)) {
            if (errorCallback) errorCallback("Failed to parse URL: " + std::to_string(GetLastError()));
            return false;
        }

        // Create session
        hSession = WinHttpOpen(
            L"WebSocket Client",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0  // Synchronous for now to simplify error handling
        );

        if (!hSession) {
            if (errorCallback) errorCallback("Failed to create session: " + std::to_string(GetLastError()));
            return false;
        }

        // Set timeouts
        if (!WinHttpSetTimeouts(hSession, 0, 60000, 30000, 30000)) {
            if (errorCallback) errorCallback("Failed to set timeouts: " + std::to_string(GetLastError()));
            Cleanup();
            return false;
        }

        // Create connection
        std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);
        hConnection = WinHttpConnect(
            hSession,
            host.c_str(),
            urlComp.nPort,
            0
        );

        if (!hConnection) {
            if (errorCallback) errorCallback("Failed to create connection: " + std::to_string(GetLastError()));
            Cleanup();
            return false;
        }

        // Create WebSocket handle
        std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
        if (!path.empty() && path[0] != L'/') {
            path = L"/" + path;
        }
        
        DWORD flags = WINHTTP_FLAG_SECURE;
        if (urlComp.nScheme == INTERNET_SCHEME_HTTP) {
            flags = 0;
        }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnection,
            L"GET",
            path.c_str(),
            L"HTTP/1.1",
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            flags
        );

        if (!hRequest) {
            if (errorCallback) errorCallback("Failed to create request: " + std::to_string(GetLastError()));
            Cleanup();
            return false;
        }

        // Add WebSocket protocol header if specified
        if (!config.subprotocol.empty()) {
            std::wstring protocol(config.subprotocol.begin(), config.subprotocol.end());
            if (!WinHttpAddRequestHeaders(hRequest,
                (L"Sec-WebSocket-Protocol: " + protocol).c_str(),
                -1L,
                WINHTTP_ADDREQ_FLAG_ADD)) {
                if (errorCallback) errorCallback("Failed to add protocol header: " + std::to_string(GetLastError()));
                WinHttpCloseHandle(hRequest);
                Cleanup();
                return false;
            }
        }

        // Send request
        if (!WinHttpSendRequest(hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0)) {
            if (errorCallback) errorCallback("Failed to send request: " + std::to_string(GetLastError()));
            WinHttpCloseHandle(hRequest);
            Cleanup();
            return false;
        }

        // Receive response
        if (!WinHttpReceiveResponse(hRequest, nullptr)) {
            if (errorCallback) errorCallback("Failed to receive response: " + std::to_string(GetLastError()));
            WinHttpCloseHandle(hRequest);
            Cleanup();
            return false;
        }

        // Complete WebSocket handshake
        hWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest, 0);
        WinHttpCloseHandle(hRequest);

        if (!hWebSocket) {
            if (errorCallback) errorCallback("Failed to complete WebSocket upgrade: " + std::to_string(GetLastError()));
            Cleanup();
            return false;
        }

        state = State::CONNECTED;
        if (stateCallback) stateCallback(state);

        // Start receive thread
        receiveThread = std::thread(&Impl::ReceiveLoop, this);
        
        // Start ping thread if enabled
        if (config.ping_interval_ms > 0) {
            pingThread = std::thread(&Impl::PingLoop, this);
        }

        return true;
    }

    void Cleanup() {
        state = State::CLOSING;
        
        if (receiveThread.joinable()) receiveThread.join();
        if (pingThread.joinable()) pingThread.join();
        
        if (hWebSocket) {
            WinHttpWebSocketClose(hWebSocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
            WinHttpCloseHandle(hWebSocket);
            hWebSocket = nullptr;
        }
        
        if (hConnection) {
            WinHttpCloseHandle(hConnection);
            hConnection = nullptr;
        }
        
        if (hSession) {
            WinHttpCloseHandle(hSession);
            hSession = nullptr;
        }

        state = State::CLOSED;
        if (stateCallback) stateCallback(state);
    }

    bool Send(const std::string& message) {
        if (!hWebSocket) return false;

        DWORD result = WinHttpWebSocketSend(
            hWebSocket,
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            (PVOID)message.c_str(),
            static_cast<DWORD>(message.length())
        );

        if (result != NO_ERROR) {
            if (errorCallback) errorCallback("Failed to send message");
            return false;
        }
        return true;
    }

    bool SendBinary(const std::vector<uint8_t>& data) {
        if (!hWebSocket) return false;

        DWORD result = WinHttpWebSocketSend(
            hWebSocket,
            WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
            (PVOID)data.data(),
            static_cast<DWORD>(data.size())
        );

        if (result != NO_ERROR) {
            if (errorCallback) errorCallback("Failed to send binary data");
            return false;
        }
        return true;
    }

private:
    void ReceiveLoop() {
        std::vector<uint8_t> buffer(4096);
        while (state == State::CONNECTED) {
            DWORD bytesRead = 0;
            WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;
            
            DWORD result = WinHttpWebSocketReceive(
                hWebSocket,
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                &bytesRead,
                &bufferType
            );

            if (result != NO_ERROR) {
                if (errorCallback) errorCallback("Failed to receive data");
                break;
            }

            if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
                if (messageCallback) {
                    std::string message(reinterpret_cast<char*>(buffer.data()), bytesRead);
                    messageCallback(message);
                }
            }
        }
    }

    void PingLoop() {
        while (state == State::CONNECTED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config.ping_interval_ms));

            if (state != State::CONNECTED) break;

            // Send ping frame using binary message type
            DWORD result = WinHttpWebSocketSend(
                hWebSocket,
                WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
                nullptr,
                0
            );

            if (result != NO_ERROR) {
                if (errorCallback) errorCallback("Ping failed");
                break;
            }
        }
    }

    HINTERNET hSession;
    HINTERNET hConnection;
    HINTERNET hWebSocket;
    
    std::thread receiveThread;
    std::thread pingThread;
};

// WebSocket implementation
WebSocket::WebSocket() : impl(std::make_unique<Impl>()) {}
WebSocket::~WebSocket() = default;

bool WebSocket::Connect(const std::string& url, const Config& config) {
    return impl->Initialize(url, config);
}

void WebSocket::Disconnect() {
    impl->Cleanup();
}

bool WebSocket::Send(const std::string& message) {
    return impl->Send(message);
}

bool WebSocket::SendBinary(const std::vector<uint8_t>& data) {
    return impl->SendBinary(data);
}

void WebSocket::SetMessageCallback(MessageCallback callback) {
    impl->messageCallback = std::move(callback);
}

void WebSocket::SetErrorCallback(ErrorCallback callback) {
    impl->errorCallback = std::move(callback);
}

void WebSocket::SetStateCallback(StateCallback callback) {
    impl->stateCallback = std::move(callback);
}

WebSocket::State WebSocket::GetState() const {
    return impl->state;
}
