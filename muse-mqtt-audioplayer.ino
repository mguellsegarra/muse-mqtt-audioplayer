#include "Arduino.h"
#include "WiFi.h"
#include "SPI.h"
#include "FS.h"
#include "Wire.h"
#include "museWrover.h"
#include "Audio.h"
#include <MQTT.h>
#include <ArduinoJson.h>

// Configuration
char ssid[] = "YourWiFiName";
char password[] = "YourWiFiPassword";
const char *mqtt_server = "192.168.1.100";
const char *mqtt_username = "your_mqtt_user";
const char *mqtt_password = "your_mqtt_password";
const int mqtt_port = 1883;
const char *mqtt_topic = "speaker/control";
const char *speaker_id = "living_room";
const int DEFAULT_VOLUME = 80;               // Default volume level (0-100)
const unsigned long WIFI_RETRY_DELAY = 5000; // 5 seconds delay between WiFi connection attempts

ES8388 es;
Audio audio;
WiFiClient net;
MQTTClient client(4096); // Increased buffer size for large messages

String lastPlayedUrl = "";
bool repeat_mode = false;
unsigned long lastWifiAttempt = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastMqttCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000; // Check WiFi every 5 seconds
const unsigned long MQTT_CHECK_INTERVAL = 2000; // Check MQTT every 2 seconds

void setup()
{
    Serial.begin(115200);
    Serial.println("\r\nReset");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(100);
    }

    Serial.printf_P(PSTR("Connected\r\nRSSI: "));
    Serial.print(WiFi.RSSI());
    Serial.print(" IP: ");
    Serial.println(WiFi.localIP());

    Serial.printf("Connect to ES8388 codec... ");

    while (not es.begin(IIC_DATA, IIC_CLK))
    {
        Serial.printf("Failed!\n");
        delay(1000);
    }
    Serial.printf("OK\n");

    // Enable amplifier
    pinMode(GPIO_PA_EN, OUTPUT);
    digitalWrite(GPIO_PA_EN, HIGH);

    audio.setPinout(I2S_BCLK, I2S_LRCK, I2S_SDOUT, I2S_MCLK);

    // Setup MQTT
    client.begin(mqtt_server, mqtt_port, net);
    client.onMessage(messageReceived);
    connectToMQTT();
}

void loop()
{
    unsigned long currentMillis = millis();

    // Handle WiFi connection
    if (currentMillis - lastWifiCheck >= WIFI_CHECK_INTERVAL)
    {
        lastWifiCheck = currentMillis;

        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi connection lost!");

            if (currentMillis - lastWifiAttempt >= WIFI_RETRY_DELAY)
            {
                lastWifiAttempt = currentMillis;
                Serial.println("Attempting to reconnect WiFi...");

                WiFi.disconnect();
                delay(100); // Give it time to disconnect properly
                WiFi.begin(ssid, password);

                // Wait up to 2 seconds for connection
                int attempt = 0;
                while (WiFi.status() != WL_CONNECTED && attempt < 20)
                {
                    delay(100);
                    attempt++;
                }

                if (WiFi.status() == WL_CONNECTED)
                {
                    Serial.println("\nWiFi reconnected!");
                    Serial.print("IP address: ");
                    Serial.println(WiFi.localIP());
                }
                else
                {
                    Serial.println("\nWiFi reconnection failed!");
                }
            }
        }
    }

    // Handle MQTT connection - only if WiFi is connected
    if (WiFi.status() == WL_CONNECTED)
    {
        if (currentMillis - lastMqttCheck >= MQTT_CHECK_INTERVAL)
        {
            lastMqttCheck = currentMillis;

            if (!client.connected())
            {
                Serial.println("MQTT disconnected, attempting reconnection...");
                connectToMQTT();
            }
        }

        // Only process MQTT loop if connected
        if (client.connected())
        {
            client.loop();
        }
    }

    // Audio processing is independent of network status
    audio.loop();

    // Small delay to prevent watchdog triggers
    delay(10);
}

void audio_info(const char *info)
{
    Serial.print("info        ");
    Serial.println(info);

    // Check if the audio has finished
    if (strstr(info, "End of webstream") || strstr(info, "end of stream") || strstr(info, "stream stopped"))
    {
        if (repeat_mode && lastPlayedUrl.length() > 0)
        {
            Serial.println("Repeat mode is on, replaying URL...");
            delay(500); // Add a small delay before replaying
            audio.connecttohost(lastPlayedUrl.c_str());
            Serial.printf("Replaying URL: %s\n", lastPlayedUrl.c_str());
        }
    }
}

// Modify the volume variable and add a function to handle volume changes
void setVolume(int vol)
{
    // Ensure input volume is within 0-100 range
    vol = constrain(vol, 0, 100);

    // Convert 0-100 range to 0-21 range for Audio library
    int audio_vol = map(vol, 0, 100, 0, 21);

    audio.setVolume(audio_vol); // Set Audio library volume (0-21)

    // Set ES8388 volume directly with 0-100 value
    es.volume(ES8388::ES_MAIN, vol);
    es.volume(ES8388::ES_OUT1, vol);

    Serial.printf("Volume set to: %d%% (Audio: %d, ES8388: %d)\n", vol, audio_vol, vol);
}

