/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "AD9910.h"
#include "ADS8688.h"
#include "math.h"
#include "tjc_usart_hmi.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADCCHS 3
#define ADCPOINT 512
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t value[10000];
//UART_HandleTypeDef huart2;
extern uchar cfr2[4]; // CFR2瀵�瀛��ㄦ�版��
extern uchar cfr1[4]; // CFR1瀵�瀛��ㄦ�版��
u16 ADS_CH_Value[ADCCHS] = { 0 };
float adc_data[ADCCHS][ADCPOINT];
float adc_fs = 0;
float u1, u2, u3, u4 = 0;
int fH = 0;
float vpp[ADCCHS];
float bate;
char str[100];
int a = 10;
int top = 0;
int freq_index[48] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60, 70,
		80, 90, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1e3, 2e3, 3e3, 4e3,
		5e3, 6e3, 7e3, 8e3, 9e3, 1e4, 2e4, 3e4, 4e4, 5e4, 6e4, 7e4, 8e4, 9e4,
		1e5, 2e5, 3e5 };
float Au[48];
float Rout1 = 0.0;
float Rin1 = 0.0;
int flag = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void adcin() {
	uint16_t i;
	for (i = 0; i < ADCPOINT; i++) {
		uint16_t j;
		Get_AUTO_RST_Mode_ADC_Data(ADCCHS, ADS_CH_Value);

		for (j = 0; j < ADCCHS; j++) {
//			ADS_CH_Value[j] = Get_MAN_Ch_n_Mode_Data();
			if (j == 0 || j == 1)
				adc_data[j][i] = (ADS_CH_Value[j] - 32765) * 10.24 / 65535;
			if (j == 2 || j == 3)
				adc_data[j][i] = (ADS_CH_Value[j] - 32765) * 20.48 / 65535;
//			adc_data[j][i] = ADS_CH_Value[j];
		}

	}
}

void findvpp() {
	float freq_pin = 1000; // 假设这是采样频率，单位是Hz

	uint16_t i, j;
	float vdc[ADCCHS] = { 0 };
	float e[ADCCHS] = { 0 };
	float power[ADCCHS] = { 0 };

	float sum[ADCCHS] = { 0 };

	for (j = 0; j < ADCCHS; j++) {
		sum[j] = 0; // 初始化 sum[j]
		e[j] = 0;   // 初始化 e[j]

		// 计算平均值 vdc[j]
		for (i = 0; i < ADCPOINT; i++) {
			sum[j] += adc_data[j][i];
		}
		vdc[j] = sum[j] / ADCPOINT;

		// 计算能量 e[j]（均方值）
		for (i = 0; i < ADCPOINT; i++) {
			e[j] += (adc_data[j][i] - vdc[j]) * (adc_data[j][i] - vdc[j]);
		}
		e[j] = e[j] / ADCPOINT;

		// 计算功率 power[j]（能量除以时间）
		// 时间周期 T = 1 / freq_pin
		power[j] = e[j] * freq_pin;

		// 计算峰峰值 vpp[j]
		// 假设信号是正弦波，峰峰值与有效值的关系为：Vpp = 2 * sqrt(2) * Vrms
		vpp[j] = 2 * sqrtf(2 * e[j]);
	}
}		//可用优化

float Rout() {

	Freq_convert(1000);   // 璁剧疆棰���涓�1kHz锛���浣�Hz
	Write_Amplitude(46); // 璁剧疆骞�搴��硷�1-800mV���村��锛�锛�杩���璁剧疆涓�200瀵瑰���骞�搴�
	delay_ms(100);
	adcin();
	findvpp();
	u4 = vpp[2];
	HAL_GPIO_WritePin(GPIO_RELAY_GPIO_Port, GPIO_RELAY_Pin, GPIO_PIN_SET);
	delay_ms(100);
	adcin();
	findvpp();
	u3 = vpp[2];
	HAL_GPIO_WritePin(GPIO_RELAY_GPIO_Port, GPIO_RELAY_Pin, GPIO_PIN_RESET);
	delay_ms(100);
	int RL = 2000;
	return (u3 - u4) * RL / u4;
}

