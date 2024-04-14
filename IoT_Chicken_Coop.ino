#include <WiFi.h>  // Include the WiFi library for ESP32
#include <PubSubClient.h>  // Include the MQTT library for ESP32
#include <DHT.h>  // Include the DHT sensor library

// Add the library for the light sensor
#include <Wire.h>  // Include the Wire library for I2C communication
#include <BH1750.h>  // Include the BH1750 library for light sensor

// Replace the next variables with your SSID/Password combination
const char* ssid = "GOOVAERTSBUREAU";  // WiFi network SSID
const char* password = "0475876973";  // WiFi network password

// Add your MQTT Broker IP address (You can find your ip with the "ifconfig" command in Debian/Ubuntu)
const char* mqtt_server = "192.168.0.146";  // MQTT Broker IP address

WiFiClient espClient;  // Create a WiFi client
PubSubClient client(espClient);  // Create an MQTT client
long lastMsg = 0;  // Variable to store the last time a message was sent
char msg[50];  // Buffer to hold MQTT message
int value = 0;  // Variable to hold a value

#define DHTPIN 15  // Pin where DHT11 is connected
#define DHTTYPE DHT11  // Type DHT-sensor

DHT dht(DHTPIN, DHTTYPE);  // Create a DHT sensor instance
float temperature = 0;  // Variable to hold temperature value
float humidity = 0;  // Variable to hold humidity value

// LED Pin
const int ledPin = 4;  // Define the pin connected to the LED

// Define the pins for the light sensor BH1750
#define SDA_PIN 21  // Define the SDA pin for I2C communication
#define SCL_PIN 22  // Define the SCL pin for I2C communication

// Create an instance of the light sensor BH1750
BH1750 lightSensor;  // Create a BH1750 sensor instance

void setup() {
  Serial.begin(115200);  // Start serial communication
  dht.begin();  // Initialize DHT sensor

  // Initialize the light sensor
  Wire.begin(SDA_PIN, SCL_PIN);  // Initialize I2C communication
  lightSensor.begin();  // Initialize light sensor

  // Make sure you use the right port (1883 by default) If no connection is made, check the listener port on the mqtt broker.
  setup_wifi();  // Call function to set up WiFi connection
  client.setServer(mqtt_server, 1883);  // Set MQTT server and port
  client.setCallback(callback);  // Set callback function for MQTT

  pinMode(ledPin, OUTPUT);  // Set LED pin as output
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);  // Connect to WiFi network

  while (WiFi.status() != WL_CONNECTED) {  // Wait for WiFi connection
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  // Print local IP address
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(ledPin, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(ledPin, LOW);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {  // Send MQTT messages every 5 seconds
    lastMsg = now;
    
    // Read temperature and humidity from DHT sensor
    temperature = dht.readTemperature();   
    humidity = dht.readHumidity();

    // Read light level from light sensor
    float lightLevel = lightSensor.readLightLevel();
    Serial.print("Light Level: ");
    Serial.println(lightLevel);

    // Turn on LED if it's dark
    if (lightLevel < 100) {  // Adjust this threshold based on your light sensor readings
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }

    // Convert the values to char arrays
    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    Serial.print("Temperature: ");
    Serial.println(tempString);
    client.publish("esp32/temperature", tempString);  // Publish temperature value to MQTT

    char humString[8];
    dtostrf(humidity, 1, 2, humString);
    Serial.print("Humidity: ");
    Serial.println(humString);
    client.publish("esp32/humidity", humString);  // Publish humidity value to MQTT

    // Publish light level data
    char lightString[8];
    dtostrf(lightLevel, 1, 2, lightString);
    Serial.print("Light Level: ");
    Serial.println(lightString);
    client.publish("esp32/light_level", lightString);  // Publish light level value to MQTT
  }
}
