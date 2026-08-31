ARDUINO_CLI ?= $(shell command -v arduino-cli 2>/dev/null || printf '%s' '/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli')
VERSION := $(shell awk -F'"' '/FIRMWARE_VERSION/ { print $$2; exit }' nag-killer.ino)

DEFAULT_FQBN ?= esp32:esp32:esp32s3
WAVESHARE_FQBN ?= esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi
PORT ?=

.PHONY: build build-default build-waveshare ports upload upload-default upload-waveshare

## BUILDING
build:
	rm -rf build/release
	make build-default
	make build-atom-s3
	make build-waveshare

build-default:
	mkdir -p "build/default"
	mkdir -p "build/release"
	"$(ARDUINO_CLI)" compile --fqbn "$(DEFAULT_FQBN)" --output-dir "build/default" .
	cp "build/default/nag-killer.ino.bin" "build/release/nag-killer-$(VERSION)-default.bin"
	cp "build/default/nag-killer.ino.bootloader.bin" "build/release/nag-killer-$(VERSION)-default.bootloader.bin"
	cp "build/default/nag-killer.ino.partitions.bin" "build/release/nag-killer-$(VERSION)-default.partitions.bin"

build-atom-s3:
	mkdir -p "build/atom-s3"
	mkdir -p "build/release"
	"$(ARDUINO_CLI)" compile --fqbn "$(DEFAULT_FQBN)" --output-dir "build/atom-s3" --build-property "compiler.cpp.extra_flags=-DCAN_RX_PIN=6 -DCAN_TX_PIN=5" .
	cp "build/atom-s3/nag-killer.ino.bin" "build/release/nag-killer-$(VERSION)-atom-s3.bin"
	cp "build/atom-s3/nag-killer.ino.bootloader.bin" "build/release/nag-killer-$(VERSION)-atom-s3.bootloader.bin"
	cp "build/atom-s3/nag-killer.ino.partitions.bin" "build/release/nag-killer-$(VERSION)-atom-s3.partitions.bin"

build-waveshare:
	mkdir -p "build/waveshare"
	mkdir -p "build/release"
	"$(ARDUINO_CLI)" compile --fqbn "$(WAVESHARE_FQBN)" --output-dir "build/waveshare" --build-property "compiler.cpp.extra_flags=-DCAN_RX_PIN=16 -DCAN_TX_PIN=15" .
	cp "build/waveshare/nag-killer.ino.bin" "build/release/nag-killer-$(VERSION)-waveshare.bin"
	cp "build/waveshare/nag-killer.ino.bootloader.bin" "build/release/nag-killer-$(VERSION)-waveshare.bootloader.bin"
	cp "build/waveshare/nag-killer.ino.partitions.bin" "build/release/nag-killer-$(VERSION)-waveshare.partitions.bin"

## UPLOAD TO DEVICE
## example: `make upload-default PORT=/dev/cu.usbmodem1101`
ports:
	"$(ARDUINO_CLI)" board list

upload-default:
	@test -n "$(PORT)" || { printf '%s\n' "Set PORT first. Run 'make ports', then use: make upload-default PORT=/dev/cu.usbmodem..." >&2; exit 2; }
	@if [ ! -f "build/release/nag-killer-$(VERSION)-default.bin" ] || [ ! -f "build/release/nag-killer-$(VERSION)-default.bootloader.bin" ] || [ ! -f "build/release/nag-killer-$(VERSION)-default.partitions.bin" ]; then $(MAKE) build-default; fi
	"$(ARDUINO_CLI)" upload --fqbn "$(DEFAULT_FQBN)" --port "$(PORT)" --input-file "build/release/nag-killer-$(VERSION)-default.bin"

upload-waveshare:
	@test -n "$(PORT)" || { printf '%s\n' "Set PORT first. Run 'make ports', then use: make upload PORT=/dev/cu.usbmodem..." >&2; exit 2; }
	@if [ ! -f "build/release/nag-killer-$(VERSION)-waveshare.bin" ] || [ ! -f "build/release/nag-killer-$(VERSION)-waveshare.bootloader.bin" ] || [ ! -f "build/release/nag-killer-$(VERSION)-waveshare.partitions.bin" ]; then $(MAKE) build-waveshare; fi
	"$(ARDUINO_CLI)" upload --fqbn "$(WAVESHARE_FQBN)" --port "$(PORT)" --input-file "build/release/nag-killer-$(VERSION)-waveshare.bin"
