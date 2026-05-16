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

#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

        return config;
    }
};

struct BestBidAsk {
    std::string symbol;

    double bid_price = 0.0;
    double bid_qty = 0.0;

    double ask_price = 0.0;
    double ask_qty = 0.0;

    [[nodiscard]] double mid_price() const {
        return (bid_price + ask_price) / 2.0;
    }

    [[nodiscard]] double spread_bps() const {
        const double mid = mid_price();

        if (mid <= 0.0) {
            return 0.0;
        }

        return (ask_price - bid_price) / mid * 10000.0;
    }
};

static double to_double(const json& value) {
    if (value.is_string()) {
        return std::stod(value.get<std::string>());
    }

    return value.get<double>();
}

static void handle_book_ticker(const json& data) {
    BestBidAsk bba;

    bba.symbol = data.at("s").get<std::string>();
    bba.bid_price = to_double(data.at("b"));
    bba.bid_qty = to_double(data.at("B"));
    bba.ask_price = to_double(data.at("a"));
    bba.ask_qty = to_double(data.at("A"));

    std::cout << std::fixed << std::setprecision(8)
        << "[BOOK] "
        << bba.symbol
        << " bid=" << bba.bid_price << " x " << bba.bid_qty
        << " ask=" << bba.ask_price << " x " << bba.ask_qty
        << " mid=" << bba.mid_price()
        << " spread_bps=" << std::setprecision(3) << bba.spread_bps()
        << '\n';
}

static void handle_trade(const json& data) {
    const auto symbol = data.at("s").get<std::string>();
    const auto trade_id = data.at("t").get<long long>();
    const auto price = to_double(data.at("p"));
    const auto qty = to_double(data.at("q"));
    const auto trade_time = data.at("T").get<long long>();
    const auto buyer_is_maker = data.at("m").get<bool>();

    std::cout << std::fixed << std::setprecision(8)
        << "[TRADE] "
        << symbol
        << " id=" << trade_id
        << " price=" << price
        << " qty=" << qty
        << " buyer_is_maker=" << std::boolalpha << buyer_is_maker
        << " trade_time_ms=" << trade_time
        << '\n';
}

static void handle_message(const std::string& message) {
    try {
        const auto root = json::parse(message);

        const json& data = root.contains("data") ? root.at("data") : root;

        if (!data.is_object()) {
            return;
        }

        if (data.contains("e") && data.at("e").get<std::string>() == "trade") {
            handle_trade(data);
            return;
        }

        if (
            data.contains("b") &&
            data.contains("B") &&
            data.contains("a") &&
            data.contains("A")
            ) {
            handle_book_ticker(data);
            return;
        }

        std::cout << "[OTHER] " << message << '\n';
    }
    catch (const std::exception& ex) {
        std::cerr << "[PARSE_ERROR] " << ex.what()
            << " raw=" << message
            << '\n';
    }
}

int main() {
    try {
        const AppConfig config = AppConfig::load_from_file("config/config.json");

        const std::string target = config.build_target();

        std::cout << "Loaded config:\n"
            << "  host:      " << config.host << '\n'
            << "  port:      " << config.port << '\n'
            << "  symbol:    " << config.symbol << '\n'
            << "  target:    " << target << '\n'
            << "  cert_file: " << config.cert_file << '\n';

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

        beast::flat_buffer buffer;

        while (true) {
            ws.read(buffer);

            const auto message = beast::buffers_to_string(buffer.data());
            buffer.consume(buffer.size());

            handle_message(message);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
