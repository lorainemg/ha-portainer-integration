# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**ha-portainer-integration** is a C++ tool that connects to Home Assistant via WebSocket, generates a Lovelace dashboard view showing Portainer container/stack status, and saves it back to HA. Designed to be re-run whenever stacks change in Portainer.

- **License:** MIT
- **Language:** C++17
- **Remote:** https://github.com/lorainemg/ha-portainer-integration.git

## Build & Run

All common workflows are exposed through the top-level `Makefile`. Running `make` with no arguments prints the help.

```bash
make build   # configure and compile inside the container
make test    # build + run the full test suite via ctest
make run     # build + execute the ha-portainer binary
make shell   # drop into a bash shell inside the container
make clean   # remove the build/ directory
make image   # rebuild the Docker image (only needed when Dockerfile changes)
```

Each target wraps `docker compose run --rm ha-portainer <command>` — the container is ephemeral and the repo is bind-mounted at `/app`, so build artifacts persist in `./build/` on the host between runs.

Requires a `.env` file with:
```
HA_URL=wss://your-ha-instance/api/websocket
HA_TOKEN=your_long_lived_access_token
PORTAINER_URL=https://your-portainer-instance
PORTAINER_TOKEN=your_portainer_api_key
PORTAINER_ENDPOINT_ID=3   # optional, defaults to 3
```

## Architecture

```
src/
  main.cpp                          # Entry point: env, orchestration (discover -> reconcile -> prompt -> save -> push)
  errors.h                          # Centralized exception hierarchy
  ws/                               # WebSocket abstraction
    i_ws_connection.h               # Interface (pure virtual)
    boost_ws_connection.h/.cpp      # Real impl using Boost.Beast + SSL
  ha/                               # Home Assistant client
    ha_client.h/.cpp                # WebSocket auth + dashboard CRUD
  dashboard/                        # Dashboard generation
    stack.h                         # Container / Stack / YamlState data
    dashboard_updater.h/.cpp        # Generates Lovelace view JSON
  config/                           # YAML round-trip
    config_parser.h/.cpp            # loadYamlState / saveYamlState
  portainer/                        # Portainer API client
    i_http_client.h                 # HTTPS GET interface (pure virtual)
    boost_http_client.h/.cpp        # Boost.Beast implementation
    portainer_models.h              # PortainerStack / PortainerContainer DTOs
    portainer_client.h/.cpp         # listStacks / listAllContainers
    slug.h/.cpp                     # Slug derivation utility
  discovery/                        # Reconciliation + interactive flow
    diff_report.h                   # DiffReport / Decisions / NewStack / GoneStack / ... structs
    reconciler.h/.cpp               # Pure-function diff: yaml vs Portainer
    prompter.h/.cpp                 # Interactive y/n/s/q prompts
    apply.h/.cpp                    # Mutates YamlState from Decisions
tests/
  ha_client_test.cpp, dashboard_updater_test.cpp, config_parser_test.cpp,
  slug_test.cpp, portainer_client_test.cpp, reconciler_test.cpp,
  prompter_test.cpp, apply_test.cpp
```

**Key design decisions:**
- `IWebSocketConnection` / `IHttpClient` interfaces enable mocking in tests via Google Mock
- Domain clients (`HomeAssistantClient`, `PortainerClient`) use dependency injection
- `DashboardUpdater`, `reconcile`, `applyDecisions` are pure logic — no I/O, no mocks needed to test
- `stacks.yaml` is tool-written, user-curated: the tool appends new discoveries and prompts to remove gone ones; `name` and `icon` once written are never auto-edited

## Dependencies

- **Boost.Beast** — WebSocket + SSL (installed via apt in Docker)
- **nlohmann/json** — JSON handling (installed via apt in Docker)
- **OpenSSL** — TLS for wss:// connections
- **Google Test + Google Mock** — testing (fetched via CMake FetchContent)

## Development Approach

- **Incremental steps:** Implement features in small, well-explained increments. Explain what will be done before doing it, and summarize what was done after.
- **Teaching focus:** Use this project to introduce C++ concepts (memory management, RAII, templates, STL, smart pointers, etc.), drawing analogies to Python/C#/TS where helpful. Never use a C++ concept without fully explaining it first — do not assume the user already knows syntax from prior passing mentions.
- **Extensibility:** The dashboard generation should be easy to extend as new containers/stacks are added to Portainer.

## Git Conventions

- **Commit messages:** keep on a single line. No multi-line bodies, no bulleted explanations in the message body.

## Future Improvements

- **Async** — convert to Boost.Asio coroutines for non-blocking I/O