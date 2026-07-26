# ⚠️ TEST VERSION ⚠️
# Nag-killer & Eu-Summon-Unlock for LilyGO/T-2Can

> ⚠️ Research / educational firmware only.
>
> This project interacts with a vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only.It sends signals directly to the controller, not a physical command to the steering wheel. Do not use this on public roads or in any situation where unsafe behavior could put people or property at risk. You are responsible for your own testing, wiring, configuration, and local laws.
---
## Hardware Target

This fork was adapted for:

| Device                       | Can Transceiver                 | CAN RX / CAN TX   | Can Bus      | Power                     |
| ---------------------------- | ------------------------------- | ----------------- | ------------ | ------------------------- |
| LilyGO/T-2Can                |CAN A (MCP2515) → Nag Killer     |                   |              |                           |
|                              |CAN B (TWAI) → Summon Unlock     |                   | 500 kbps CAN | USB-C or stable 12V supply|



---

## Board Setup (Arduino IDE)  
- Board: LilyGo T-Display S3 

## Libraries needed:  
- ESP32 BLE Arduino (built-in)  
- mcp2515 by autowp (install via Library Manager)  
https://github.com/autowp/arduino-mcp2515

## Files in sketch folder:
- T2CAN_Unified.ino  
- index_html.h  
- pin_config.h  

## Wiring
- CAN A (MCP2515): connect to the bus carrying 0x370 / 0x399 / 0x129  
- CAN B (TWAI): connect to the bus carrying 280 / 390 / 921 / 1016 / 1021  

## Dashboard Notes

- The AP name will be something like T2CAN-A1B2 (password: 12345678).  
- Open 192.168.4.1 in a browser.  
- Use the tabs to switch between Nag Echo and Summon Unlock.  
- BLE is also active open URL https://06066060606060.github.io/Summon-Unlock/ in Chrome on Android.

