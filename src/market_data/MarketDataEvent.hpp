#pragma once

#include "BestBidAsk.hpp"
#include "Trade.hpp"

#include <string>
#include <variant>

struct UnknownMarketDataEvent {
    std::string raw_message;
};

using MarketDataEvent = std::variant<
    BestBidAsk,
    Trade,
    UnknownMarketDataEvent
>;
