///////////////////////////////////////////////////////////////////////////
// Prénom et Nom : Nelen Nathan
// Date : 25|06|2024
// Classe : 6ème année secondaire Technique de qualification en électronique
// Projet : La station météo
// Matériel : ESP32, encodeur, BME680
///////////////////////////////////////////////////////////////////////////

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

const char* ssid = "#LaboD5";
const char* password = "xjzn7y973uur3hkw";

WebServer serveur(80);

// Capteur BME680
Adafruit_BME680 bme;

// Altitude de référence pour le calcul de l'altitude
#define PRESSIONNIVEAUSEALE (1013.25)

const int brocheEntree = 6;                   // Pin où se trouve le signal carré de l'encodeur
volatile unsigned long compteImpulsions = 0;  // Variable pour compter le nombre de fronts montants
const float facteur = 1.0 / 2.0;              // Facteur de conversion pour convertir les pulses en vitesse du vent (à ajuster...)

float sommeTemperatureBME = 0;
float sommeHumiditeBME = 0;
float sommePressionBME = 0;
int nombreLectures = 0;

void gestionInterruption() {
  compteImpulsions++;  // Incrémenter le compteur à chaque front montant détecté
}

// Fonction demandée par monsieur Mazzeo
void calculerMoyennes() {
  sommeTemperatureBME += bme.temperature;
  sommeHumiditeBME += bme.humidity;
  sommePressionBME += bme.pressure;
  nombreLectures++;
}

float obtenirTemperatureMoyenne() {
  return sommeTemperatureBME / nombreLectures;
}

float obtenirHumiditeMoyenne() {
  return sommeHumiditeBME / nombreLectures;
}

float obtenirPressionMoyenne() {
  return sommePressionBME / nombreLectures;
}

//*********************************************************************************************************************
void gererRacine() {
  digitalWrite(LED_BUILTIN, HIGH);

  // Lecture des données des capteurs
  calculerMoyennes();
  float temperatureBME = bme.temperature;
  float pressionBME = bme.pressure / 100.0;  // Conversion en hPa
  float humiditeBME = bme.humidity;

  // Calcul de la vitesse du vent
  detachInterrupt(digitalPinToInterrupt(brocheEntree));                               // Détacher l'interruption pendant le calcul
  float vitesseVent = compteImpulsions * facteur;                                     // Calculer la vitesse du vent en utilisant le facteur de conversion
  compteImpulsions = 0;                                                               // Réinitialiser le compteur de pulses
  attachInterrupt(digitalPinToInterrupt(brocheEntree), gestionInterruption, RISING);  // Réattacher l'interruption

  // Construction de la réponse HTML avec les données des capteurs
  String page = "<!DOCTYPE html><html><head><title>ESP32 Capteurs</title></head><body>";
  page += "<h1>Données des capteurs :</h1>";
  page += "<h2>Capteur BME680 :</h2>";
  page += "<ul>";
  page += "<li>Température BME680: " + String(temperatureBME) + " *C</li>";
  page += "<li>Pression BME680: " + String(pressionBME) + " hPa</li>";
  page += "<li>Humidité BME680: " + String(humiditeBME) + " %</li>";
  page += "</ul>";
  page += "<h2>Moyennes :</h2>";
  page += "<ul>";
  page += "<li>Température moyenne: " + String(obtenirTemperatureMoyenne()) + " *C</li>";
  page += "<li>Pression moyenne: " + String(obtenirPressionMoyenne()) + " hPa</li>";
  page += "<li>Humidité moyenne: " + String(obtenirHumiditeMoyenne()) + " %</li>";
  page += "<li>Vitesse du vent : " + String(vitesseVent) + " m/s</li>";
  page += "</ul>";
  page += "</body></html>";

  // Envoi de la réponse HTML au client
  serveur.send(200, "text/html", page);

  digitalWrite(LED_BUILTIN, LOW);
}

void configuration() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Initialisation du capteur BME680
  if (!bme.begin()) {
    Serial.println("Impossible de trouver le capteur BME680, vérifiez le câblage !");
    while (1)
      ;
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
  Serial.print("Connecté à ");
  Serial.println(ssid);
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());

  // Configuration de l'interruption pour le comptage des pulses de l'anémomètre
  pinMode(brocheEntree, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(brocheEntree), gestionInterruption, RISING);

  serveur.on("/", gererRacine);
  serveur.begin();
  Serial.println("Serveur HTTP démarré");
}

void setup() {
  configuration();
}

void loop() {
  serveur.handleClient();
  delay(2);
}
