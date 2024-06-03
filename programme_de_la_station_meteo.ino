#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_BME680.h>

#define PLUIE A2
#define DIRECTION_VENT A1
#define VITESSE_VENT A0

float niveauPluie = 0;
float ventVitesse = 0;
int ventDirection = 0;
long pulseCount = 0;

const char* ssid = "#LaboD5";
const char* password = "0123456789";
const char* hostname = "Station_meteo Nathan";

WebServer server(80);

Adafruit_BME680 bme;  // Crée une instance du capteur BME680

void handleRoot() {
  pluie();
  MesureDirectionVent();
  MesureVitesseVent();
  String content = "<html><head><title>Weather Station</title>";
  content += "<meta http-equiv=\"refresh\" content=\"1\">";  //Rafraichissement de la page web
  content += "</head><body><h1>Weather Station</h1>";
  // Lire les données du capteur BME680
  if (bme.performReading()) {
    content += "<p>Temperature : " + String(bme.temperature) + " °C</p>";
    content += "<p>Pression : " + String(bme.pressure / 100.0) + " hPa</p>";
    content += "<p>Humidite : " + String(bme.humidity) + " %</p>";
    content += "<p>Gaz resistance : " + String(bme.gas_resistance / 1000.0) + " KOhms</p>";
  } else {
    content += "<p>Impossible de lire les donnees du capteur BME680.</p>";
  }
  content += "<p>Niveau de Pluie : " + String(niveauPluie) + " mm</p>";
  content += "<p>Vitesse du vent : " + String(ventVitesse) + " Km/h</p>";
  content += "<p>Direction du vent : " + String(vraiDirection) + "</p>";
  content += "</body></html>";
  server.send(200, "text/html", content);
}

void pluie() {
  if (!digitalRead(PLUIE)) {
    niveauPluie += 0.2794;
  }
}
String vraiDirection;
void MesureDirectionVent() {
  
  ventDirection = analogRead(DIRECTION_VENT);
  if ((ventDirection >= 0) && (ventDirection <= 255)) {
    vraiDirection = "S";
  }
  else if ((ventDirection >= 256) && (ventDirection <= 511)) {
    vraiDirection = "O";
  }
  else if ((ventDirection >= 512) && (ventDirection <= 767)) {
    vraiDirection = "N";
  }
  else if ((ventDirection >= 768) && (ventDirection <= 1023)) {
    vraiDirection = "E";
  }
}

void handleInterrupt() {
  pulseCount++;  // Incrémenter le compteur à chaque front montant détecté
}

void MesureVitesseVent() {

  static unsigned long lastTime = 0;   // Variable pour stocker le dernier moment où nous avons mesuré la fréquence
  static unsigned long lastCount = 0;  // Variable pour stocker le dernier compteur de pulses

  unsigned long currentTime = millis();                // Obtenir le temps actuel
  unsigned long elapsedTime = currentTime - lastTime;  // Calculer le temps écoulé depuis la dernière mesure

  if (elapsedTime >= 1000) {                                // Si 1 seconde s'est écoulée
    detachInterrupt(digitalPinToInterrupt(VITESSE_VENT));       // Détacher l'interruption pendant le calcul
    unsigned long pulsePerSecond = pulseCount - lastCount;  // Calculer le nombre de pulses par seconde
    ventVitesse = pulsePerSecond / 384, 62;

    lastCount = pulseCount;  // Mettre à jour le dernier compteur de pulses
    lastTime = currentTime;  // Mettre à jour le dernier moment de mesure

    attachInterrupt(digitalPinToInterrupt(VITESSE_VENT), handleInterrupt, RISING);  // Réattacher l'interruption
  }
}

void setup() {
  Serial.begin(115200);  // Initialisation du port série à 115200 baud

  pinMode(PLUIE, INPUT_PULLUP);  // On définit les capteurs de pluie et vent comme entrée
  pinMode(VITESSE_VENT, INPUT_PULLUP);

  bme.setTemperatureOversampling(BME680_OS_8X);  // On definit la fréquence d'échantillonage
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);  // On definit la résolution du filtre RII
  bme.setGasHeater(320, 150);                  // On chauffe la plaque gas sensor à 320 °C pendant 150ms

  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  // Ajouter un court délai pour ne pas surcharger le serveur web
  delay(100);
}