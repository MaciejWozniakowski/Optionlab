#pragma once

#include <functional>
#include <memory>
#include <string>
#include <map>

namespace optionlab {

// A single trade print as delivered by the market-data feed.
struct Trade {
    std::string symbol;
    double      price     = 0.0;
    double      volume    = 0.0;
    long long   timestamp = 0;  // epoch milliseconds (Finnhub "t")
};

// Streams live trades from a Finnhub-style WebSocket feed and hands each
// trade to a user-supplied callback. The heavy Boost.Beast / Asio / OpenSSL
// machinery is hidden behind a pimpl so consumers (e.g. the pricing engine)
// only need this lightweight header.
class DataGatherer {
public:
    using TickCallback = std::function<void(const Trade&)>;

    explicit DataGatherer(std::string token,
                          std::string host = "ws.finnhub.io",
                          std::string port = "443");
    ~DataGatherer();

    // Non-copyable, non-movable (owns a live connection and thread).
    DataGatherer(const DataGatherer&)            = delete;
    DataGatherer& operator=(const DataGatherer&) = delete;

    // Queue a symbol to subscribe to. Call before start()/run().
    void subscribe(const std::string& symbol);

    // Set the handler invoked for every incoming trade. Called on the I/O
    // thread, so keep it short (hand off to a queue / update a snapshot).
    // If none is set, trades are printed to stdout.
    void on_tick(TickCallback cb);

    // Connect, subscribe, and read until stop() or a connection error.
    // Blocks the calling thread.
    void run();

    // Run run() on a dedicated background I/O thread.
    void start();

    // Signal the read loop to stop and join the background thread.
    void stop();

    // True while the read loop is active.
    bool running() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class DataParser{
private:
    struct Impl;
    std::map <long long, double> graph;
public:
    void graph_data();

};
}  // namespace optionlab
