#include "MarketDataParser.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <string>

using json = nlohmann::json;

namespace {

    double to_double(const json& value) {
        if (value.is_string()) {
            return std::stod(value.get<std::string>());
        }

        return value.get<double>();
    }

    BestBidAsk parse_book_ticker(const json& data) {
        BestBidAsk bba;

        bba.symbol = data.at("s").get<std::string>();
        bba.bid_price = to_double(data.at("b"));
        bba.bid_qty = to_double(data.at("B"));
        bba.ask_price = to_double(data.at("a"));
        bba.ask_qty = to_double(data.at("A"));

        return bba;
    }

    Trade parse_trade(const json& data) {
        Trade trade;

        trade.symbol = data.at("s").get<std::string>();
        trade.trade_id = data.at("t").get<long long>();
        trade.price = to_double(data.at("p"));
        trade.qty = to_double(data.at("q"));
        trade.trade_time_ms = data.at("T").get<long long>();
        trade.buyer_is_maker = data.at("m").get<bool>();

        return trade;
    }

} // namespace

MarketDataEvent MarketDataParser::parse(const std::string& message) const {
    try {
        const auto root = json::parse(message);

        const json& data = root.contains("data") ? root.at("data") : root;

        if (!data.is_object()) {
            return UnknownMarketDataEvent{ message };
        }

        if (data.contains("e") && data.at("e").get<std::string>() == "trade") {
            return parse_trade(data);
        }

        if (
            data.contains("b") &&
            data.contains("B") &&
            data.contains("a") &&
            data.contains("A")
            ) {
            return parse_book_ticker(data);
        }

        return UnknownMarketDataEvent{ message };
    }
    catch (const std::exception& ex) {
        std::cerr << "[PARSE_ERROR] " << ex.what()
            << " raw=" << message
            << '\n';

        return UnknownMarketDataEvent{ message };
    }
}
