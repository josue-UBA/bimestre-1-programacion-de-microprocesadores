/* librerías definidas para el funcionamiento del proyecto */
#include <stdio.h>
#include "main.h"

/* definiciones para no tener problema a la hora de cambiar de puerto de la tarjeta */
# define INPUT_LUZ			GPIO_PIN_1
# define INPUT_PUSH			GPIO_PIN_9

# define OUTPUT_ASPIRADORA	GPIO_PIN_0
# define OUTPUT_PARAGUAS	GPIO_PIN_4
# define OUTPUT_AVANZA		GPIO_PIN_5
# define OUTPUT_RETROCEDE	GPIO_PIN_6

ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart2;

/* ----------------------------------------------------------------
 * otras variables
 ---------------------------------------------------------------- */
/* array donde se almacena los datos y se envía por la UART */
uint8_t dataT[50] = "";
/* se ejecuta cálculo para tener el tamaño del array */
int m = sizeof(dataT) / sizeof(dataT[0]);

/* ----------------------------------------------------------------
 * Estados
 ---------------------------------------------------------------- */
/* estadoActual es el estado real de la máquina de estado. estadoMemoria es el estado anterior. Se utiliza a la hora de tomar decisiones en el estado "PARAGUAS" */
typedef enum {
	ESTADO_INICIAL, DORMIR, LIMPIAR, PARAGUAS, ESCONDERSE
} estado_MEF_t;
estado_MEF_t estadoActual, estadoMemoria;

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
/* declaraciones de funciones implementadas líneas abajo */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
/* ----------------------------------------------------------------
 * main
 ---------------------------------------------------------------- */
int main(void) {
	HAL_Init();
	SystemClock_Config();
	/* inicializa puertos logicos */
	MX_GPIO_Init();
	/* inicializa puerto ADC */
	MX_ADC1_Init();
	/* inicializa UART */
	MX_USART2_UART_Init();
	/* bucle infinito */
	while (1) {
		actualizarMEF();
		HAL_Delay(50);
	}
}
/* ----------------------------------------------------------------
 * funciones definidas por el STMCube
 ---------------------------------------------------------------- */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

static void MX_ADC1_Init(void) {
	ADC_ChannelConfTypeDef sConfig = { 0 };
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		Error_Handler();
	}
	sConfig.Channel = ADC_CHANNEL_7;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}
}
static void MX_USART2_UART_Init(void) {
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
}

static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6,
			GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : PA0 PA4 PA5 PA6 */
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

void Error_Handler(void) {
	__disable_irq();
	while (1) {
	}
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif



