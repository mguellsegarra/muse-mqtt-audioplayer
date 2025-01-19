#include "Arduino.h"
#include "WiFi.h"
#include "SPI.h"
#include "FS.h"
#include "Wire.h"
#include "museWrover.h"
#include "Audio.h"
#include "PubSubClient.h"
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
WiFiClient espClient;
PubSubClient client(espClient);

String lastPlayedUrl = "";
bool repeat_mode = false;
unsigned long lastWifiAttempt = 0;

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
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    reconnectMQTT();
}

void loop()
{
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi connection lost!");

        // Only try to reconnect after delay
        if (millis() - lastWifiAttempt > WIFI_RETRY_DELAY)
        {
            Serial.println("Attempting to reconnect WiFi...");

            WiFi.disconnect();
            WiFi.begin(ssid, password);

            // Wait a bit to see if connection is successful
            int attempt = 0;
            while (WiFi.status() != WL_CONNECTED && attempt < 20)
            {
                delay(100);
                attempt++;
                Serial.print(".");
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.println("\nWiFi reconnected!");
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());

                // Reconnect MQTT after WiFi is back
                if (!client.connected())
                {
                    reconnectMQTT();
                }
            }
            else
            {
                Serial.println("\nWiFi reconnection failed!");
            }

            lastWifiAttempt = millis();
        }
    }
    else
    {
        // Check MQTT connection only if WiFi is connected
        if (!client.connected())
        {
            reconnectMQTT();
        }
        client.loop();
    }

    audio.loop();
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

// Modify the callback function
void callback(char *topic, uint8_t *payload, unsigned int length)
{
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);

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
    if (!target_speaker || !command || String(target_speaker) != speaker_id)
    {
        return;
    }

    // Handle commands
    if (strcmp(command, "play") == 0)
    {
        const char *url = doc["url"];
        if (url)
        {
            lastPlayedUrl = String(url);
            audio.connecttohost(url);
            Serial.printf("Playing URL: %s\n", url);

            // Handle volume with default value of 80
            int vol = doc["volume"] | DEFAULT_VOLUME;
            setVolume(vol);

            // Handle repeat mode
            repeat_mode = doc["repeat"] | false;
            Serial.printf("Repeat mode: %s\n", repeat_mode ? "on" : "off");
        }
    }
    else if (strcmp(command, "stop") == 0)
    {
        audio.stopSong();
        repeat_mode = false; // Always disable repeat mode when stopping
        Serial.println("Playback stopped, repeat mode disabled");
    }
}

// Function to reconnect to MQTT
void reconnectMQTT()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "MUSE-MQTT-AudioPlayer-";
        clientId += String(random(0xffff), HEX);

        if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
        {
            Serial.println("MQTT connected");
            client.subscribe(mqtt_topic);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println("Try again in 5 seconds");
            delay(5000);
        }
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
