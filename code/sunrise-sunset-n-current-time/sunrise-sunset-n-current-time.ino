#include <WiFi.h>              // For ESP32 Wi-Fi connectivity
#include <HTTPClient.h>        // For API requests
#include <Adafruit_NeoPixel.h> // NeoPixel library
#include <ArduinoJson.h>       // For JSON parsing

#define NEOPIXEL_PIN 17
#define NUM_PIXELS 8
#define SDPT_SWITCH_PIN 38

Adafruit_NeoPixel strip(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Wi-Fi credentials
const char* ssid = "-YOUR WIFI NAME-";
const char* password = "-YOUR WIFI PASSWORD-";

// API Endpoints
const char* apiUrl = "https://api.sunrise-sunset.org/json?lat=40.7128&lng=-74.0060&formatted=0";
const char* timeApiUrl = "http://worldtimeapi.org/api/timezone/America/New_York";

// Function prototypes
String fetchTimeData();
String fetchCurrentTime();
String parseCurrentPhase(const String& jsonData, const String& currentTime);
void setNeoPixelColor(uint8_t r, uint8_t g, uint8_t b);

void setup() {
  pinMode(SDPT_SWITCH_PIN, INPUT_PULLUP);
  strip.begin();
  strip.show();

  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  if (digitalRead(SDPT_SWITCH_PIN) == LOW) {
    String jsonData = fetchTimeData();
    String currentTime = fetchCurrentTime();

    if (!jsonData.isEmpty() && !currentTime.isEmpty()) {
      // Parse the JSON and determine the current phase
      String phase = parseCurrentPhase(jsonData, currentTime);

      // Map phases to colors
      if (phase == "nautical_twilight_begin" || phase == "nautical_twilight_end") {
        setNeoPixelColor(210, 162, 38);
      } else if (phase == "civil_twilight_begin" || phase == "civil_twilight_end") {
        setNeoPixelColor(255, 216, 115);
      } else if (phase == "sunrise" || phase == "sunset") {
        setNeoPixelColor(255, 229, 121);
      } else if (phase == "solar_noon") {
        setNeoPixelColor(252, 246, 220);
      }
    }
  } else {
    strip.clear();
    strip.show();
  }

  delay(60000); // Update every minute
}

String fetchTimeData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiUrl);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();
      http.end();
      return payload;
    } else {
      Serial.println("Failed to fetch data");
      http.end();
      return "";
    }
  }
  return "";
}

String fetchCurrentTime() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(timeApiUrl);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();
      http.end();

      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.println("Failed to parse current time JSON");
        return "";
      }

      return doc["datetime"].as<String>(); // Returns the full ISO 8601 datetime string
    } else {
      Serial.println("Failed to fetch current time");
      http.end();
      return "";
    }
  }
  return "";
}

String parseCurrentPhase(const String& jsonData, const String& currentTime) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    Serial.println("Failed to parse JSON");
    return "";
  }

  if (doc.containsKey("results")) {
    JsonObject results = doc["results"];

    String nauticalTwilightBegin = results["nautical_twilight_begin"].as<String>();
    String civilTwilightBegin = results["civil_twilight_begin"].as<String>();
    String sunrise = results["sunrise"].as<String>();
    String solarNoon = results["solar_noon"].as<String>();
    String sunset = results["sunset"].as<String>();
    String civilTwilightEnd = results["civil_twilight_end"].as<String>();
    String nauticalTwilightEnd = results["nautical_twilight_end"].as<String>();

    if (currentTime >= nauticalTwilightBegin && currentTime < civilTwilightBegin) {
      return "nautical_twilight_begin";
    } else if (currentTime >= civilTwilightBegin && currentTime < sunrise) {
      return "civil_twilight_begin";
    } else if (currentTime >= sunrise && currentTime < solarNoon) {
      return "sunrise";
    } else if (currentTime == solarNoon) {
      return "solar_noon";
    } else if (currentTime > solarNoon && currentTime <= sunset) {
      return "sunset";
    } else if (currentTime > sunset && currentTime <= civilTwilightEnd) {
      return "civil_twilight_end";
    } else if (currentTime > civilTwilightEnd && currentTime <= nauticalTwilightEnd) {
      return "nautical_twilight_end";
    }
  }

  return "";
}

void setNeoPixelColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}
