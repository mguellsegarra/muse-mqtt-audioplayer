# 🔊 MUSE Luxe MQTT Audio Player

An Arduino sketch for ESP32-based [RASPIAUDIO](https://github.com/RASPIAUDIO) [MUSE Luxe Speaker](https://raspiaudio.com/product/esp-muse-luxe/) that enables network audio streaming via MQTT control. Features include WiFi connectivity, ES8388 codec support, volume control, and playback management through MQTT commands.

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
  - Tested on ESP32 version 3.0.7
  - [Muse Library](https://github.com/RASPIAUDIO/Muse_library)
  - [ESP32-audioI2S-master](https://github.com/schreibfaul1/ESP32-audioI2S)
  - [PubSubClient](https://github.com/knolleary/pubsubclient)
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

4. If not already installed, install also these libraries:

    - [PubSubClient](https://github.com/knolleary/pubsubclient)
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

7. Also you have to configure the `speaker_id` in the code and if you want, the `mqtt_topic` for controlling it:

   ```cpp
   const char *speaker_id = "living_room";
   const char *mqtt_topic = "speaker/control";
   ```

8. Upload the sketch to your MUSE Luxe board.

9. Use the MQTT commands to control the audio player and enjoy 🎉

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
- RASPIAUDIO and the MUSE Luxe Development Team for this fantastic piece of open source hardware :)
- Contributors to the required libraries

##  Author 🙋🏽‍♂️

I'm Marc Güell Segarra, a freelance software developer at [Ondori.dev](https://ondori.dev).

##  Buy Me a Coffee ☕

If you found this extension useful, consider **[buying me a coffee](https://buymeacoffee.com/mguellsegarra)!**
