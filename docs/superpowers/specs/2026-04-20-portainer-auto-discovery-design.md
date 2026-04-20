# Portainer Auto-Discovery — Design

**Status:** Approved (design phase)
**Date:** 2026-04-20
**Author:** Loraine Monteagudo (with Claude)

## Goal

Replace the hand-maintained `stacks.yaml` with a tool-assisted reconciliation flow that discovers stacks and containers from Portainer's API, prompts the user to approve new discoveries, and persists the curated result back to `stacks.yaml`. The dashboard generation continues to read from `stacks.yaml` unchanged.

## Non-goals

- Async I/O (kept synchronous; tracked separately as a future improvement).
- Multi-endpoint Portainer support (single endpoint, configurable).
- Auto-curating display `name` and `icon` fields (those remain user-owned).
- Cron/CI scheduling (the tool is interactive by design when drift exists).

## Summary of decisions

| Decision | Choice |
|---|---|
| Discovery scope | Auto-discover all stacks/containers from Portainer; persist & curate via `stacks.yaml`. |
| Execution model | One mode. Interactive only when there is drift; silent passthrough otherwise. |
| Authentication | Long-lived API key in `PORTAINER_TOKEN` env var (`X-API-Key` header). |
| File ownership | User owns `stacks.yaml`. Tool only adds new entries (with defaults) and prompts to remove gone ones. Tool never edits existing `name` / `icon`. |
| Endpoint selection | Env var `PORTAINER_ENDPOINT_ID`, defaults to `3`. |
| Decline memory | Per-stack ignores at the top-level `ignored:` list; per-container ignores under each stack's `ignored_containers:` list. |
| HTTP client | Reuse Boost.Beast (already linked for SSL/WebSocket). |
| Errors | New `PortainerError` in `errors.h`, fail-loud, exit 1. |

## Architecture

Five new pieces, each with a single responsibility. Each is testable in isolation.

```
src/
  portainer/
    i_http_client.h           # Pure-virtual HTTP interface, mockable in tests
    boost_http_client.h/.cpp  # Boost.Beast implementation (HTTPS GET)
    portainer_client.h/.cpp   # Domain operations: listStacks(), listContainers(stack_id)
    portainer_models.h        # DTOs: PortainerStack, PortainerContainer
  discovery/
    diff_report.h             # Plain data: new_stacks, gone_stacks, new_containers, gone_containers, id_changes
    reconciler.h/.cpp         # Pure function: (YamlState, PortainerState) -> DiffReport
    prompter.h/.cpp           # Interactive: walks DiffReport, asks user, returns Decisions
  config/
    config_parser.cpp         # Extended: existing loadStacks() + new saveStacks()
  main.cpp                    # Orchestration / wiring
```

### Why this split

- **`portainer_client`** does only HTTP+JSON. No business logic. Mockable via `IHttpClient` (mirrors how `HomeAssistantClient` uses `IWebSocketConnection`).
- **`reconciler`** does only set comparison. Pure function with no I/O, no prompts. Fully unit-testable without mocks.
- **`prompter`** does only stdin/stdout. Receives a `DiffReport`, returns `Decisions`. Testable by feeding a fake `std::istream`.
- The orchestration in `main.cpp` is pure wiring; all logic lives in pieces that can be poked at in isolation.

## yaml schema

```yaml
stacks:
  - slug: immich
    name: Immich
    icon: mdi:image-multiple
    portainer_id: 1
    containers:
      - slug: immich_server
        name: Server
        icon: mdi:server
      - slug: immich_postgres
        name: Postgres
        icon: mdi:database
    ignored_containers:        # NEW — declined containers within this stack
      - immich_redis

  # ... other approved stacks ...

ignored:                        # NEW — top-level, declined whole stacks
  - some_dev_only_stack
  - watchtower
```

### Contract rules (enforced by the reconciler)

1. **`slug` is identity.** A stack/container is "the same one" between runs iff its slug matches. Slugs are never auto-changed once written.
2. **`portainer_id` is mutable.** If Portainer rebuilds and the ID changes, the tool silently updates it on next run (slug still matches).
3. **`name`, `icon` are user-owned.** Tool writes a default once on creation; never touches them again.
4. **`containers[]` is reconciler-managed.** The tool adds new containers (with default `name` / `icon`) and prompts to remove gone ones. Never reorders or rewrites existing entries.
5. **`ignored_containers[]` and top-level `ignored:`** are append-only via the tool. To un-decline, the user deletes a slug from the list by hand.
6. **Precedence:** if a slug appears in both `stacks:` and `ignored:` (or in both `containers:` and `ignored_containers:`), the active list wins — the entry is treated as approved and the duplicate `ignored` entry is left in place (the reconciler does not auto-clean it; this can only happen via manual yaml edits).

### Defaults written for a brand-new entry

| Field | Default |
|---|---|
| Stack `name` | Portainer's stack name as-is (e.g. `"Home Assistant"`) |
| Stack `icon` | `mdi:cube-outline` |
| Container `name` | Portainer's container name as-is |
| Container `icon` | `mdi:application-outline` |

