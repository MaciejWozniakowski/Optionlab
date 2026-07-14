#include "optionlab/data_parser.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/json.hpp>

#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace beast     = boost::beast;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
namespace ssl       = boost::asio::ssl;
namespace json      = boost::json;
using tcp           = boost::asio::ip::tcp;

namespace optionlab {

// ---- Private implementation ------------------------------------------------
struct DataGatherer::Impl {
    Impl(std::string token, std::string host, std::string port)
        : token(std::move(token)),
          host(std::move(host)),
          port(std::move(port)),
          ctx(ssl::context::tls_client),
          ws(ioc, ctx) {
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);
    }

    void connect() {
        tcp::resolver resolver{ioc};
        auto const results = resolver.resolve(host, port);
        auto ep = net::connect(beast::get_lowest_layer(ws), results);

        // SNI — the TLS handshake fails without it.
        if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str())) {
            throw beast::system_error{beast::error_code{
                static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()}};
        }

        ws.next_layer().handshake(ssl::stream_base::client);            // TLS
        ws.handshake(host + ':' + std::to_string(ep.port()),
                     "/?token=" + token);                               // WebSocket

        for (auto const& sym : symbols) {
            json::object sub{{"type", "subscribe"}, {"symbol", sym}};
            ws.write(net::buffer(json::serialize(sub)));
        }
    }

    void read_loop() {
        beast::flat_buffer buffer;
        while (running.load(std::memory_order_acquire)) {
            beast::error_code ec;
            ws.read(buffer, ec);
            if (ec) {
                if (running.load(std::memory_order_acquire))
                    std::cerr << "DataGatherer read error: " << ec.message() << '\n';
                break;
            }

            auto msg = beast::buffers_to_string(buffer.data());
            buffer.consume(buffer.size());
            dispatch(msg);
        }
    }

    void dispatch(const std::string& msg) {
        boost::system::error_code jec;
        auto v = json::parse(msg, jec);
        if (jec) return;

        auto* obj = v.if_object();
        if (!obj) return;

        auto it = obj->find("type");
        if (it == obj->end() || it->value() != "trade") return;

        auto* data = obj->at("data").if_array();
        if (!data) return;

        for (auto& elem : *data) {
            auto* to = elem.if_object();
            if (!to) continue;

            Trade trade;
            trade.symbol    = std::string(to->at("s").as_string());
            trade.price     = to->at("p").to_number<double>();
            trade.volume    = to->at("v").to_number<double>();
            trade.timestamp = to->at("t").to_number<long long>();

            if (callback)
                callback(trade);
            else
                std::cout << trade.symbol << "  " << trade.price
                          << "  vol " << trade.volume << '\n';
        }
    }

    void shutdown() {
        // Break a blocking read from another thread by closing the socket.
        beast::error_code ec;
        beast::get_lowest_layer(ws).close(ec);
    }

    std::string token, host, port;
    std::vector<std::string>       symbols;
    DataGatherer::TickCallback     callback;

    net::io_context                ioc;
    ssl::context                   ctx;
    websocket::stream<beast::ssl_stream<tcp::socket>> ws;

    std::thread       worker;
    std::atomic<bool> running{false};
};

// ---- Public interface ------------------------------------------------------
DataGatherer::DataGatherer(std::string token, std::string host, std::string port)
    : impl_(std::make_unique<Impl>(std::move(token), std::move(host), std::move(port))) {}

DataGatherer::~DataGatherer() {
    stop();
}

void DataGatherer::subscribe(const std::string& symbol) {
    impl_->symbols.push_back(symbol);
}

void DataGatherer::on_tick(TickCallback cb) {
    impl_->callback = std::move(cb);
}

void DataGatherer::run() {
    impl_->running.store(true, std::memory_order_release);
    try {
        impl_->connect();
        impl_->read_loop();
    } catch (const std::exception& e) {
        std::cerr << "DataGatherer error: " << e.what() << '\n';
    }
    impl_->running.store(false, std::memory_order_release);
}

void DataGatherer::start() {
    if (impl_->running.load(std::memory_order_acquire)) return;
    impl_->worker = std::thread([this] { run(); });
}

void DataGatherer::stop() {
    if (impl_->running.exchange(false, std::memory_order_acq_rel))
        impl_->shutdown();
    if (impl_->worker.joinable())
        impl_->worker.join();
}

bool DataGatherer::running() const noexcept {
    return impl_->running.load(std::memory_order_acquire);
}

}  // namespace optionlab
