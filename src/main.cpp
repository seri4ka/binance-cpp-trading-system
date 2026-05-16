#include "market_data/MarketDataEvent.hpp"
#include "market_data/MarketDataParser.hpp"
#include "storage/JsonlWriter.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <nlohmann/json.hpp>
#include <openssl/err.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <variant>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;

using tcp = net::ip::tcp;
using json = nlohmann::json;

struct AppConfig {
    std::string host = "stream.binance.com";
    std::string port = "9443";
    std::string symbol = "btcusdt";
    std::vector<std::string> streams = { "bookTicker", "trade" };
    std::string cert_file = "cacert.pem";
    std::string raw_recording_path = "data/recordings/raw_events.jsonl";

    [[nodiscard]] std::string build_target() const {
        std::ostringstream oss;

        oss << "/stream?streams=";

        for (std::size_t i = 0; i < streams.size(); ++i) {
            if (i > 0) {
                oss << '/';
            }

            oss << symbol << '@' << streams[i];
        }

        return oss.str();
    }

    static AppConfig load_from_file(const std::string& path) {
        std::ifstream file(path);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + path);
        }

        json root;
        file >> root;

        AppConfig config;

        if (root.contains("host")) {
            config.host = root.at("host").get<std::string>();
        }

        if (root.contains("port")) {
            config.port = root.at("port").get<std::string>();
        }

        if (root.contains("symbol")) {
            config.symbol = root.at("symbol").get<std::string>();
        }

        if (root.contains("streams")) {
            config.streams = root.at("streams").get<std::vector<std::string>>();
        }

        if (root.contains("cert_file")) {
            config.cert_file = root.at("cert_file").get<std::string>();
        }

        if (root.contains("raw_recording_path")) {
            config.raw_recording_path = root.at("raw_recording_path").get<std::string>();
        }

        if (config.host.empty()) {
            throw std::runtime_error("Config error: host is empty");
        }

        if (config.port.empty()) {
            throw std::runtime_error("Config error: port is empty");
        }

        if (config.symbol.empty()) {
            throw std::runtime_error("Config error: symbol is empty");
        }

        if (config.streams.empty()) {
            throw std::runtime_error("Config error: streams list is empty");
        }

        if (config.cert_file.empty()) {
            throw std::runtime_error("Config error: cert_file is empty");
        }

        if (config.raw_recording_path.empty()) {
            throw std::runtime_error("Config error: raw_recording_path is empty");
        }

        return config;
    }
};

static long long now_ms() {
    const auto now = std::chrono::system_clock::now();

    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    );

    return ms.count();
}

static void print_book_ticker(const BestBidAsk& bba) {
    std::cout << std::fixed << std::setprecision(8)
        << "[BOOK] "
        << bba.symbol
        << " bid=" << bba.bid_price << " x " << bba.bid_qty
        << " ask=" << bba.ask_price << " x " << bba.ask_qty
        << " mid=" << bba.mid_price()
        << " spread_bps=" << std::setprecision(3) << bba.spread_bps()
        << '\n';
}

static void print_trade(const Trade& trade) {
    std::cout << std::fixed << std::setprecision(8)
        << "[TRADE] "
        << trade.symbol
        << " id=" << trade.trade_id
        << " price=" << trade.price
        << " qty=" << trade.qty
        << " buyer_is_maker=" << std::boolalpha << trade.buyer_is_maker
        << " trade_time_ms=" << trade.trade_time_ms
        << '\n';
}

static void handle_event(const MarketDataEvent& event) {
    std::visit(
        [](const auto& value) {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, BestBidAsk>) {
                print_book_ticker(value);
            }
            else if constexpr (std::is_same_v<T, Trade>) {
                print_trade(value);
            }
            else if constexpr (std::is_same_v<T, UnknownMarketDataEvent>) {
                std::cout << "[UNKNOWN] " << value.raw_message << '\n';
            }
        },
        event
    );
}

int main() {
    try {
        const AppConfig config = AppConfig::load_from_file("config/config.json");

        const std::string target = config.build_target();

        std::cout << "Loaded config:\n"
            << "  host:               " << config.host << '\n'
            << "  port:               " << config.port << '\n'
            << "  symbol:             " << config.symbol << '\n'
            << "  target:             " << target << '\n'
            << "  cert_file:          " << config.cert_file << '\n'
            << "  raw_recording_path: " << config.raw_recording_path << '\n';

        MarketDataParser parser;
        JsonlWriter raw_writer(config.raw_recording_path);

        net::io_context ioc;

        ssl::context ctx{ ssl::context::tlsv12_client };
        ctx.set_verify_mode(ssl::verify_peer);
        ctx.load_verify_file(config.cert_file);

        tcp::resolver resolver{ ioc };
        websocket::stream<beast::ssl_stream<tcp::socket>> ws{ ioc, ctx };

        if (!SSL_set_tlsext_host_name(
            ws.next_layer().native_handle(),
            config.host.c_str()
        )) {
            beast::error_code ec{
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category()
            };

            throw beast::system_error{ ec };
        }

        const auto results = resolver.resolve(config.host, config.port);

        net::connect(
            beast::get_lowest_layer(ws),
            results
        );

        ws.next_layer().handshake(ssl::stream_base::client);

        ws.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client
            )
        );

        ws.set_option(
            websocket::stream_base::decorator(
                [](websocket::request_type& req) {
                    req.set(
                        boost::beast::http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                        " binance-cpp-trading-system"
                    );
                }
            )
        );

        ws.handshake(config.host + ":" + config.port, target);

        std::cout << "Connected to wss://"
            << config.host
            << ":"
            << config.port
            << target
            << '\n';

        std::cout << "Recording raw events to: "
            << config.raw_recording_path
            << '\n';

        beast::flat_buffer buffer;

        while (true) {
            ws.read(buffer);

            const auto message = beast::buffers_to_string(buffer.data());
            buffer.consume(buffer.size());

            const long long local_recv_time_ms = now_ms();

            raw_writer.write_raw_message(local_recv_time_ms, message);

            const MarketDataEvent event = parser.parse(message);
            handle_event(event);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