### Slug derivation

Used for new stack entries. Container slugs are taken verbatim from the Docker container name (which is already lowercase-with-underscores from docker-compose, e.g. `immich_server`).

```
"Home Assistant"   → "home_assistant"
"Trakt TG Bot"     → "trakt_tg_bot"
"my-cool-stack"    → "my_cool_stack"
```

Algorithm: lowercase, replace any run of non-alphanumeric chars with a single `_`, trim leading/trailing `_`.

## Control flow on a single run

```
1. Connect to Portainer  →  fetch stacks for endpoint N
                         →  for each stack, fetch its containers
2. Load stacks.yaml      →  YamlState (approved + ignored)
3. Reconciler            →  produce DiffReport:
                              new_stacks         (in Portainer, not in yaml.stacks, not in yaml.ignored)
                              gone_stacks        (in yaml.stacks, not in Portainer)
                              new_containers     (per approved stack: in Portainer, not in yaml, not in ignored_containers)
                              gone_containers    (per approved stack: in yaml, not in Portainer)
                              id_changes         (slug matches, portainer_id changed — applied silently)
4. If DiffReport has no human-decision items (no new_stacks, gone_stacks, new_containers, or gone_containers):
       → silent: skip prompts; if id_changes is also empty, skip yaml write and go to step 7;
         if id_changes is non-empty, apply them and continue to step 5
   Else:
       → Prompter walks the diff (excluding id_changes, which are silent), asks user for each item
       → Decisions get applied: yaml.stacks gets entries added/removed,
         yaml.ignored / ignored_containers get appended on "no"
5. Write stacks.yaml back to disk (only if changed).
6. Connect to HA via existing HomeAssistantClient.
7. DashboardUpdater generates view from yaml.stacks → push to HA.
```

The HA push always happens — even on a silent no-diff run — because a yaml change since last push (e.g. user edited an icon) might still need to be reflected in HA.

## Interactive prompt UX

For each diff entry, the prompter asks `[y / n / s / q]`:

- `y` — yes (add to `stacks` / remove from `stacks` depending on the diff kind)
- `n` — no, persistently (append slug to `ignored:` / `ignored_containers:`)
- `s` — skip this run (no yaml change, will be re-prompted next run)
- `q` — quit (abort the whole run, write nothing, push nothing)

Example session:

```
Detected changes vs. Portainer:

[1/3] New stack in Portainer: "monitoring" (id=14)
      Add to dashboard? [y/n/s/q] y
[2/3] New stack in Portainer: "watchtower" (id=15)
      Add to dashboard? [y/n/s/q] n
      → Added to ignored.
[3/3] Container "immich_redis" disappeared from Immich stack
      Remove from yaml? [y/n/s/q] y
```

## HTTP client, errors, env vars

- **HTTP client:** Boost.Beast. Already linked for WebSocket+SSL; HTTPS GET is a small extension of the same APIs. No new dependency.
- **Errors:** new `PortainerError` in `errors.h` (sibling to `ConfigError`, `WebSocketError`, etc.). Thrown on connection failures, non-2xx HTTP, malformed JSON. `main.cpp`'s existing top-level catch handles it like everything else — fail loud, exit 1.
- **New runtime dependency:** the tool now requires Portainer to be reachable on every run, not just the URL string. Previously `PORTAINER_URL` was used only for building dashboard hyperlinks; auto-discovery makes it a hard runtime dependency. If Portainer is down, the dashboard does not get pushed (no fallback to existing yaml). This is intentional — pushing a stale dashboard while Portainer is down would mask the problem.
- **New env vars in `.env`:**
  - `PORTAINER_TOKEN` — required, the API key
  - `PORTAINER_ENDPOINT_ID` — optional, defaults to `3`
  - `PORTAINER_URL` — already exists, reused as the API base

## Testing strategy

- **`portainer_client_test`** — mocks `IHttpClient`; asserts the right requests are sent and responses parse into the right DTOs. Mirrors how `ha_client_test` mocks the WebSocket today.
- **`reconciler_test`** — pure-function tests, no mocks. Big table of `(yaml_state, portainer_state) → expected DiffReport` cases. This is where most bug-catching value lives; reconciler is where the logic is.
- **`prompter_test`** — feeds a fake `std::istream` (e.g. a `std::stringstream` of `"y\nn\ny\n"`); asserts the resulting `Decisions` struct.
- **`config_parser_test`** — extended for round-trip: `loadStacks(write(stacks)) == stacks`. Catches yaml-format regressions.

## Open implementation choices (decided in plan, not spec)

- Exact API endpoints used (`/api/stacks?filters=...` vs. `/api/endpoints/{id}/docker/...` for containers).
- Whether `saveStacks()` rewrites the whole file or attempts to preserve user comments / formatting (yaml-cpp does not preserve comments by default — likely accepted as a known limitation).
- Implementation order / build sequence.

These are deferred to the implementation plan.