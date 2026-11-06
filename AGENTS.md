# AGENTS.md

Operating guide for AI agents working in this repository.

## Overview

- `ed-healthcheck` / `EDHealthCheck` is a small Arduino framework library for ESP32
  that exposes healthcheck HTTP endpoints over ESPAsyncWebServer; it is part of the
  home-automation project family (sibling libs: `ed-utils`, etc.).
- Stack: PlatformIO library, C++ (Arduino ESP32). Declared dependencies in
  `library.json`: **ArduinoJSON**, **ESP32Async/ESPAsyncWebServer**, and **ed-utils**
  (`https://github.com/lan143/ed-utils`).
- JSON building goes through `EDUtils::buildJson` from ed-utils `src/Json.h`, which is
  ArduinoJson v6-based (`DynamicJsonDocument`) — this effectively pins ArduinoJson v6.
- Version source of truth: `library.json` **and** `library.properties` (both currently
  `0.2.0`). Bump both in the same change and add an annotated git tag.
- Known API quirks: the liveness endpoint path is `/healthcheck/healty` and its JSON
  key is `healty` (historical typo, still present after commit `1ff91d4` "Fix endpoint
  path"). The ready endpoint also emits the literal message
  `service success count less min sucess count` (typo `sucess`). These strings are the
  public API — do not "fix" them without explicit approval.

## Layout

- `src/healthcheck.{h,cpp}` — `EDHealthCheck::HealthCheck`:
  `init(checkInterval = 500 ms, successCount = 1, failsCount = 3)`,
  `registerService(Ready*)`, `registerHandlers(AsyncWebServer*)`, `loop()`.
  Internally keeps a `std::list<Service>`.
- `src/service.h` — `Service` wraps a `Ready*`; `check()` calls `ready()`: on failure it
  increments `_failsCount`, resets `_successCount = 0`, stores the message and clears
  `_isWarning`/`_warningMessage`; on warning success (`ReadyResult::isWarning()` with
  `isReady` true) it resets `_failsCount = 0`, increments `_successCount` and stores
  `_warningMessage` with `_isWarning = true`; on clean success it updates the counts
  the same way and clears `_isWarning`/`_warningMessage`.
- `src/ready.h` — abstract `Ready { virtual ReadyResult ready() = 0; }` and
  `ReadyResult(bool isReady, std::string message, bool isWarning = false)`.
- Root manifests: `library.json` (PlatformIO) and `library.properties` (Arduino IDE).
- No `platformio.ini`, `examples/`, `tests/`, or CI. `.vscode/settings.json` (gitignored)
  only contains `files.associations` entries. MIT `LICENSE` is present.

## Endpoints and semantics

- `GET /healthcheck/healty` — always HTTP 200 with body `{"healty":true}`; it does not
  consult services.
- `GET /healthcheck/ready` — HTTP 200 `{"ready":true,"message":""}` only if **every**
  registered service has `getSuccessCount() >= _successCount` **and**
  `getFailsCount() <= _failsCount`; otherwise HTTP 500
  `{"ready":false,"message":...}`. The fail comparison is **strict**
  (`getFailsCount() > _failsCount` means not ready). The body now also contains the
  additive `"warning"` and `"warningMessage"` keys; status-code semantics are unchanged.
  Warning is a per-`Service` state from `ReadyResult::isWarning()`, cleared on the next
  non-warning successful check, and a hard fail dominates it. The same
  last-failing/last-warning-without-break convention applies.
- Services are iterated **without break**: if several are failing, the reported message
  comes from the **last** failing service in registration order.
- `loop()` runs all `Service::check()` passes when
  `(_lastCheckTime + _checkInterval) < millis()`, then updates `_lastCheckTime`.
- A freshly registered service reports **not ready** until the first successful
  `loop()` check (`successCount` starts at 0, below the default `successCount` of 1).

## Commands

Use `rtk` as the output-condensing proxy (see `RTK.md` for the full cheatsheet).

- Browse: `rtk ls`, `rtk tree src`.
- Read: `rtk read src/healthcheck.cpp`.
- Search: `rtk rg 'healthcheck' src`.
- Git: `rtk git status`, `rtk diff`.
- Manifest: `rtk json library.json`.

### Validation (consumer compile)

PlatformIO CLI (`pio`) is installed globally; do **not** add `platformio.ini` to this
library root. There is no test suite, no linter, and no CI. The only gate is a compile
check in a throwaway consumer project **outside** the repo.

1. Create the project:

   ```sh
   mkdir -p /tmp/ed-healthcheck-check && cd /tmp/ed-healthcheck-check
   pio project init --board esp32dev
   ```

2. In its `platformio.ini` add:

   ```ini
   lib_deps = file:///absolute/path/to/ed-healthcheck
   ```

   Transitive deps are resolved from the library's `library.json`; add
   `ArduinoJson@^6` explicitly if resolution complains.

3. Add a `src/main.cpp` that includes `<healthcheck.h>`, instantiates
   `EDHealthCheck::HealthCheck`, subclasses `EDHealthCheck::Ready` with a dummy
   `ready()`, creates `AsyncWebServer server(80)`, and calls
   `init` / `registerService` / `registerHandlers` / `loop`.

4. Compile:

   ```sh
   rtk err pio run
   ```

   Assumption: board `esp32dev`.

Optional runtime smoke check: `curl http://<device-ip>/healthcheck/ready`, expecting
a 200 or 500 JSON response.

## Conventions

- Everything lives in `namespace EDHealthCheck`; `#pragma once` in every header.
- Underscore-prefixed private members (`_services`, `_checkInterval`, ...).
- Keep the existing getter naming style (`getFailsCount`, `getSuccessCount`, `getMessage`).
- 4-space indentation.
- Commits are short and imperative (see `git log`: "Fix bug in service check update",
  "Base code").
- Docs language: `README.md` in Russian; `AGENTS.md` and `RTK.md` in English.

## Invariants and risks

- **Raw pointer lifetime.** `registerService` stores a raw `Ready*`; the consumer owns
  the lifetime, so the object must outlive `HealthCheck`.
- **ArduinoJson v6 lock via ed-utils.** Do not upgrade to v7 — `DynamicJsonDocument` was
  removed in v7.
- **ESPAsyncWebServer fork.** The dependency is the ESP32Async maintained fork, per `library.json`.
- **`millis()` rollover (~49.7 days).** The `(_lastCheckTime + _checkInterval) < millis()`
  comparison is not the rollover-safe `millis() - last >= interval` idiom, so checks
  stall after overflow. Known and intentionally not changed (consistent with sibling
  libs) — accepted risk.
- **Handler lambda captures `this`.** The `HealthCheck` instance (and its services) must
  outlive the `AsyncWebServer`.
- **Concurrency.** `std::list` per-service heap usage is acceptable for a handful of
  services on ESP32, and there is no locking: `loop()` and HTTP handlers touch the same
  list from different async contexts (ESPAsyncWebServer callbacks run on the async TCP
  task while `loop()` runs on the Arduino task). This is a documented known limitation;
  mutation only happens during setup (`registerService` before server start) in
  practice — keep it that way.

## Change checklist

1. Modify `src/` only as needed; keep unrelated files untouched.
2. Run the consumer compile check (`rtk err pio run` in the throwaway project).
3. If public API/behavior changed, update the matching `README.md` section (Russian).
4. If releasing, bump **both** `library.json` and `library.properties`, then add an annotated git tag.
5. Never rename the `healty` / `sucess` wire strings without explicit approval.

## Do not

- Do not add `platformio.ini`, `examples/`, or `tests/` unless explicitly asked.
- Do not upgrade ArduinoJson or replace `EDUtils::buildJson` usage.
- Do not fix the `healty` typo by default — it is the deployed API contract.
- Do not reformat unrelated files.
- Do not `git add`, commit, or push unless instructed (remote is SSH:
  `git@github.com:lan143/ed-healthcheck.git`).
