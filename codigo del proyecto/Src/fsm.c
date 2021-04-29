/*
 * fsm.c
 *
 *  Created on: 22 abr. 2021
 *      Author: Elias
 */


#include "main.h"

/* ----------------------------------------------------------------
 * Entrada para la máquina de Mealy
 ---------------------------------------------------------------- */
/* se crean las variables para sensar el estado de los puertos durante el debug. No es necesario para el código */
int sensorParaguas = 0;
/* función para leer el sensor de luz */
int leerLuz() {
	if (HAL_GPIO_ReadPin(GPIOA, INPUT_LUZ) == GPIO_PIN_SET) {
		return 1;
	} else {
		return 0;
	}
}
/* función para leer el pulsador */
int leerPush() {
	if (HAL_GPIO_ReadPin(GPIOA, INPUT_PUSH) == GPIO_PIN_SET) {
		return 1;
	} else {
		return 0;
	}
}
/* función para leer la variable interna que representa un sensor que capta si el paraguas está desplegado o no */
int leerParaguas() {
	return sensorParaguas;
}
/* función donde se lee el valor del sensor de agua. Retorna 1 (si supera el umbral definido) o 0 (si no lo supera) */
int leerHumedad() {
	/* variable de dominio local. Se utiliza para simplificar el código */
	int var_temporal;
	/* se inicializa el conversor analogico digital */
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	/* se pone el valor a la variable creada */
	var_temporal = HAL_ADC_GetValue(&hadc1);
	/* se utiliza esta función para concatenar el texto definido con el valor de la variable. Útil durante el debug. Se castea la variable "dataT" ya que la función "snprinf" acepta datos tipo "char" pero no "uint_8" */
	snprintf((char*) dataT, m, "el valor del sensor de humedad es: %d\n\r",
			var_temporal);
	/* ARM envia a traves de su interfaz UART el array concatenado */
	HAL_UART_Transmit(&huart2, dataT, m, HAL_MAX_DELAY);
	/* si el valor de var_temporal supera los 2000, la función retorna 1, caso contrario retorna 0.
	 * 3200 = valor máximo
	 * 200 = valor mínimo
	 *  */
	if (var_temporal > 2000) {
		return 1;
	} else {
		return 0;
	}
}
/* ----------------------------------------------------------------
 * Salida para la máquina de Mealy
 ---------------------------------------------------------------- */
/* función que controla el motor (representado por el led verde) de la aspiradora */
void salidaAspiradora(int a) {
	HAL_GPIO_WritePin(GPIOA, OUTPUT_ASPIRADORA, a);
}
/* función que controla el motor (representado por el led rojo) de la paraguas */
void salidaParaguas(int a) {
	if (a == 0) {
		sensorParaguas = 1;
		HAL_GPIO_WritePin(GPIOA, OUTPUT_PARAGUAS, GPIO_PIN_SET);
	} else {
		sensorParaguas = 0;
		HAL_GPIO_WritePin(GPIOA, OUTPUT_PARAGUAS, GPIO_PIN_RESET);
	}
}
/* funcion que controla el motor (represantado por los led azul-avanza y amarillo-retrocede) de las ruedas */
void salidaRueda(int a) {
	// 0 = apagado
	// 1 = avanza
	// 2 = retrocede
	if (a == 0) {
		HAL_GPIO_WritePin(GPIOA, OUTPUT_AVANZA, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, OUTPUT_RETROCEDE, GPIO_PIN_RESET);
	} else if (a == 1) {
		HAL_GPIO_WritePin(GPIOA, OUTPUT_AVANZA, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, OUTPUT_RETROCEDE, GPIO_PIN_RESET);
	} else if (a == 2) {
		HAL_GPIO_WritePin(GPIOA, OUTPUT_AVANZA, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, OUTPUT_RETROCEDE, GPIO_PIN_SET);
	}
}
/* ----------------------------------------------------------------
 * Máquina de Mealy
 ---------------------------------------------------------------- */

