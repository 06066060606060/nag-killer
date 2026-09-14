# Validation notes — v3.5a1-SC

Validation for this source package covers source invariants and dashboard integrity.

- Working-v3.1/Mode-H runtime source retained; only the firmware version string changes in the `.ino`.
- `nag_human_pure.h` is byte-identical to the v3.1H1 input package.
- Production dashboard contains no demo/mock fetch interception.
- Dashboard real API endpoints are retained.
- `dashboard_source.html` and the HTML embedded in `index_html.h` are byte-identical.
- JavaScript syntax and host C++ header syntax are checked during packaging.
- A native Arduino-ESP32 toolchain build is not claimed unless separately compiled on the target toolchain.
