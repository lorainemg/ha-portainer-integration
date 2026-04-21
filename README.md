# ha-portainer-integration

A C++ tool that auto-discovers Portainer stacks/containers, lets you curate which ones land in Home Assistant, and generates a Lovelace dashboard view for monitoring them.

## What it does

1. Queries the Portainer API for current stacks and containers
2. Compares against the curated `stacks.yaml` on disk and prompts you for any new/removed stacks or containers (y/n/s/q)
3. Writes your decisions back to `stacks.yaml` (approvals → active list, declines → `ignored:` / `ignored_containers:`)
4. Connects to Home Assistant via WebSocket and generates (or replaces) the Portainer monitoring view:
   - Docker host overview (containers, images, CPU, memory)
   - Per-stack status cards (standalone or expandable for multi-container stacks)
   - Container controls (power, restart)
   - Status badges
5. Saves the updated dashboard back to HA

## Prerequisites

- Docker and Docker Compose
- A Home Assistant instance with the [Portainer integration](https://github.com/tomaae/homeassistant-portainer)
- A [long-lived access token](https://developers.home-assistant.io/docs/auth_api/#long-lived-access-token) from HA
- A Portainer API access token (Portainer UI → *My account → Access tokens*)

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
   PORTAINER_TOKEN=your_portainer_api_key
   PORTAINER_ENDPOINT_ID=3   # optional, defaults to 3
   ```

3. Build and run:
   ```bash
   docker compose build
   docker compose run --rm ha-portainer bash -c "cmake -B build && cmake --build build && ./build/ha-portainer"
   ```

   The first run discovers all your Portainer stacks and prompts you for each one. Approved stacks are persisted to `stacks.yaml`; declined ones go into an `ignored:` list so you won't be re-asked. On later runs, only drift vs. Portainer triggers new prompts — if nothing changed, the dashboard is just re-pushed silently.

4. To polish the dashboard, edit `stacks.yaml` by hand: the tool never overwrites `name` or `icon` on entries it already wrote, so you can swap default `mdi:cube-outline` for whatever you prefer.

## Running Tests

```bash
docker compose run --rm ha-portainer bash -c "cmake -B build && cmake --build build && cd build && ctest --output-on-failure"
```

## Tech Stack

- **C++17** with CMake
- **Boost.Beast** for WebSocket + HTTPS + SSL
- **nlohmann/json** for JSON handling
- **yaml-cpp** for YAML round-tripping
- **Google Test / Google Mock** for testing

## License

MIT