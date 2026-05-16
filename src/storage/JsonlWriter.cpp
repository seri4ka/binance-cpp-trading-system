#include "JsonlWriter.hpp"

#include <filesystem>
#include <stdexcept>

JsonlWriter::JsonlWriter(const std::string& path) {
    const std::filesystem::path file_path(path);
    const auto parent_path = file_path.parent_path();

    if (!parent_path.empty()) {
        std::filesystem::create_directories(parent_path);
    }

    file_.open(path, std::ios::out | std::ios::trunc);

    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open JSONL file: " + path);
    }
}

void JsonlWriter::write_raw_message(
    long long local_recv_time_ms,
    const std::string& raw_message
) {
    /*
     * raw_message is expected to be a valid JSON object received from Binance.
     * Therefore we can embed it directly as the value of "raw".
     */
    file_ << "{\"local_recv_time_ms\":"
        << local_recv_time_ms
        << ",\"raw\":"
        << raw_message
        << "}\n";
}
