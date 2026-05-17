#include "CsvMarketDataWriter.hpp"

#include <filesystem>
#include <iomanip>
#include <stdexcept>

namespace {

    void ensure_parent_directory_exists(const std::string& path) {
        const std::filesystem::path file_path(path);
        const auto parent_path = file_path.parent_path();

        if (!parent_path.empty()) {
            std::filesystem::create_directories(parent_path);
        }
    }

} // namespace

CsvMarketDataWriter::CsvMarketDataWriter(
    const std::string& book_ticker_path,
    const std::string& trades_path
) {
    ensure_parent_directory_exists(book_ticker_path);
    ensure_parent_directory_exists(trades_path);

    book_ticker_file_.open(book_ticker_path, std::ios::out | std::ios::trunc);
    trades_file_.open(trades_path, std::ios::out | std::ios::trunc);

    if (!book_ticker_file_.is_open()) {
        throw std::runtime_error(
            "Failed to open book ticker CSV file: " + book_ticker_path
        );
    }

    if (!trades_file_.is_open()) {
        throw std::runtime_error(
            "Failed to open trades CSV file: " + trades_path
        );
    }

    book_ticker_file_
        << "local_recv_time_ms,"
        << "symbol,"
        << "bid_price,"
        << "bid_qty,"
        << "ask_price,"
        << "ask_qty,"
        << "mid_price,"
        << "spread_bps\n";

    trades_file_
        << "local_recv_time_ms,"
        << "symbol,"
        << "trade_id,"
        << "price,"
        << "qty,"
        << "buyer_is_maker,"
        << "trade_time_ms\n";
}

void CsvMarketDataWriter::write_book_ticker(
    long long local_recv_time_ms,
    const BestBidAsk& book
) {
    book_ticker_file_
        << local_recv_time_ms << ','
        << book.symbol << ','
        << std::fixed << std::setprecision(8)
        << book.bid_price << ','
        << book.bid_qty << ','
        << book.ask_price << ','
        << book.ask_qty << ','
        << book.mid_price() << ','
        << std::setprecision(6)
        << book.spread_bps()
        << '\n';
}

void CsvMarketDataWriter::write_trade(
    long long local_recv_time_ms,
    const Trade& trade
) {
    trades_file_
        << local_recv_time_ms << ','
        << trade.symbol << ','
        << trade.trade_id << ','
        << std::fixed << std::setprecision(8)
        << trade.price << ','
        << trade.qty << ','
        << std::boolalpha
        << trade.buyer_is_maker << ','
        << trade.trade_time_ms
        << '\n';
}
