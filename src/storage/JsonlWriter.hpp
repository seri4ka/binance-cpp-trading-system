#pragma once

#include <fstream>
#include <string>

class JsonlWriter {
public:
    explicit JsonlWriter(const std::string& path);

    void write_raw_message(
        long long local_recv_time_ms,
        const std::string& raw_message
    );

private:
    std::ofstream file_;
};
