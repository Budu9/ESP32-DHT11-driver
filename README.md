# ESP32 DHT11 Driver (ESP-IDF)

A bare-metal driver for the DHT11 temperature and humidity sensor, written in C with the ESP-IDF framework. No third-party sensor libraries, the single-wire protocol (start sequence, response detection, bit timing, checksum validation) is implemented directly against GPIO with microsecond-level delays.

## How it works

The DHT11 uses a single-wire, half-duplex protocol over one GPIO pin (I chose GPIO4 by default):

1. **Start sequence** — the ESP32 drives the line high, then low for 18ms to signal the sensor to wake up, then releases the line high for 20–40µs before switching the pin to input mode with a pull-up.
2. **Sensor response** — the DHT11 acknowledges by pulling the line low then high; the driver detects these two edges with a timeout-bounded polling loop (`wait_for_level_change`).
3. **Data read** — 40 bits (5 bytes: humidity integer, humidity decimal, temperature integer, temperature decimal, checksum) are read by measuring the duration of each bit's high pulse. A short pulse (~28µs) decodes to `0`, a long pulse (~70µs) decodes to `1`.
4. **Checksum** — the first four bytes are summed and compared against the fifth to validate the reading before printing temperature and humidity.

RTOS interrupts are disabled (`portDISABLE_INTERRUPTS`) for the duration of the read to keep the microsecond-scale bit timing from being disrupted by task preemption.

## Hardware

- ESP32 (any ESP-IDF-supported variant)
- DHT11 temperature/humidity sensor
- 4.7kΩ–10kΩ pull-up resistor on the data line (or rely on the internal pull-up configured in software)

### Wiring

| DHT11 Pin | ESP32 Pin  |
|-----------|------------|
| VCC       | 3.3V       |
| DATA      | GPIO4      |
| GND       | GND        |

`GPIO_PIN` is defined at the top of the source file and can be changed to any suitable GPIO.

## Building and flashing

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) to be installed and sourced.

```bash
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Expected output on a successful read:

```
startup seq worked
5 bytes read
temp: 24, humidity: 51
```