float Rin() {
	Freq_convert(1000);   // 璁剧疆棰���涓�1kHz锛���浣�Hz
	Write_Amplitude(100);   // 璁剧疆骞�搴��硷�1-800mV���村��锛�锛�杩���璁剧疆涓�200瀵瑰���骞�搴�
	delay_ms(100);
	adcin();
	findvpp();
	u1 = vpp[0];
	u2 = vpp[1];
	int RP = 2000;
	return u1 * RP / (u2 - u1);
}
void FAu() {
	HAL_GPIO_WritePin(GPIO_RELAY_GPIO_Port, GPIO_RELAY_Pin, GPIO_PIN_SET);
	delay_ms(100);
	float max = 0;
	top = 0;
	for (ulong i = 1; i <= 300e3;) {
		Freq_convert(i);   // 璁剧疆棰���涓�1kHz锛���浣�Hz
		Write_Amplitude(46);
		delay_ms(100);
		adcin();
		findvpp();
		Au[top] = vpp[2] * 1000 / 50;
		if (Au[top] > max)
			max = Au[top];
		if (i >= 100e3)
			i += 100e3;
		else if (i >= 10e3)
			i += 10e3;
		else if (i >= 1e3)
			i += 1e3;
		else if (i >= 1e2)
			i += 1e2;
		else if (i >= 10)
			i += 10;
		else
			i++;
		top++;

	}

	int freq_min;
	int freq_max;
	for (int i = 0; i < top - 1; i++) {
		if ((Au[i] / max - 0.707) * (Au[i + 1] / max - 0.707) < 0) {
			freq_min = freq_index[i];
			freq_max = freq_index[i + 1];
		}
	}
	Freq_convert(freq_min);   // 璁剧疆棰���涓�1kHz锛���浣�Hz
	Write_Amplitude(46);
	delay_ms(100);
	adcin();
	findvpp();
	int prev = vpp[2] * 1000 / 50;
	for (int i = freq_min + 1; i < freq_max; i += 1e2) {
		Freq_convert(i);   // 璁剧疆棰���涓�1kHz锛���浣�Hz
		Write_Amplitude(46);
		delay_ms(100);
		adcin();
		findvpp();
		int tmp = vpp[2] * 1000 / 50;
		if ((prev / max - 0.707) * (tmp / max - 0.707) < 0)
			fH = i - 1;
		prev = tmp;
	}
	HAL_GPIO_WritePin(GPIO_RELAY_GPIO_Port, GPIO_RELAY_Pin, GPIO_PIN_RESET);
	delay_ms(100);
}
void linearInterpolation(float *amp, int originalSize, float *ampInterpolated,
		int newSize) {
	int i, j;
	float ratio;

	for (i = 0; i < newSize; i++) {
		// 计算当前插值点在原始数据中的位置
		float position = (float) i / (newSize - 1) * (originalSize - 1);
		j = (int) position;    // 当前插值点所在的区间起始点
		ratio = position - j; // 插值点在区间内的比例位置

		// 线性插值公式
		ampInterpolated[i] = amp[j] + ratio * (amp[j + 1] - amp[j]);
	}
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	MX_SPI2_Init();
	MX_SPI1_Init();
	/* USER CODE BEGIN 2 */

	HAL_Delay(72);	//涓��靛欢�剁��寰�绋冲��
	Init_ad9910();	//��濮���AD9910����
	HAL_Delay(72);
//��缃��镐�棰���妫�娴�妯″�锛�PFD锛����板����瀹���璺�锛�DLL锛�
//	AD9910_DRG_FreInit_AutoSet(ENABLE);		// ENABLE:����DLL妯″�锛�DISABLE:����DLL妯″�
//娉ㄩ��锛�����DLL妯″�涓�锛�AD9910浼����ㄨ��寸�镐�棰���妫�娴���锛������ㄦā寮���瑕���杩�DRCTL瀵�瀛��ㄨ�琛��у��
//娉ㄩ��锛�DLL锛��板����瀹���璺�锛��ㄤ�绋冲��棰�������锛�纭�淇�杈��洪�����绋冲����
//娉ㄩ��锛�PFD锛��镐�棰���妫�娴���锛��ㄤ�妫�娴�杈��ュ����棰���涓����ㄩ�����宸�寮�
//	AD9910_DRG_FrePara_Set(100000, 100000000, 100, 100, 100,100);						// ��缃�DLL����
//AD9910_DRG_FrePara_Set(400000, 300000000, 1200000, 2200000, 100,300);		// ��缃�DLL���拌����
//娉ㄩ��锛�DLL���拌�剧疆浼�褰卞��棰�����瀹�����搴���绋冲����
//娉ㄩ��锛�������DLL���板��浠ュ��蹇�棰�����瀹�杩�绋�骞舵��楂�棰���绋冲����

//RAM娉㈠舰�����ㄦā寮���缃�
//AD9910_RAM_WAVE_Set(SQUARE_WAVE);	// ��缃�RAM娉㈠舰�����ㄦā寮�
//娉ㄩ��锛�SQUARE_WAVE琛ㄧず�规尝锛�TRIG_WAVE琛ㄧず瑙���娉㈠舰锛�SQUARE_WAVE琛ㄧず�规尝锛�SINC_WAVE琛ㄧずSINC娉㈠舰

////�烘��棰�����骞�搴�璁剧疆
//	for (int i = 40; i < 55;) {
//		Freq_convert(1000);   // 璁剧疆棰���涓�1kHz锛���浣�Hz
//		Write_Amplitude(i); // 璁剧疆骞�搴��硷�1-800mV���村��锛�锛�杩���璁剧疆涓�200瀵瑰���骞�搴�
//		i += 1;
//	}
	ADS8688_Init();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */

	while (1) {
		FAu();
		Rin1 = Rin();
		Rout1 = Rout();
		bate = u3 * 1000 / 50;
		sprintf(str, "x0.val=%d", (int) (Rout1 * 100));
		tjc_send_string(str);
		sprintf(str, "x1.val=%d", (int) (Rin1 * 100));
		tjc_send_string(str);
		sprintf(str, "x2.val=%d", (int) (bate * 100));
		tjc_send_string(str);
		sprintf(str, "n0.val=%d", fH);
		tjc_send_string(str);

		if (flag == 0) {
			float Au1[256];
			linearInterpolation(Au, 48, Au1, 256);
			for (int i = 0; i < 256; i++) {
				// 向曲线s0的通道0传输1�?数据,add指令不支持跨页面
				uint16_t Amp = Au1[i];
				sprintf(str, "add s0.id,0,%d\xff\xff\xff", (int) Amp * 2 - 10);
				tjc_send_string(str);
			}
			flag++;
		}

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
