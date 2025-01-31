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
const char *speaker_id = "living_room";
const char *base_topic = "speaker";                        // Base topic path for all MQTT messages
const int DEFAULT_VOLUME = 100;                            // Default volume level (0-100)
const unsigned long WIFI_RETRY_DELAY = 5000;               // 5 seconds delay between WiFi connection attempts
const unsigned long STATUS_UPDATE_INTERVAL_PLAYING = 1000; // Status update interval when playing (1 second)
const unsigned long STATUS_UPDATE_INTERVAL_IDLE = 300000;  // Status update interval when idle (5 minutes)

String mqtt_status_topic;  // Topic for status updates
String mqtt_control_topic; // Topic for control messages

ES8388 es;
Audio audio;
WiFiClient net;
MQTTClient client(4096); // Increased buffer size for large messages
MuseLuxe museluxe;       // Add MuseLuxe instance for battery monitoring

String lastPlayedUrl = "";
bool repeat_mode = false;
bool is_playing = false;
int current_volume = DEFAULT_VOLUME; // Track current volume
unsigned long lastWifiAttempt = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastMqttCheck = 0;
unsigned long lastStatusUpdate = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000; // Check WiFi every 5 seconds
const unsigned long MQTT_CHECK_INTERVAL = 2000; // Check MQTT every 2 seconds

void setup()
{
    Serial.begin(115200);
    Serial.println("\n[SETUP] Starting...");

    // Construct the MQTT topics using base_topic and speaker_id
    mqtt_status_topic = String(base_topic) + "/" + speaker_id + "/status";
    mqtt_control_topic = String(base_topic) + "/" + speaker_id + "/control";
    Serial.printf("[SETUP] Status topic: %s\n", mqtt_status_topic.c_str());
    Serial.printf("[SETUP] Control topic: %s\n", mqtt_control_topic.c_str());

    museluxe.begin(); // Initialize MuseLuxe

    WiFi.mode(WIFI_STA);
    Serial.printf("[SETUP] Attempting to connect to %s\n", ssid);
    WiFi.begin(ssid, password);

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 50)
    {
        delay(100);
        attempt++;
        if (attempt % 10 == 0)
        {
            Serial.printf("[SETUP] WiFi status: %d\n", WiFi.status());
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("[SETUP] WiFi Connected!");
        Serial.printf("[SETUP] IP address: %s\n", WiFi.localIP().toString().c_str());
    }
    else
    {
        Serial.println("[SETUP] WiFi connection failed!");
    }

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
            Serial.println("\n[WiFi] Connection lost!");

            if (currentMillis - lastWifiAttempt >= WIFI_RETRY_DELAY)
            {
                lastWifiAttempt = currentMillis;
                Serial.println("[WiFi] Starting reconnection process...");

                WiFi.disconnect(true); // Added true to disconnect completely
                Serial.println("[WiFi] Disconnected");
                delay(1000);

                Serial.printf("[WiFi] Attempting to connect to %s\n", ssid);
                WiFi.begin(ssid, password);

                // Wait up to 5 seconds for connection
                int attempt = 0;
                while (WiFi.status() != WL_CONNECTED && attempt < 50)
                {
                    delay(100);
                    attempt++;
                }

                if (WiFi.status() == WL_CONNECTED)
                {
                    Serial.println("[WiFi] Successfully reconnected!");
                    Serial.printf("[WiFi] IP address: %s\n", WiFi.localIP().toString().c_str());
                }
                else
                {
                    Serial.printf("[WiFi] Reconnection failed! Status: %d\n", WiFi.status());
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
                Serial.println("[MQTT] Disconnected, attempting reconnection...");
                connectToMQTT();
            }
        }

        // Only process MQTT loop if connected
        if (client.connected())
        {
            client.loop();
        }
    }

    // Handle status updates with dynamic interval
    unsigned long statusInterval = is_playing ? STATUS_UPDATE_INTERVAL_PLAYING : STATUS_UPDATE_INTERVAL_IDLE;
    if (currentMillis - lastStatusUpdate >= statusInterval)
    {
        lastStatusUpdate = currentMillis;
        publishStatus();
    }

    // Audio processing is independent of network status
    audio.loop();

    // Small delay to prevent watchdog triggers
    delay(10);
}

void publishStatus()
{
    if (!client.connected())
    {
        return;
    }

    StaticJsonDocument<512> status;
    status["status"] = is_playing ? "playing" : "idle";
    status["repeat_mode"] = repeat_mode;
    status["battery_level"] = museluxe.getBatteryPercentage();
    status["is_charging"] = museluxe.isCharging();
    status["current_url"] = lastPlayedUrl;
    status["volume"] = current_volume;

    String statusJson;
    serializeJson(status, statusJson);

    client.publish(mqtt_status_topic.c_str(), statusJson.c_str());
}

void audio_info(const char *info)
{
    Serial.print("info        ");
    Serial.println(info);

    // Update playing status based on audio info
    if (strstr(info, "End of webstream") || strstr(info, "end of stream") || strstr(info, "stream stopped"))
    {
        is_playing = false;

        if (repeat_mode && lastPlayedUrl.length() > 0)
        {
            Serial.println("Repeat mode is on, replaying URL...");
            delay(500);
            audio.connecttohost(lastPlayedUrl.c_str());
            Serial.printf("Replaying URL: %s\n", lastPlayedUrl.c_str());
            is_playing = true;
        }
        else
        {
            lastPlayedUrl = ""; // Only clear the URL if we're not repeating
            publishStatus();    // Immediately publish the status change
        }
    }
    else if (strstr(info, "connect to:"))
    {
        is_playing = true;
        publishStatus(); // Immediately publish the status change
    }
}

// Modify the volume variable and add a function to handle volume changes
void setVolume(int vol)
{
    // Ensure input volume is within 0-100 range
    vol = constrain(vol, 0, 100);
    current_volume = vol; // Store current volume

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

    const char *command = doc["command"];

    // Command is required
    if (!command)
    {
        Serial.println("Message rejected: missing command");
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
            is_playing = true;
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
        is_playing = false;
        repeat_mode = false;
        Serial.println("Playback stopped, repeat mode disabled");
    }
}

void connectToMQTT()
{
    Serial.println("[MQTT] Starting connection process...");
    // Create a random client ID
    String clientId = "MUSE-MQTT-AudioPlayer-";
    clientId += String(random(0xffff), HEX);
    Serial.printf("[MQTT] Using client ID: %s\n", clientId.c_str());

    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
        Serial.println("[MQTT] Successfully connected!");
        Serial.printf("[MQTT] Subscribing to topic: %s\n", mqtt_control_topic.c_str());
        if (client.subscribe(mqtt_control_topic.c_str()))
        {
            Serial.println("[MQTT] Successfully subscribed!");
        }
        else
        {
            Serial.println("[MQTT] Failed to subscribe!");
        }
    }
    else
    {
        Serial.println("[MQTT] Connection failed!");
        Serial.println("[MQTT] Will try again in 5 seconds");
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