void actualizarMEF(void) {
	switch (estadoActual) {
	case ESTADO_INICIAL: {
		estadoActual = LIMPIAR;
	}
		break;
	case DORMIR: {
		/* salida del estado */
		estadoMemoria = DORMIR;
		salidaAspiradora(0);
		salidaRueda(0); // dormir
		/* toma de decisión para pasar al siguiente estado. Ojo: Solo es un cambio de estado. No se cambia ninguna variable */
		if ((leerHumedad() == 0 && leerParaguas() == 0 && leerLuz() == 1
				&& leerPush() == 1)
				|| (leerHumedad() == 1 && leerParaguas() == 1 && leerLuz() == 1
						&& leerPush() == 1)) {
			estadoActual = DORMIR;
		} else if ((leerHumedad() == 0 && leerParaguas() == 0 && leerLuz() == 0)
				|| (leerHumedad() == 1 && leerParaguas() == 1 && leerLuz() == 0)) {
			estadoActual = LIMPIAR;
		} else if ((leerHumedad() == 1 && leerParaguas() == 0)
				|| (leerHumedad() == 0 && leerParaguas() == 1)) {
			estadoActual = PARAGUAS;
		} else if ((leerHumedad() == 0 && leerParaguas() == 0 && leerLuz() == 1
				&& leerPush() == 0)
				|| (leerHumedad() == 1 && leerParaguas() == 1 && leerLuz() == 1
						&& leerPush() == 0)) {
			estadoActual = ESCONDERSE;
		}
	}
		break;
	case LIMPIAR: {
		/* salida del estado */
		salidaRueda(1); // avanza
		salidaAspiradora(1); // prendido
		estadoMemoria = LIMPIAR;
		/* toma de decisión para pasar al siguiente estado. Ojo: Solo es un cambio de estado. No se cambia ninguna variable */
		if ((leerHumedad() == 0 && leerLuz() == 0)
				|| (leerHumedad() == 1 && leerParaguas() == 1 && leerLuz() == 0)) {
			estadoActual = LIMPIAR;
		} else if (leerHumedad() == 1 && leerParaguas() == 0) {
			estadoActual = PARAGUAS;
		} else if (leerHumedad() == 1 && leerParaguas() == 1
				&& leerLuz() == 0) {
			estadoActual = LIMPIAR;
		} else if ((leerHumedad() == 0 && leerLuz() == 1)
				|| (leerHumedad() == 1 && leerParaguas() == 1 && leerLuz() == 1)) {
			estadoActual = ESCONDERSE;
		}
	}
		break;
	case PARAGUAS: {
		/* salida del estado */
		salidaParaguas(leerParaguas());
		/* toma de decisión para pasar al siguiente estado. Ojo: Solo es un cambio de estado. No se cambia ninguna variable */
		if (estadoMemoria == LIMPIAR) {
			estadoActual = LIMPIAR;
		} else if (estadoMemoria == ESCONDERSE) {
			estadoActual = ESCONDERSE;
		} else if (estadoMemoria == DORMIR) {
			estadoActual = DORMIR;
		}
	}
		break;
	case ESCONDERSE: {
		/* salida del estado */
		salidaAspiradora(0);
		salidaRueda(2); // retrocede
		estadoMemoria = ESCONDERSE;
		/* toma de decisión para pasar al siguiente estado. Ojo: Solo es un cambio de estado. No se cambia ninguna variable */
		if ((leerHumedad() == 0 && leerParaguas() == 0 && leerLuz() == 1
				&& leerPush() == 1)
				|| (leerHumedad() == 1 && leerParaguas() == 1 && leerLuz() == 1
						&& leerPush() == 1)) {
			estadoActual = DORMIR;
		} else if ((leerHumedad() == 0 && leerParaguas() == 0 && leerLuz() == 0)
				|| (leerHumedad() == 1 && leerParaguas() == 1 && leerLuz() == 0)) {
			estadoActual = LIMPIAR;
		} else if ((leerHumedad() == 1 && leerParaguas() == 0)
				|| (leerHumedad() == 0 && leerParaguas() == 1)) {
			estadoActual = PARAGUAS;
		} else if ((leerHumedad() == 0 && leerParaguas() == 0 && leerLuz() == 1
				&& leerPush() == 0)
				|| (leerHumedad() == 1 && leerParaguas() == 1 && leerLuz() == 1
						&& leerPush() == 0)) {
			estadoActual = ESCONDERSE;
		}
	}
		break;
	default: {
		/* toma de decisión para pasar al siguiente estado. Ojo: Solo es un cambio de estado. No se cambia ninguna variable */
		estadoActual = ESTADO_INICIAL;
	}
		break;
	}
}
