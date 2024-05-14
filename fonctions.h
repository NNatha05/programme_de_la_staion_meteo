#ifndef FONCTIONS_H
#define FONCTIONS_H

#include <Arduino.h> // Inclure cette bibliothèque si nécessaire

// Déclarations des prototypes de fonctions
void calculerMoyennes();
float obtenirTemperatureMoyenne();
float obtenirHumiditeMoyenne();
float obtenirPressionMoyenne();
void gestionInterruption();

// Déclarations des variables globales utilisées dans les fonctions
extern float sommeTemperatureBME;
extern float sommeHumiditeBME;
extern float sommePressionBME;
extern float sommeTemperatureBMP;
extern float sommePressionBMP;
extern int nombreLectures;
extern volatile unsigned long compteImpulsions;

#endif // FONCTIONS_H
