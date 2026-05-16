#pragma once

#include "MarketDataEvent.hpp"

#include <string>

class MarketDataParser {
public:
    [[nodiscard]] MarketDataEvent parse(const std::string& message) const;
};
