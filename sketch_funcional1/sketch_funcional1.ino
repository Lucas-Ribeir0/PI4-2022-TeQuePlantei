#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <CayenneMQTTESP8266.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <math.h>
#include "DHT.h"

// Autenticação Cayenne
char username[] = "b7667fb0-5207-11ed-baf6-35fab7fd0ac8";
char password[] = "4642726cb274b85fdb3d74c3d44b5c7d30e4a6e3";
char clientID[] = "6eb8a230-523d-11ed-bf0a-bb4ba43bd3f6";


#define DHTPIN 5     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

#define DHTTYPE DHT11 // DHT 11

// Connect pin 1 (on the left) of the sensor to 3.3V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN (5) is
// Connect pin 3 (on the right) of the sensor to GROUND (if your sensor has 3 pins)

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);


// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


// Variáveis Conexão WI-FI
#define WIFI_SSID "Ribeiro's Fibra"
#define WIFI_PASSWORD "familia1751"

// Autenticação com o Firebase
#define API_KEY "AIzaSyA9hcAP-aB6IAQNm3_d5--jTnLlXKHyXpw"

#define USER_EMAIL "tequeplantei@gmail.com"
#define USER_PASSWORD "tequeplantei"

#define DATABASE_URL "project-pi-2022-default-rtdb.firebaseio.com"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;

// Database child nodes
String timeStampPath = "/dadosProjeto/data";
String groundUmidityPath = "/dadosProjeto/umidadeSolo";
String airUmidityPath = "/dadosProjeto/umidadeAr";
String temperaturePath = "/dadosProjeto/temperatura";

// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current epoch time
int timestamp;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 900000;


// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}


void setup() {
  Cayenne.begin(username, password, clientID);

  Serial.begin(115200);
  initWiFi();
  timeClient.begin();
  dht.begin();


  pinMode(A0, INPUT);
  
  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  databasePath = "/ProjectData/leituras";
}

// Variáveis de coleta de dados
  float umidadeSolo;
  float umidadeAr;
  float temperatura;
  float valorSensor;
  float umidPercent;
  float temperaturaRounded;

void loop() { 
  umidadeAr = dht.readHumidity();
  temperatura = dht.readTemperature();

  valorSensor = analogRead(A0);

  umidPercent = map(valorSensor, 0, 1023, 100, 0);

  

  temperaturaRounded = roundf(temperatura * 100) /100;

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.println ("time: ");
    Serial.println (timestamp);


    if (isnan(temperatura) || isnan(umidadeAr)) {
      Serial.println("Failed to read from DHT");
      umidadeAr = 0;
      temperatura = 0;
    } else {
      Serial.print("Umidade Ar: ");
      Serial.println(umidadeAr);
      Serial.print("Temperatura: ");
      Serial.println(temperatura);
      Serial.print("Umidade Solo: ");
      Serial.println(umidPercent);
    }

    parentPath= databasePath + "/" + String(timestamp);

    json.set(groundUmidityPath.c_str(), String(umidPercent));
    json.set(airUmidityPath.c_str(), String(umidadeAr));
    json.set(temperaturePath.c_str(), String(temperatura));
    json.set(timeStampPath.c_str(), String(timestamp));

    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
  
  Cayenne.loop(60000);
}

CAYENNE_OUT_DEFAULT(){
    Serial.println("Sending Data to Cayenne!");

    Cayenne.celsiusWrite(0, temperaturaRounded);
    Cayenne.virtualWrite(1, umidadeAr, "rel_hum", "p");
    Cayenne.virtualWrite(2, umidPercent, "soil_moist", "p");
}

CAYENNE_IN_DEFAULT()
{
	CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
	//Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");

	CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.setError("This is an error!"));
}
