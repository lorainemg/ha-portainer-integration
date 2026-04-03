# ha-portainer-integration

A C++ tool that auto-generates a Home Assistant Lovelace dashboard view for monitoring Portainer containers and stacks.

## What it does

1. Connects to Home Assistant via WebSocket
2. Fetches the current dashboard configuration
3. Generates (or replaces) a Portainer monitoring view with:
   - Docker host overview (containers, images, CPU, memory)
   - Per-stack status cards (standalone or expandable for multi-container stacks)
   - Container controls (power, restart)
   - Status badges
4. Saves the updated dashboard back to HA

## Prerequisites

- Docker and Docker Compose
- A Home Assistant instance with the [Portainer integration](https://github.com/tomaae/homeassistant-portainer)
- A [long-lived access token](https://developers.home-assistant.io/docs/auth_api/#long-lived-access-token) from HA

## Setup

1. Clone the repository:
   ```bash
   git clone https://github.com/lorainemg/ha-portainer-integration.git
   cd ha-portainer-integration
   ```

2. Create a `.env` file:
   ```
   HA_URL=wss://your-ha-instance/api/websocket
   HA_TOKEN=your_long_lived_access_token_here
   PORTAINER_URL=https://your-portainer-instance
   ```

3. Edit the stack definitions in `src/main.cpp` to match your Portainer setup.

5. Build and run:
   ```bash
   docker compose build
   docker compose run --rm ha-portainer bash -c "cmake -B build && cmake --build build && ./build/ha-portainer"
   ```

## Running Tests

```bash
docker compose run --rm ha-portainer bash -c "cmake -B build && cmake --build build && cd build && ctest --output-on-failure"
```

## Tech Stack

- **C++17** with CMake
- **Boost.Beast** for WebSocket + SSL
- **nlohmann/json** for JSON handling
- **Google Test / Google Mock** for testing

## License

MIT