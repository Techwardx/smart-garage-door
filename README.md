# Smart Garage Door Controller

Arduino-based garage door controller with Wi-Fi connectivity and Home Assistant integration.

## Features

- Remote garage door control via HTTP
- Home Assistant integration
- Hall effect sensor for door position detection
- Multiple iterations from prototype to production

## Project Structure

| Directory | Description |
|-----------|-------------|
| `draft/` | Initial prototype |
| `hardware_test/` | Hardware component testing |
| `wifi_http_home_assistant_test/` | Wi-Fi and Home Assistant integration testing |
| `hall_module/` | Hall effect sensor calibration |
| `final_test/` | Pre-deployment testing |
| `oprate_24_7/` | Production 24/7 operation version |
| `final/` | Final polished version |

## Hardware

- Arduino (ESP8266/ESP32)
- Hall effect sensor (door position)
- Relay module (door trigger)
- Wi-Fi module for HTTP communication

## Setup

1. Open the `.ino` file in Arduino IDE
2. Configure your Wi-Fi credentials and Home Assistant endpoint
3. Flash to your board
