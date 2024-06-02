#include <Wire.h>
#include <DHT.h>
#include <WiFiNINA.h>

// Constants
#define DHTPIN 2             // Pin connected to the DHT11 sensor
#define DHTTYPE DHT11        // DHT 11
#define SOIL_MOISTURE_PIN A0 // Pin connected to the soil moisture sensor
#define RELAY_PIN 3          // Pin connected to the relay controlling the water pump

// Thresholds
#define SOIL_MOISTURE_THRESHOLD_LOW 400  // Threshold for soil moisture (below this value means the soil is dry)
#define SOIL_MOISTURE_THRESHOLD_HIGH 600 // Threshold for soil moisture (above this value means the soil is wet)
#define WATERING_DURATION 2000           // Watering duration in milliseconds

// WiFi and IFTTT setup
char ssid[] = "S22Nisal";
char pass[] = "nisal2004";
char HOST_NAME[] = "maker.ifttt.com";
String PATH_NAME = "/trigger/soil_readings/with/key/PwZS4Xw-FXkukWG8TRCaV";
String queryString = "?value1=";

WiFiClient client;
DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;
const long interval = 10000; // Interval in milliseconds (10 seconds)
bool firstReading = true;

void setup() {
  // Start serial communication
  Serial.begin(9600);

  // Initialize the DHT sensor
  dht.begin();

  // Set relay pin as output
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Ensure the pump is off initially (assuming HIGH means off)

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Display startup message
  Serial.println("Plant Watering System Initialized");
  delay(2000);
}

void loop() {
  unsigned long currentMillis = millis();

  // Read temperature and humidity from DHT11 sensor every loop iteration
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if it's time to read soil moisture
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Save the last time we read the sensor

    // Read soil moisture level
    int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);

    // Print readings to the Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" °C, Humidity: ");
    Serial.print(h);
    Serial.print(" %, Soil Moisture: ");
    Serial.println(soilMoistureValue);

    // Check soil moisture levels and take appropriate actions
    if (soilMoistureValue < SOIL_MOISTURE_THRESHOLD_LOW) {
      // Soil moisture is low, wait 12 seconds before watering the plant
      Serial.print("Soil moisture is low (");
      Serial.print(soilMoistureValue);
      Serial.println("). Waiting 12 seconds before watering the plant...");
      delay(12000); // 12-second delay

      // Turn on the water pump (assuming LOW turns the pump on)
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Watering the plant...");
      delay(WATERING_DURATION);

      // Turn off the water pump
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Watering done.");

      // Send alert via IFTTT
      sendIFTTTEvent(PATH_NAME + queryString + String(soilMoistureValue));
    } else if (soilMoistureValue >= SOIL_MOISTURE_THRESHOLD_LOW && soilMoistureValue <= SOIL_MOISTURE_THRESHOLD_HIGH) {
      // Soil moisture is optimal
      Serial.print("Soil moisture is sufficient and at optimal level (");
      Serial.print(soilMoistureValue);
      Serial.println("). No watering needed.");
    } else {
      // Soil moisture is high
      Serial.print("Soil is wet (");
      Serial.print(soilMoistureValue);
      Serial.println("). No watering needed.");
    }
  }

  delay(1000); // Wait for a second before taking the next reading
}

void sendIFTTTEvent(String path) {
  // Send HTTP request to IFTTT Webhooks
  if (client.connect(HOST_NAME, 80)) {
    client.println("GET " + path + " HTTP/1.1");
    client.println("Host: " + String(HOST_NAME));
    client.println("Connection: close");
    client.println();
    Serial.println("HTTP GET request sent to IFTTT with soil moisture readings");

    // Wait for the server's response and print the status line only
    while (client.connected() || client.available()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          // Headers are finished, we can break out of the loop
          break;
        }
      }
    }

    // Print the response body
    while (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
    client.stop();
  } else {
    Serial.println("Failed to send HTTP GET request to IFTTT");
  }
}
