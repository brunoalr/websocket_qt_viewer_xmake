#include <QApplication>
#include <QTextEdit>
#include <QTimer>
#include <QString>
#include <thread>
#include <atomic>
#include <sstream>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

// Thread-safe queue for passing messages from Boost to Qt
#include <queue>
#include <mutex>
#include <condition_variable>

class MessageQueue {
public:
    void push(const std::string& msg) {
        std::lock_guard<std::mutex> lock(m_);
        q_.push(msg);
    }
    std::queue<std::string> pop_all() {
        std::lock_guard<std::mutex> lock(m_);
        auto ret = q_;
        while(!q_.empty()) q_.pop();
        return ret;
    }
private:
    std::mutex m_;
    std::queue<std::string> q_;
};

std::atomic<bool> running{true};

void websocket_thread(MessageQueue* queue) {
    try {
        boost::asio::io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        // Public echo websocket server
        auto const host = "echo.websocket.events";
        auto const port = "80";
        auto const path = "/";

        auto const results = resolver.resolve(host, port);
        boost::asio::connect(ws.next_layer(), results.begin(), results.end());

        // Perform websocket handshake
        ws.handshake(host, path);

        // Send a message
        ws.write(boost::asio::buffer(std::string("Hello, world!")));

        while (running) {
            boost::beast::multi_buffer buffer;
            ws.read(buffer);
            std::ostringstream oss;
            oss << boost::beast::make_printable(buffer.data());
            queue->push(oss.str());
        }
    } catch (const std::exception& ex) {
        queue->push(std::string("Exception: ") + ex.what());
    }
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QTextEdit text;
    text.setReadOnly(true);
    text.setWindowTitle("WebSocket Payloads (Boost.Beast + Qt)");
    text.show();

    MessageQueue queue;
    std::thread ws_thread(websocket_thread, &queue);

    // Poll the queue and display messages in the QTextEdit
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        auto msgs = queue.pop_all();
        while (!msgs.empty()) {
            text.append(QString::fromStdString(msgs.front()));
            msgs.pop();
        }
    });
    timer.start(100);

    int ret = app.exec();
    running = false;
    ws_thread.join();
    return ret;
}
