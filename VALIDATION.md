# v3.8.4-SC Validation Record

Validation date: 2026-10-10 (Asia/Seoul).

## Scope and source identity

Based on the unchanged `firmware/nag-killer/nag-killer-v3.8.3-SC` package. This bounded patch changes OTA payload receive idle timeout and failure reporting/retry behavior. Earlier package results are historical evidence and do not validate this patch.

## Focused regression evidence

A compiled probe executing the production OTA handlers confirmed behavioral RED before implementation: the live client retained 5,000 ms instead of the intended 30,000 ms. Focused GREEN passed `test_sc_ota_handler.py` with warnings treated as errors. The probe exercises a successful upload, begin failure, failed write followed by parser abort, partial-byte abort, immediate retry, abort after file END without duplicate byte counting, finalization failure, and a POST without an upload. First Update failures survive abort cleanup; parser-aborted uploads retain diagnostics for GET `/api/ota` without assuming a POST finish callback runs.

The workspace separately exercised the installed ESP32 core 3.3.12 `_uploadReadByte` implementation with virtual time: a 6-second pause fails at the default 5 seconds and succeeds at 30 seconds; a 31-second pause aborts at 30 seconds; disconnect exits promptly; repeated 29-second gaps allow a 58-second total duration. This supplemental SDK probe is not a physical network or flash test and is not a package test dependency.

Dashboard behavioral RED was confirmed for unlocked active-upload controls, falsely successful HTTP 200 replies, invalid attempt attribution, and unbounded diagnostic GETs before each corresponding fix. The final focused run passed 3/3 Python files (10 unittest cases, including the compiled handler probe) and 2/2 JavaScript browser files in 32.883 seconds.

Browser checks covered 12 source/embedded × 320/390/430 px × light/dark scenarios per browser file. OTA coverage includes 48 regular upload paths plus 8 malformed/rejected reply, invalid attempt, and hung diagnostic cases. Both dashboards expose complete wrapping error details without horizontal overflow, restore upload controls after failure, recover only new-attempt diagnostics, suspend status polling during upload, and resume after a diagnostic timeout. Existing all-page, restore-waiting, countdown, and failed-save behavior also passed. The 320 px light source and 390 px dark embedded screenshots were visually inspected; existing typography, controls, cards, and themes were retained.

The targeted generated-dashboard contract rebuilt twice and obtained identical headers. Dashboard sizes: source 19,432 bytes; minified 19,379 bytes; gzip 6,147 bytes. Header SHA-256: `3e92170db6ad1643ebfbe1a092e7205564a5a74fab6609ce8dfd9b74bc9c5c66`.

Final whole-package completion and target compilation passed after review. Paired OTA artifact evidence is recorded below.

## OTA diagnostics semantics

`/api/ota` is read-only and reports the current boot's latest upload attempt, active/failed state, failure stage, Update error code/message, and `receivedBytes`. An interrupted parser does not call the POST completion handler; reconnect and read this endpoint instead. The first Update failure is retained even if abort cleanup changes the SDK's internal error.

`receivedBytes` counts known file payload bytes, including an undelivered partial upload buffer on abort. It excludes multipart framing and any bytes still held privately by the parser. It does not measure wire bytes or prove successful flash writes. After file END, the final buffer is not counted twice. Diagnostics are in RAM, reset on reboot/new upload, and contain no uploaded filename or firmware content.

Dashboard diagnostic GETs have an independent 2.5-second deadline. An unresponsive diagnostic endpoint leaves the original upload error visible and releases the controls; it does not impose an overall deadline on the upload POST. HTTP 200 is accepted as upload success only with an explicit `ok: true` response.

The 30-second limit is an idle limit for each payload read, not an overall upload deadline. Header/body parsing outside the payload reader retains SDK defaults. This patch does not validate product family compatibility of an uploaded image.

## Target configuration and physical boundaries

Required OTA build target remains `esp32:esp32:m5stack_atoms3` (8 MB flash, default OTA partitions, PSRAM disabled), matching v3.8.3-SC. Pins remain TX GPIO 5 / RX GPIO 6, TWAI 500 kbit/s. OTA artifacts contain only the application image.

No physical flash, serial access, live CAN access, controller boot, device OTA, vehicle behavior, or road testing was performed. A host test or successful target build cannot establish Wi-Fi reliability, power-loss tolerance, actual flash health, or vehicle behavior. Verify a slow upload, disconnect/reconnect diagnostics, immediate retry, and normal restart on the installed controller before relying on the patch.

## Final completion and OTA artifacts

- After all review fixes, `firmware_workflow.py completion` passed Python **7/7 files (41 unittest cases)**, C++ **8/8 binaries**, and JavaScript/browser **2/2 files**. Report: `/private/tmp/sc-v384-completion.json`; log: `/private/tmp/sc-v384-completion.log`.
- Independent read-only review found no remaining actionable issue. A fresh official dashboard generation matches the tested/built header SHA-256 `3e92170db6ad1643ebfbe1a092e7205564a5a74fab6609ce8dfd9b74bc9c5c66`.
- ESP32-S3 compile/link passed with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, FQBN `esp32:esp32:m5stack_atoms3` (8 MB flash, default OTA partitions, PSRAM disabled). Program **941,549 / 1,310,720 bytes (71%)**; static RAM **47,392 / 327,680 bytes (14%)**, leaving **280,288 bytes**. Log: `/private/tmp/sc-v384-build.log`.
- OTA `releases/nag-killer-v3.8.4-SC/nag-killer-v3.8.4-SC.bin`: **941,696 bytes**, SHA-256 `7d93e672fe955d4d7ab13ff81f98b2a5f8abc1112df9f2f37821469f1ca45ac4`. A matching deterministic, byte-verified source ZIP is provided in the same directory; artifact sizes/hashes are recorded in `build-evidence.json`.
- Payload idle timeout applies after multipart file START; request/header parsing retains SDK defaults. Abort diagnostics cannot distinguish disconnect from timeout on their own. The reported real-device failure cause remains unconfirmed.
- Product-family admission remains absent: this patch does not prevent SC/Universal cross-installation on compatible ESP32-S3 hardware. No controller flash, boot, actual OTA, serial/CAN access, vehicle or road test was performed.
