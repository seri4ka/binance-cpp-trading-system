#pragma once

#include <string>

struct Trade {
    std::string symbol;
    long long trade_id = 0;

    double price = 0.0;
    double qty = 0.0;

    long long trade_time_ms = 0;
    bool buyer_is_maker = false;
};
