#pragma once

#include "../market_data/BestBidAsk.hpp"
#include "../market_data/Trade.hpp"

#include <fstream>
#include <string>

class CsvMarketDataWriter {
public:
    CsvMarketDataWriter(
        const std::string& book_ticker_path,
        const std::string& trades_path
    );

    void write_book_ticker(
        long long local_recv_time_ms,
        const BestBidAsk& book
    );

    void write_trade(
        long long local_recv_time_ms,
        const Trade& trade
    );

private:
    std::ofstream book_ticker_file_;
    std::ofstream trades_file_;
};
