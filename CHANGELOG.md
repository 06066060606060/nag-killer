# v3.8.4-SC

## 2026-10-10 — OTA receive timeout and failure diagnostics hotfix

- Set the current OTA payload connection's idle timeout to 30 seconds at upload start. Other WebServer requests retain their SDK defaults.
- Retain the first failure stage, Update error code/message, and known received payload bytes. Return details on failed POST completion and expose `/api/ota` for reconnecting after a parser abort.
- Abort failed Update sessions safely and reset upload state for an immediate retry.
- Pause dashboard status polling during uploads, prevent duplicate clicks/file changes, show device failure details, and recover diagnostics only for a newly started upload attempt.

Based on the unchanged v3.8.3-SC package. CAN policy, vehicle profiles, NVS format, SDK sources, global timeouts, and board settings are unchanged. Physical controller, OTA, vehicle, and road validation have not been performed.
