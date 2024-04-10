#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

const char* ssid = "#LaboD5";
const char* password = "0123456789";

WebServer server(80);

// Capteur BME680
Adafruit_BME680 bme;

// Altitude de référence pour le calcul de l'altitude
#define SEALEVELPRESSURE_HPA (1013.25)

const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);
  
  // Lecture des données du capteur BME680
  float temperature = bme.temperature;
  float pressure = bme.pressure / 100.0;
  float humidity = bme.humidity;
  float gas_resistance = bme.gas_resistance / 1000.0;
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  // Construction de la réponse HTML avec les données du capteur
  String page = "<!DOCTYPE html><html><head><title>ESP32 Capteurs</title></head><body>";
  page += "<h1>Données du capteur BME680 :</h1>";
  page += "<ul>";
  page += "<li>Température BME680: " + String(temperature) + " *C</li>";
  page += "<li>Pression BME680: " + String(pressure) + " hPa</li>";
  page += "<li>Humidité BME680: " + String(humidity) + " %</li>";
  page += "<li>Résistance au gaz BME680: " + String(gas_resistance) + " KOhms</li>";
  page += "<li>Altitude BME680: " + String(altitude) + " m</li>";
  page += "</ul>";
  page += "</body></html>";

  // Envoi de la réponse HTML au client
  server.send(200, "text/html", page);
  
  digitalWrite(led, 0);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Initialisation du capteur BME680
  if (!bme.begin()) {
    Serial.println("Impossible de trouver le capteur BME680, vérifiez le câblage !");
    while (1);
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  // Attente de la connexion WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  delay(2);
}
