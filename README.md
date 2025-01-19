# 🔊 MUSE Luxe MQTT Audio Player

An Arduino sketch for ESP32-based [MUSE Luxe Speaker](https://raspiaudio.com/product/esp-muse-luxe/) that enables network audio streaming via MQTT control. Features include WiFi connectivity, ES8388 codec support, volume control, and playback management through MQTT commands.

## 💡 Background

This project was born out of frustration with commercial smart speakers in home automation setups. After years of struggling with [Google Home and Google Nest devices](https://community.home-assistant.io/t/i-did-it-i-defeated-the-horrible-google-home-cast-start-prompt-sound/36123/40), where the mandatory chime before any sound playback was a constant annoyance, I needed a better solution. This open-source, local-only device provides:

- Direct sound playback without unnecessary chimes or delays
- Complete control through MQTT for easy Home Assistant integration
- Ability to loop sounds (perfect for alarms and notifications)
- No cloud dependencies or internet requirements
- Full control over the playback experience

If you've ever been frustrated by the limitations of commercial smart speakers in your home automation setup, this project might be exactly what you're looking for.

## 🌟 Features

- WiFi connectivity with auto-reconnect
- MQTT control interface
- ES8388 codec support
- Adjustable volume control (0-100%)
- Stream playback with repeat mode

## 🛠️ Prerequisites

### Hardware

- [MUSE Luxe ESP32 Board](https://raspiaudio.com/product/esp-muse-luxe/)

### Software

- [Arduino IDE](https://www.arduino.cc/en/software)
- Required Libraries:
  - [ESP32 Board Support Package](https://github.com/espressif/arduino-esp32)
  - [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)
  - [PubSubClient](https://github.com/knolleary/pubsubclient)
  - [ArduinoJson](https://arduinojson.org/)

## ⚙️ Installation

1. Clone this repository:

```bash
git clone https://github.com/mguellsegarra/muse-mqtt-audioplayer.git
```

2. Open the project in Arduino IDE.

3. Configure the WiFi credentials and MQTT server details in the `muse-mqtt-audioplayer.ino` file:

```cpp
char ssid[] = "YourWiFiName";
char password[] = "YourWiFiPassword";
const char mqtt_server = "192.168.1.100";
const char mqtt_username = "your_mqtt_user";
const char mqtt_password = "your_mqtt_password";
```

4. Upload the sketch to your MUSE Luxe board.

5. Use the MQTT commands to control the audio player.

## 📡 MQTT Control

### Topic Structure

- Default topic: `speaker/control`
- Each command should include `speaker_id` for targeting specific devices

### Commands

#### Play Audio

```json
{
    "speaker_id": "living_room",
    "command": "play",
    "url": "http://example.com/audio.mp3"
}
```

#### Stop playback

```json
{
    "speaker_id": "living_room",
    "command": "stop"
}
```

#### Play Audio with Volume

```json
{
    "speaker_id": "living_room",
    "command": "play",
    "url": "http://example.com/audio.mp3",
    "volume": 80
}
```

#### Play Audio in Repeat mode

```json
{
    "speaker_id": "living_room",
    "command": "play",
    "url": "http://example.com/alarm.mp3",
    "volume": 100,
    "repeat": true
}
```

#### Play Radio Station

```json
{
    "speaker_id": "living_room",
    "command": "play",
    "url": "http://direct.fipradio.fr/live/fip-midfi.mp3",
}
```

## 📊 Status Updates

The device provides real-time status updates through Serial output, including:

- Connection status (WiFi & MQTT)
- Playback information
- Stream metadata
- Error messages

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- ESP32 Community
- RASPIAUDIO and the MUSE Luxe Development Team
- Contributors to the required libraries
