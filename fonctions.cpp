#include "fonctions.h"
#include <Adafruit_BME680.h>
#include <Adafruit_BMP3XX.h>


void calculerMoyennes(Adafruit_BME680& bme, Adafruit_BMP3XX& bmp) {
  sommeTemperatureBME += bme.temperature;
  sommeHumiditeBME += bme.humidity;
  sommePressionBME += bme.pressure;
  sommeTemperatureBMP += bmp.temperature;
  sommePressionBMP += bmp.pressure;
  nombreLectures++;
}

// Les autres fonctions restent inchangées

float obtenirTemperatureMoyenne(float sommeTemperatureBME, float sommeTemperatureBMP, int nombreLectures) {
  return (sommeTemperatureBME + sommeTemperatureBMP) / (2 * nombreLectures);
}

float obtenirHumiditeMoyenne(float sommeHumiditeBME, int nombreLectures) {
  return (sommeHumiditeBME) / nombreLectures;
}

float obtenirPressionMoyenne(float sommePressionBME, float sommePressionBMP, int nombreLectures) {
  return (sommePressionBME + sommePressionBMP) / (2 * nombreLectures);
}

void gestionInterruption() {
  compteImpulsions++;  // Incrémenter le compteur à chaque front montant détecté
}
