#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define SEALEVELPRESSURE_HPA (1013.25)
const char* ssid = "Proximus-Home-923458";
const char* password = "xjzn7y973uur3hkw";

// const char* ssid = "#LaboD5";
// const char* password = "0123456789";

// Déclaration des variables pour la connexion WiFi
WebServer server(80);

// Déclaration des broches pour l'encodeur
int encoder_Pin_1 = 2;
int encoder_Pin_2 = 3;

// Constantes pour le calcul de la vitesse
const float diametreAxe = 0.006;
const float circonferenceAxe = PI * diametreAxe;
const int pulsesParRevol = 600;

// Variables pour la gestion de l'encodeur
volatile int dernierEncodage = 0;
volatile long valeurEncodeur = 0;
long dernierValeurEncodeur = 0;
int dernierMSB = 0;
int dernierLSB = 0;

Adafruit_BME680 bme;

unsigned long debutTemps;
unsigned long tempsEcoule;
unsigned long dernierTemps = 0;
float vitesse = 0.0;

// Déclaration des fonctions
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("Test BME680"));

  // Connexion au WiFi
  Serial.print("Connexion au WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connecté au WiFi");

  // Configuration du serveur
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.begin();
  Serial.println("Serveur démarré");
  Serial.println("Visitez http://" + WiFi.localIP().toString() + " pour voir les données");

  // Configuration des broches pour l'encodeur
  pinMode(encoder_Pin_1, INPUT);
  pinMode(encoder_Pin_2, INPUT);
  digitalWrite(encoder_Pin_1, HIGH);
  digitalWrite(encoder_Pin_2, HIGH);
  attachInterrupt(digitalPinToInterrupt(encoder_Pin_1), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoder_Pin_2), updateEncoder, CHANGE);

  debutTemps = millis();

  // Initialisation du capteur BME680
  if (!bme.begin()) {
    Serial.println("Erreur de connexion au capteur BME680 !");
    while (1);
  }
  // Configuration du capteur BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C pour 150 ms

  // Initialisation de la connexion NTP pour la synchronisation de l'heure
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void loop() {
  server.handleClient();
  calculerVitesse();
}

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<title>ESP32 Station Météo Connectée de Nathan</title>
<meta charset="UTF-8">
<script type="text/javascript">
    function startTimer() {
      setInterval(updateData, 1000); // Mise à jour toutes les secondes
    }

    function updateData() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var data = JSON.parse(this.responseText);
          document.getElementById("temperature").innerHTML = data.temperature + " °C";
          document.getElementById("pressure").innerHTML = data.pressure + " hPa";
          document.getElementById("humidity").innerHTML = data.humidity + " %";
          document.getElementById("iaq_index").innerHTML = data.iaq_index;
          document.getElementById("altitude").innerHTML = data.altitude + " m";
          document.getElementById("encoderValue").innerHTML = data.encoderValue;
          document.getElementById("speed").innerHTML = data.speed + " km/h";
          document.getElementById("currentTime").innerHTML = data.time;
        } else if (this.readyState == 4) {
          console.error("Erreur lors de la récupération des données: " + this.status);
        }
      };
      xhr.open("GET", "/data", true);
      xhr.send();
    }

    window.onload = function() {
      startTimer();
    }
</script>
</head>
<body>
<h1>ESP32 Station Météo Connectée de Nathan</h1>
<h2>Données Actuelles:</h2>
<ul>
  <li>Température: <span id="temperature">0.0 °C</span></li>
  <li>Pression: <span id="pressure">0.0 hPa</span></li>
  <li>Humidité: <span id="humidity">0.0 %</span></li>
  <li>Indice IAQ: <span id="iaq_index">0</span></li>
  <li>Altitude: <span id="altitude">0.0 m</span></li>
  <li>Valeur de l'Encodeur: <span id="encoderValue">0</span></li>
  <li>Vitesse: <span id="speed">0.0 km/h</span></li>
</ul>
<h2>Heure Actuelle (Bruxelles):</h2>
<p id="currentTime">00:00:00</p>
</body>
</html>)rawliteral");
}

void handleData() {
  tempsEcoule = millis();

  if (!bme.performReading()) {
    Serial.println("Erreur lors de la lecture des données du capteur BME680 !");
    return;
  }

  float temperature = bme.temperature;
  float pressure = bme.pressure / 100.0;  // Convertir en hPa
  float humidity = bme.humidity;
  String indiceIAQ = obtenirLabelIAQ(calculerIAQ());
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  // Requête HTTP à l'API worldtimeapi.org pour obtenir l'heure actuelle
  HTTPClient http;
  http.begin("https://worldtimeapi.org/api/timezone/Europe/Brussels");
  int httpCode = http.GET();
  String currentTime = "00:00:00";
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    int timeIndex = payload.indexOf("\"datetime\":\"") + 12;
    currentTime = payload.substring(timeIndex, timeIndex + 19);
    currentTime.replace("T", " ");
  }
  http.end();

  String jsonData = "{\"temperature\":" + String(temperature) +
                    ",\"pressure\":" + String(pressure) +
                    ",\"humidity\":" + String(humidity) +
                    ",\"iaq_index\":\"" + indiceIAQ + "\"" +
                    ",\"altitude\":" + String(altitude) +
                    ",\"encoderValue\":" + String(valeurEncodeur) +
                    ",\"speed\":" + String(vitesse) +
                    ",\"time\":\"" + currentTime + "\"}";

  server.send(200, "application/json", jsonData);
}

String obtenirLabelIAQ(float indiceIAQ) {
  if (indiceIAQ <= 50) {
    return "Excellent";
  } else if (indiceIAQ <= 100) {
    return "Bon";
  } else if (indiceIAQ <= 150) {
    return "Légèrement pollué";
  } else if (indiceIAQ <= 200) {
    return "Modérément pollué";
  } else if (indiceIAQ <= 300) {
    return "Fortement pollué";
  } else if (indiceIAQ <= 400) {
    return "Très pollué";
  } else {
    return "Extrêmement pollué";
  }
}

float calculerIAQ() {
  float resistanceGaz = bme.gas_resistance / 1000.0;
  float humidity = bme.humidity;

  float a = 0.0;
  float b = 0.0;

  if (resistanceGaz > 0) {
    a = log(resistanceGaz);
  }

  if (humidity > 0) {
    b = log(humidity);
  }

  return a + b;
}

void updateEncoder() {
  int MSB = digitalRead(encoder_Pin_1);  // MSB = most significant bit
  int LSB = digitalRead(encoder_Pin_2);  // LSB = least significant bit

  int encoded = (MSB << 1) | LSB;              // Concaténer les deux bits
  int sum = (dernierEncodage << 2) | encoded;  // Concaténer avec le dernier état

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) valeurEncodeur++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) valeurEncodeur--;

  dernierEncodage = encoded;  // Stocker cet état pour la prochaine fois
}

void calculerVitesse() {
  unsigned long currentTime = millis();
  unsigned long timeDifference = currentTime - dernierTemps;

  if (timeDifference >= 1000) {
    float revolutions = (valeurEncodeur - dernierValeurEncodeur) / (float)pulsesParRevol;
    float distance = revolutions * circonferenceAxe;
    float speedMPerSec = distance / (timeDifference / 1000.0);
    vitesse = speedMPerSec * 3.6;  // Convertir m/s en km/h

    dernierValeurEncodeur = valeurEncodeur;
    dernierTemps = currentTime;
  }
}
