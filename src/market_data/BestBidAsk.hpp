#pragma once

#include <string>

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
