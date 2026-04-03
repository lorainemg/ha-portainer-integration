# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**ha-portainer-integration** is a C++ tool that connects to Home Assistant via WebSocket, generates a Lovelace dashboard view showing Portainer container/stack status, and saves it back to HA. Designed to be re-run whenever stacks change in Portainer.

- **License:** MIT
- **Language:** C++17
- **Remote:** https://github.com/lorainemg/ha-portainer-integration.git

## Build & Run

```bash
# Build the Docker image (only needed when dependencies change)
docker compose build

# Build inside container
docker compose run --rm ha-portainer bash -c "cmake -B build && cmake --build build"

# Run
docker compose run --rm ha-portainer bash -c "cmake -B build && cmake --build build && ./build/ha-portainer"

# Run tests
docker compose run --rm ha-portainer bash -c "cmake -B build && cmake --build build && cd build && ctest --output-on-failure"
```

Requires a `.env` file with:
```
HA_URL=wss://your-ha-instance/api/websocket
HA_TOKEN=your_long_lived_access_token
PORTAINER_URL=https://your-portainer-instance
```

## Architecture

```
src/
  main.cpp                          # Entry point: env setup, stack definitions, orchestration
  ws/                               # WebSocket abstraction layer
    i_ws_connection.h               # Interface (pure virtual)
    boost_ws_connection.h/.cpp      # Real implementation using Boost.Beast + SSL
  ha/                               # Home Assistant client
    ha_client.h/.cpp                # WebSocket auth + dashboard CRUD via HA WebSocket API
  dashboard/                        # Dashboard generation
    stack.h                         # Container/Stack data structs
    dashboard_updater.h/.cpp        # Generates Portainer Lovelace view JSON
tests/
  ha_client_test.cpp                # HA client tests (mocked WebSocket)
  dashboard_updater_test.cpp        # Dashboard updater tests (pure JSON logic)
```

**Key design decisions:**
- `IWebSocketConnection` interface enables mocking in tests via Google Mock
- `HomeAssistantClient` uses dependency injection (receives the connection, doesn't create it)
- `DashboardUpdater` is pure logic — JSON in, JSON out, no side effects
- Stack definitions are hardcoded in `main.cpp` (see Future Improvements)

## Dependencies

- **Boost.Beast** — WebSocket + SSL (installed via apt in Docker)
- **nlohmann/json** — JSON handling (installed via apt in Docker)
- **OpenSSL** — TLS for wss:// connections
- **Google Test + Google Mock** — testing (fetched via CMake FetchContent)

## Development Approach

- **Incremental steps:** Implement features in small, well-explained increments. Explain what will be done before doing it, and summarize what was done after.
- **Teaching focus:** Use this project to introduce C++ concepts (memory management, RAII, templates, STL, smart pointers, etc.), drawing analogies to Python/C#/TS where helpful. Never use a C++ concept without fully explaining it first — do not assume the user already knows syntax from prior passing mentions.
- **Extensibility:** The dashboard generation should be easy to extend as new containers/stacks are added to Portainer.

## Future Improvements

- **Config file** — read stack definitions from a JSON/YAML file instead of hardcoding in C++, so adding a stack doesn't require recompiling
- **Header HTML** — add the tailwindcss template card header to the generated Portainer view
- **Error handling** — graceful handling of HA unreachable, dashboard not found, etc.
- **Auto-discovery** — pull stack/container info directly from the Portainer API instead of manual definitions
- **Async** — convert to Boost.Asio coroutines for non-blocking I/O