void messageReceived(String &topic, String &payload)
{
    Serial.println("MQTT message received:");
    Serial.println(payload);

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Check if this message is for this speaker
    const char *target_speaker = doc["speaker_id"];
    const char *command = doc["command"];

    // Both speaker_id and command are required
    Serial.printf("Target speaker: %s\n", target_speaker ? target_speaker : "null");
    Serial.printf("Command: %s\n", command ? command : "null");
    Serial.printf("Current speaker_id: %s\n", speaker_id);

    if (!target_speaker || !command || String(target_speaker) != speaker_id)
    {
        Serial.println("Message rejected: speaker_id mismatch or missing command");
        return;
    }

    int vol = doc["volume"] | DEFAULT_VOLUME;
    setVolume(vol);

    // Handle commands
    if (strcmp(command, "play") == 0)
    {
        const char *url = doc["url"];
        if (url)
        {
            lastPlayedUrl = String(url);
            audio.connecttohost(url);
            Serial.printf("Playing URL: %s\n", url);
            repeat_mode = doc["repeat"] | false;
            Serial.printf("Repeat mode: %s\n", repeat_mode ? "on" : "off");
        }
    }
    else if (strcmp(command, "google_tts") == 0)
    {
        const char *text = doc["text"];
        const char *language = doc["language"] | "en";
        if (text)
        {
            audio.connecttospeech(text, language);
            Serial.printf("Playing Google TTS: %s (Language: %s)\n", text, language);
            repeat_mode = false; // TTS doesn't support repeat
        }
    }
    else if (strcmp(command, "openai_tts") == 0)
    {
        Serial.println("OpenAI TTS command received");
        const char *text = doc["text"];
        const char *api_key = doc["openai_api_key"];
        const char *model = doc["model"] | "tts-1";
        const char *voice = doc["voice"] | "shimmer";
        Serial.println("OpenAI TTS command received-2");

        if (text && api_key)
        {
            Serial.println("OpenAI TTS Debug Info:");
            Serial.printf("- Text: %s\n", text);
            Serial.printf("- Model: %s\n", model);
            Serial.printf("- Voice: %s\n", voice);
            Serial.println("Attempting to call OpenAI TTS...");

            bool result = audio.openai_speech(api_key, model, text, voice, "mp3", "1");

            if (result)
            {
                Serial.println("OpenAI TTS call successful");
            }
            else
            {
                Serial.println("OpenAI TTS call failed!");
                Serial.println("Please check:");
                Serial.println("1. API key validity");
                Serial.println("2. Internet connection");
                Serial.println("3. Memory availability");
            }
            repeat_mode = false;
        }
        else
        {
            Serial.println("Error: Missing required parameters for OpenAI TTS");
            if (!text)
                Serial.println("- Missing 'text' parameter");
            if (!api_key)
                Serial.println("- Missing 'openai_api_key' parameter");
        }
    }
    else if (strcmp(command, "stop") == 0)
    {
        audio.stopSong();
        repeat_mode = false; // Always disable repeat mode when stopping
        Serial.println("Playback stopped, repeat mode disabled");
    }
}

void connectToMQTT()
{
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "MUSE-MQTT-AudioPlayer-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
        Serial.println("MQTT connected!");
        client.subscribe(mqtt_topic);
        Serial.printf("Subscribed to topic: %s\n", mqtt_topic);
    }
    else
    {
        Serial.println("MQTT connection failed!");
        Serial.println("Will try again in 5 seconds");
        delay(5000);
        return;
    }
}

// Optional; other callbacks for debugging
void audio_id3data(const char *info)
{
    Serial.print("id3data     ");
    Serial.println(info);
}

void audio_eof_mp3(const char *info)
{
    Serial.print("eof_mp3     ");
    Serial.println(info);
}

void audio_showstation(const char *info)
{
    Serial.print("station     ");
    Serial.println(info);
}

void audio_showstreaminfo(const char *info)
{
    Serial.print("streaminfo  ");
    Serial.println(info);
}

void audio_showstreamtitle(const char *info)
{
    Serial.print("streamtitle ");
    Serial.println(info);
}

void audio_bitrate(const char *info)
{
    Serial.print("bitrate     ");
    Serial.println(info);
}

void audio_commercial(const char *info)
{
    Serial.print("commercial  ");
    Serial.println(info);
}

void audio_icyurl(const char *info)
{
    Serial.print("icyurl      ");
    Serial.println(info);
}

void audio_lasthost(const char *info)
{
    Serial.print("lasthost    ");
    Serial.println(info);
}

void audio_eof_speech(const char *info)
{
    Serial.print("eof_speech  ");
    Serial.println(info);
}
