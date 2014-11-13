/*
 * Copyright (C) 2014 Drinkuino
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//Proyecto ya funcionando con las electroválvulas orientado al uso de Bluetooth para servir las mezclas (con retroalmentación por parte de Arduino)
//Los líquidos se expulsan secuencialmente
#include <Ultrasonic.h>
#include <MeetAndroid.h>

/*
 HC-SR04 sensor de distancia:
 VCC a Arduino 5V
 GND a Arduino GND
 Echo a Arduino pin 7 
 Trig a Arduino pin 8
 */
 /*
 JY-MCU módulo bluetooth:
 VCC a Arduino 5V GND
 GND a Arduino GND
 TX a Arduino RX (pin digital 0 de Arduino)
 RX a Arduino TX (pin digital 1 de Arduino)
 */
 /*
 Piezoelectrico
 Cable negro a Arduino GND
 Cable rojo a Arduino pin 2
 */
 
//Constantes que indican el rango de distancias entre los que se puede situar el objeto (en centímetros)
#define DISTANCIA_MAX 2
#define DISTANCIA_MIN 1
//Constante que define el caudal (relación volumen/tiempo) de las electroválvulas (medido en ml/s)
#define CAUDAL 100
//Constante que define el número de válvulas en el circuito (una por cada líquido)
#define N_LIQUIDOS 4
//Constante que definen las frecuencias de las notas musicales
#define DO 261.626
#define MI 299.628
#define SOL 391.995

Ultrasonic ultrasonic(8,7);  //Sensor de proximidad. Trig PIN = 8. Echo PIN = 7
MeetAndroid meetAndroid; //Declaración para utilizar funciones de la librería meetAndroid (comunicación Arduino-Android)
#define Valve1Pin 9  //Pin del LED de la válvula 1
#define Valve2Pin 10  //Pin del LED de la válvula 2
#define Valve3Pin 11  //Pin del LED de la válvula 3
#define Valve4Pin 12  //Pin del LED de la válvula 4
#define LEDEnRangoPin 13 //Pin del LED que se encenderá si hay un objeto a la distancia adecuada
#define PiezoelectricoPin 2 //Pin del piezoeléctrico que emitirá sonido cuando una mezcla sea servida


boolean enRango; //Variable global que indica si el objeto está situado en el rango permitido
int mezcla[N_LIQUIDOS];  //Array global que almacena la mezcla que se va a servir, cada mezcla se compone de una cantidad (medida en cl) de cada líquido. Dato en la posición 0 es la cantidad del primer líquido, etc.
int valvulas[N_LIQUIDOS] = {Valve1Pin, Valve2Pin, Valve3Pin, Valve4Pin}; //Array global que almacena los pines de las válvulas en orden. Posición 0 = primera válvula, etc

//Devuelve true si hay colocado un objeto entre la menor y la mayor distancia hasta el sensor
boolean estaEnRango(int distancia_min, int distancia_max) {
  long distancia_medida;
  distancia_medida = ultrasonic.Ranging(CM); //Calcula la distancia en cm (libreria Ultrasonic)
  
  return (distancia_medida >= distancia_min && distancia_medida <= distancia_max);
}

//Abre la electroválvula indicada durante el tiempo justo para expulsar una determinada cantidad (en mililitros) de líquido
void expulsarCantidad(int valvulaPin, int cantidad) {
  if (cantidad > 0) { //La cantidad debe ser mayor que 0, de lo contrario no se hace nada
    unsigned long milisegundos;
    float segundos;
    segundos = (float) cantidad / CAUDAL; //Tiempo en segundos que permanecerá abierta la válvula almacenado en coma flotante para no perder precisión
    milisegundos =  (unsigned long) (segundos * 1000); //Conversión a milisegundos (con casting a long)
    digitalWrite(valvulaPin, HIGH); //Se abre la válvula
    //Se abre la válvula
    delay(milisegundos); //Se espera el tiempo indicado
    //Se cierra la valvula
    digitalWrite(valvulaPin, LOW); //Se cierra la válvula
  }
}

//Similar a la función anterior pero estableciendo la mezcla a partir de un array
void establecerMezclaArray(int cantidades[]) {
  int i;
  for(i = 0; i < N_LIQUIDOS; i++) {
    mezcla[i] = cantidades[i];
  }
}

//Expulsa la mezcla establecida
void expulsarMezcla() {
  int i;
  for (i = 0; i < N_LIQUIDOS; i++) { //Recorre las válvulas expulsando la cantidad correspondiente con cada una
    expulsarCantidad(valvulas[i], mezcla[i]);
  }
}

//El piezolectrico emite un sonido
void emitirSonido(boolean ascendente) {
  float notas[] = {DO, MI, SOL};
  int i;
  int inicio;
  int fin;
  if (ascendente) {
    for(i = 0; i < 3; i++) {
        tone(PiezoelectricoPin, round(notas[i]));
        delay(100);
    }
  }
  else {
    for(i = 2; i > -1; i--) {
        tone(PiezoelectricoPin, round(notas[i]));
        delay(100);
    }
  }
  noTone(PiezoelectricoPin); //Se detiene el sonido 
}

//Se ejecutará cuando desde Android se decida servir una determinada mezcla
void servirMezcla(byte flag, byte numOfValues) {
  int cantidades[N_LIQUIDOS];
  if(enRango) { //Si en efecto hay un objeto a una distancia adecuada
    meetAndroid.getIntValues(cantidades); //Recibe la mezcla desde Android
    establecerMezclaArray(cantidades); //Establece la mezcla que se va a servir
    emitirSonido(true); //El piezoelectrico emite un sonido indicando que la mezcla ha empezado a servirse
    expulsarMezcla(); //Sirve la mezcla
    meetAndroid.send('k'); //Arduino envía a la app Android el carácter 'k' (ok) para indicar que terminó de servir la mezcla
    emitirSonido(false); //El piezoelectrico emite un sonido indicando que la mezcla ha sido servida
  }
  else {
    meetAndroid.send('n'); //Arduino envía a la app Android el carácter 'n' (no) para indicar que no hay un vaso a la distancia adecuada
  }
}

void setup()
{
    Serial.begin(9600); //"Baud rate" (tasa de baudios" del módulo bluetooth, para el JY-MCU es de 9600 
    pinMode(Valve1Pin, OUTPUT);
    pinMode(Valve2Pin, OUTPUT);
    pinMode(Valve3Pin, OUTPUT);
    pinMode(Valve4Pin, OUTPUT);
    pinMode(LEDEnRangoPin, OUTPUT);
    pinMode(PiezoelectricoPin,OUTPUT);
    //Registro de la función que será llamada cuando el evento asociado en la app Android ocurra 
    meetAndroid.registerFunction(servirMezcla, 's');
}

void loop()
{
  enRango = estaEnRango(DISTANCIA_MIN, DISTANCIA_MAX);
  if(enRango) { //Si hay un objeto a una distancia adecuada
    digitalWrite(LEDEnRangoPin, HIGH); //Enciende el LED que indica si hay un objeto a la distancia adecuada
  }
  else {
    digitalWrite(LEDEnRangoPin, LOW); //Apaga el LED que indica si hay un objeto a la distancia adecuada
  }
  meetAndroid.receive(); //Para recibir eventos desde Android

  delay(50); //Esperar 50ms antes de la siguiente lectura del sensor de proximidad
}
