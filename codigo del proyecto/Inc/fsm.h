/*
 * fsm.h
 *
 *  Created on: 22 abr. 2021
 *      Author: Elias
 */

#ifndef INC_FSM_H_
#define INC_FSM_H_

/* ----------------------------------------------------------------
 * Entrada para la máquina de Mealy
 ---------------------------------------------------------------- */
/* se crean las variables para sensar el estado de los puertos durante el debug. No es necesario para el código */
int sensorParaguas = 0;
/* función para leer el sensor de luz */
int leerLuz();
/* función para leer el pulsador */
int leerPush();
/* función para leer la variable interna que representa un sensor que capta si el paraguas está desplegado o no */
int leerParaguas();
/* función donde se lee el valor del sensor de agua. Retorna 1 (si supera el umbral definido) o 0 (si no lo supera) */
int leerHumedad();

/* ----------------------------------------------------------------
 * Salida para la máquina de Mealy
 ---------------------------------------------------------------- */
/* función que controla el motor (representado por el led verde) de la aspiradora */
void salidaAspiradora(int a);
/* función que controla el motor (representado por el led rojo) de la paraguas */
void salidaParaguas(int a);
/* funcion que controla el motor (represantado por los led azul-avanza y amarillo-retrocede) de las ruedas */
void salidaRueda(int a);

/* ----------------------------------------------------------------
 * Máquina de Mealy
 ---------------------------------------------------------------- */
void actualizarMEF(void);


#endif /* INC_FSM_H_ */
