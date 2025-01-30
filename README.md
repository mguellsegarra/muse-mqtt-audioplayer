# 🔊 MUSE Luxe MQTT Audio Player

An Arduino sketch for ESP32-based [RASPIAUDIO](https://github.com/RASPIAUDIO) [MUSE Luxe Speaker](https://raspiaudio.com/product/esp-muse-luxe/) that enables network audio streaming via MQTT control. Features include WiFi connectivity, ES8388 codec support, volume control, battery monitoring, and playback management through MQTT commands.

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
- MQTT control interface with speaker-specific topics
- ES8388 codec support
- Adjustable volume control (0-100%)
- Stream playback with repeat mode
- Play Google TTS
- Play OpenAI TTS
- Battery level monitoring
- Charging status reporting
- Dynamic status updates (1s when playing, 5min when idle)

## 🛠️ Prerequisites

### Hardware

- [MUSE Luxe ESP32 Board](https://raspiaudio.com/product/esp-muse-luxe/)

### Software

- [Arduino IDE](https://www.arduino.cc/en/software)
- Required Libraries:
  - Tested on ESP32 version 3.0.7
  - [Muse Library](https://github.com/RASPIAUDIO/Muse_library)
  - [ESP32-audioI2S-master](https://github.com/schreibfaul1/ESP32-audioI2S)
  - [MQTT.h](https://github.com/256dpi/arduino-mqtt)
  - [ArduinoJson](https://arduinojson.org/)

## ⚙️ Installation

1. Clone this repository:

```bash
git clone https://github.com/mguellsegarra/muse-mqtt-audioplayer.git
```

2. Open the project in Arduino IDE.

3. Install the required libraries through Arduino Library Manager:

    - [Muse Library](https://github.com/RASPIAUDIO/Muse_library)
    - [ESP32-audioI2S-master](https://github.com/schreibfaul1/ESP32-audioI2S)
    - [MQTT.h](https://github.com/256dpi/arduino-mqtt)

4. If not already installed, install also these libraries:

    - [ArduinoJson](https://arduinojson.org/)

5. Set the board to `ESP32 Dev Module`

6. Configure the connection settings in the code:

   ```cpp
   char ssid[] = "YourWiFiName";
   char password[] = "YourWiFiPassword";
   const char *mqtt_server = "192.168.1.100";
   const char *mqtt_username = "your_mqtt_user";
   const char *mqtt_password = "your_mqtt_password";
   ```

7. Configure the speaker settings in the code:

   ```cpp
   const char *speaker_id = "living_room";
   const char *base_topic = "speaker";  // Base topic for all MQTT messages
   ```

   This will create the following MQTT topics:
   - Control: `speaker/living_room/control`
   - Status: `speaker/living_room/status`

8. Upload the sketch to your MUSE Luxe board.

9. Use the MQTT commands to control the audio player and enjoy 🎉

## 📡 MQTT Control

### Topic Structure

Each speaker uses two MQTT topics based on its ID:
- Control topic: `speaker/{speaker_id}/control` (e.g., `speaker/living_room/control`)
- Status topic: `speaker/{speaker_id}/status` (e.g., `speaker/living_room/status`)

### Commands

#### Play Audio ▶️

```json
{
    "command": "play",
    "url": "http://example.com/audio.mp3"
}
```

#### Stop playback ⏹️

```json
{
    "command": "stop"
}
```

#### Play Audio with Volume 🔊

```json
{
    "command": "play",
    "url": "http://example.com/audio.mp3",
    "volume": 80
}
```

#### Play Audio with Repeat 🔁

```json
{
    "command": "play",
    "url": "http://example.com/alarm.mp3",
    "volume": 100,
    "repeat": true
}
```

#### You can also play live radio stations 📻

```json
{
    "command": "play",
    "url": "http://direct.fipradio.fr/live/fip-midfi.mp3"
}
```

#### Play Google TTS

```json
{
    "command": "google_tts",
    "text": "Hello Raspiaudio, this text was generated using google speech API",
    "language": "en",
    "volume": 80
}
```

#### Play OpenAI TTS

```json
{
    "command": "openai_tts",
    "text": "Your text to convert to speech",
    "openai_api_key": "your-openai-api-key",
    "model": "tts-1",
    "voice": "shimmer",
    "volume": 80
}
```

> **Note**: The TTS commands do not support the `repeat` parameter. Only `volume` can be adjusted for these commands.
> For OpenAI TTS, you'll need a valid billable OpenAI API key.

## 📊 Status Updates

The device provides real-time status updates through its status topic (`speaker/{speaker_id}/status`):

- Every 1 second when playing audio
- Every 5 minutes when idle
- Immediately when state changes (play/stop)

Status message format:
```json
{
    "status": "playing",          // "playing" or "idle"
    "repeat_mode": false,         // true if repeat is enabled
    "battery_level": 85,          // battery percentage
    "is_charging": true,          // charging status
    "current_url": "http://...",  // current playing URL
    "volume": 80                  // current volume (0-100)
}
```

## 🐛 Troubleshooting

If you get some error like:

```text
Sketch uses 1477137 bytes (112%) of program storage space. Maximum is 1310720 bytes.
Global variables use 63848 bytes (19%) of dynamic memory, leaving 263832 bytes for local variables. Maximum is 327680 bytes.
Sketch too big; see https://support.arduino.cc/hc/en-us/articles/360013825179 for tips on reducing it.
text section exceeds available space in board

Compilation error: text section exceeds available space in board
```

You can try to change the Partition Scheme to `NO OTA (2 MB APP/2 MB SPIFFS)` in the Arduino IDE.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- ESP32 Community
- RASPIAUDIO and the MUSE Luxe Development Team for this fantastic piece of open source hardware :)
- Contributors to the required libraries

## 🙋🏽‍♂️ Author

I'm Marc Güell Segarra, a freelance software developer at [Ondori.dev](https://ondori.dev).

## ☕ Buy Me a Coffee

If you found this project useful, consider **[buying me a coffee](https://buymeacoffee.com/mguellsegarra)!**
