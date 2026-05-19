# Binance C++ Trading System

A C++20 market data infrastructure project for Binance Spot.  
The current version connects to Binance public WebSocket streams, supports live and record modes, parses BTCUSDT book ticker and trade events, and stores raw and normalized market data for future event-driven HFT backtesting.

## Current Features

- Binance Spot public WebSocket connection
- JSON-based configuration
- `live` mode for real-time market data monitoring
- `record` mode for market data collection
- Raw WebSocket event recording in JSONL format
- Normalized CSV logging for book ticker and trades
- TLS certificate verification using `cacert.pem`
- Windows build with CMake, MSVC and vcpkg

## Architecture

```text
config/config.json
        ↓
Binance WebSocket
        ↓
raw WebSocket message
        ├── raw_events.jsonl
        ↓
MarketDataParser
        ↓
BestBidAsk / Trade
        ├── book_ticker.csv
        └── trades.csv
```

## Requirements

- Windows 10/11
- Visual Studio 2022 Build Tools or Visual Studio Community 2022
- CMake
- vcpkg
- OpenSSL
- Boost.Beast / Boost.Asio
- nlohmann-json

## Build

```PowerShell
Example will be updated soon
cmake --build build --config Release
```

## Configuration

Configuration is stored in:

```
config/config.json
```

## Example:

```json
{
  "host": "stream.binance.com",
  "port": "9443",
  "symbol": "btcusdt",
  "streams": [
    "bookTicker",
    "trade"
  ],
  "cert_file": "cacert.pem",
  "raw_recording_path": "data/recordings/raw_events.jsonl",
  "book_ticker_csv_path": "data/recordings/book_ticker.csv",
  "trades_csv_path": "data/recordings/trades.csv"
}
```

## Usage

### Live mode

Live mode connects to Binance and prints parsed market data without writing files.

```PowerShell
.\build\Release\binance_ws.exe --mode live
```

### Record mode

Record mode connects to Binance, records raw JSONL events, writes normalized CSV files, and stops after the specified duration.

```PowerShell
.\build\Release\binance_ws.exe --mode record --duration-sec 30
```

## Output Files

Record mode creates:

```
data/recordings/raw_events.jsonl
data/recordings/book_ticker.csv
data/recordings/trades.csv
```

`raw_events.jsonl` stores original Binance WebSocket messages with local receive timestamps.
`book_ticker.csv` contains:

```
local_recv_time_ms, symbol, bid_price, bid_qty, ask_price, ask_qty, mid_price, spread_bps
```

`trades.csv` contains:
local_recv_time_ms, symbol, trade_id, price, qty, buyer_is_maker, trade_time_ms

## Roadmap

- Market data replay engine
- Event-driven backtesting mode
- Portfolio and PnL accounting
- Paper broker with market order execution
- Configurable order execution latency
- Latency sweep experiments
- L2 order book reconstruction
- Queue-aware limit order simulation
- Python strategy API

## Disclaimer

This project is for educational and research purposes only.
It does not provide financial advice and should not be used for live trading without proper testing, risk controls and security review.


