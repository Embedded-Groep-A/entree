/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/*!
 * \file Lichtkrant_Main.c
 * \brief Lichtkrant code
 */


#include "CHARS.h"

#include <assert.h>

#include "sht3x.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

typedef enum
{
	SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH = 0x2c06,
	SHT3X_COMMAND_CLEAR_STATUS = 0x3041,
	SHT3X_COMMAND_SOFT_RESET = 0x30A2,
	SHT3X_COMMAND_HEATER_ENABLE = 0x306d,
	SHT3X_COMMAND_HEATER_DISABLE = 0x3066,
	SHT3X_COMMAND_READ_STATUS = 0xf32d,
	SHT3X_COMMAND_FETCH_DATA = 0xe000,
	SHT3X_COMMAND_MEASURE_HIGHREP_10HZ = 0x2737,
	SHT3X_COMMAND_MEASURE_LOWREP_10HZ = 0x272a
} sht3x_command_t;


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c3;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

int maxDevices = 4;
unsigned char spidata[16];

char InteruptData[2];

uint8_t status[64];

sht3x_handle_t TempVochtSensor;//set vocht/temp sensor struct op

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C3_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

///wordt gebruikt om de temperatuur/vocht data op bitfouten te controleren 
static uint8_t calculate_crc(const uint8_t *data, size_t length)
{
	uint8_t crc = 0xff;
	for (size_t i = 0; i < length; i++) {
		crc ^= data[i];
		for (size_t j = 0; j < 8; j++) {
			if ((crc & 0x80u) != 0) {
				crc = (uint8_t)((uint8_t)(crc << 1u) ^ 0x31u);
			} else {
				crc <<= 1u;
			}
		}
	}
	return crc;
}
///stuur commando's naar de Temp sensor om uit te lezen of op te starten
static int sht3x_send_command(sht3x_handle_t *handle, sht3x_command_t command)
{
	uint8_t command_buffer[2] = {(command & 0xff00u) >> 8u, command & 0xffu};

	if (HAL_I2C_Master_Transmit(handle->i2c_handle, handle->device_address << 1u, command_buffer, sizeof(command_buffer),
	                            SHT3X_I2C_TIMEOUT) != HAL_OK) {
		return 0;
	}

	return 1;
}
//wordt gebruikt om de meerdere losse bytes die via I2C ontvangen worden aan elkaar te hechten 
static uint16_t uint8_to_uint16(uint8_t msb, uint8_t lsb)
{
	return (uint16_t)((uint16_t)msb << 8u) | lsb;
}

int sht3x_init(sht3x_handle_t *handle)
{
	assert(handle->i2c_handle->Init.NoStretchMode == I2C_NOSTRETCH_DISABLE);

	uint8_t status_reg_and_checksum[3];
	if (HAL_I2C_Mem_Read(handle->i2c_handle, handle->device_address << 1u, SHT3X_COMMAND_READ_STATUS, 2, (uint8_t*)&status_reg_and_checksum,
					  sizeof(status_reg_and_checksum), SHT3X_I2C_TIMEOUT) != HAL_OK) {
		return 0;
	}

	uint8_t calculated_crc = calculate_crc(status_reg_and_checksum, 2);

	if (calculated_crc != status_reg_and_checksum[2]) {
		return 0;
	}

	return 1;
}
///stuur lees commando naar de sensor en zet de data om in de gewenste grootheden in de meegegeven pointers
int sht3x_read_temperature_and_humidity(sht3x_handle_t *handle, uint16_t *temperature, uint16_t *humidity)
{
	sht3x_send_command(handle, SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH);

	HAL_Delay(1);

	uint8_t buffer[6];
	if (HAL_I2C_Master_Receive(handle->i2c_handle, handle->device_address << 1u, buffer, sizeof(buffer), SHT3X_I2C_TIMEOUT) != HAL_OK) {
		return 0;
	}

	uint8_t temperature_crc = calculate_crc(buffer, 2);
	uint8_t humidity_crc = calculate_crc(buffer + 3, 2);
	if (temperature_crc != buffer[2] || humidity_crc != buffer[5]) {
		return 0;
	}

	uint16_t temperature_raw = uint8_to_uint16(buffer[0], buffer[1]);
	uint16_t humidity_raw = uint8_to_uint16(buffer[3], buffer[4]);

//	*temperature = -45.0f + 175.0f * temperature_raw / 65535.0f;

	*temperature = temperature_raw * 175/65535 - 45;

//	*humidity = 100.0f * humidity_raw / 65535.0f;

	*humidity = humidity_raw * 100 / 65535;

	return 1;
}

int sht3x_set_header_enable(sht3x_handle_t *handle, int enable)
{
	if (enable) {
		return sht3x_send_command(handle, SHT3X_COMMAND_HEATER_ENABLE);
	} else {
		return sht3x_send_command(handle, SHT3X_COMMAND_HEATER_DISABLE);
	}
}

///GeSTMificeerde arduino functie die een SPI bericht MSB first de deur uit gooit, was eigenlijk overbodig als k gewoon de SPI MSB/LSB setting in de ioc had aangepast  
void shiftOut(uint8_t val)
{
      uint8_t i;

      for (i = 0; i < 8; i++)  {

           	HAL_GPIO_WritePin(GPIOB,SPI_MOSI_TEST_Pin, !!(val & (1 << (7 - i))));

            HAL_GPIO_WritePin(GPIOB,SPI_CLK_TEST_Pin, SET);
            HAL_GPIO_WritePin(GPIOB,SPI_CLK_TEST_Pin, RESET);
      }
}
///Spi implementatie omdat de STM SPI niet wilde werken, achteraf overbodig als ik gwn de ioc goed had gemaakt
void spiTransfer(int addr, volatile uint8_t opcode, volatile uint8_t data) {
    //Create an array with the data to shift out
    int offset=addr*2;
    int maxbytes=maxDevices*2;

    for(int i=0;i<maxbytes;i++)
        spidata[i]=(uint8_t)0;
    //put our device data into the array
    spidata[offset+1]=opcode;
    spidata[offset]=data;
    //enable the line
    HAL_GPIO_WritePin(GPIOB,SPI_CS_TEST_Pin,RESET);

    //Now shift out the data
    for(int i=maxbytes;i>0;i--)
    {
    	shiftOut(spidata[i-1]);
    }

    //latch the data onto the display
    HAL_GPIO_WritePin(GPIOB,SPI_CS_TEST_Pin,SET);
}
///set een specifieke led op de matrix
void setLed(int addr, int row, int column, int state) {
    int offset;
    uint8_t val=0x00;

    offset=addr*8;
    val=0b10000000 >> column;
    if(state)
        status[offset+row]=status[offset+row]|val;
    else {
        val=~val;
        status[offset+row]=status[offset+row]&val;
    }
    spiTransfer(addr, row+1,status[offset+row]);
}
///gebruik de setled 8 keer om een hele rij te setten :o
void setRow(int addr, int row, uint8_t value) {
    int offset;
    offset=addr*8;
    status[offset+row]=value;
    spiTransfer(addr, row+1,status[offset+row]);
}
///gebruik de setled 8 keer om een hele colom te setten :o
void setColumn(int addr, int col, uint8_t value) {
	uint8_t val;
    for(int row=0;row<8;row++) {
        val=value >> (7-row);
        val=val & 0x01;
        setLed(addr,row,col,val);
    }
}

///stuurt een clear commando naar de display
void clearDisplay(int addr) {
	int offset = addr*8;

    for(int i=0;i<8;i++) {
    	status[offset+i]=0;
    	spiTransfer(addr, i+1,status[offset+i]);
    }
}

///print een karakter op de display, Narray is de pointer naar t karakter, Size de lengte in bytes van het teken en spacing hoeveel pixels die opgeschoven wordt
void printthingy(const uint8_t* Narray, int ofset, int size, int spacing)
{
  for (int i = 0; i < size; i++)
  {
    setColumn((3-(ofset/8)), (i + ofset%8), Narray[i]);//3-(ofset/8) vind t juiste address van de matrix, i + ofset%8 de juiste colom binnen die matrix
  }
  for (int i = 0; i < spacing; i++)
  {
    setColumn((3-(ofset/8)), (size + ofset%8), 0x00);//voeg extra emptys toe na de geprinte char
  }
}
///vertaal een chararray zijn askii naar pointers binnen een array die gebruikt worden voor printthingy met de juiste ofset ertussen(ofset is overal nu 0 maar de optie bestaat) 
void displayTekst(char* txt, int size, int ofset)
{
  for (int i = 0; i < size; i++)
  {
	if (txt[i] >= 65)//is t een letter
	{
		printthingy(&LetterArray[(txt[i]-65)*3],i*4+ofset, 3, 1);
//txt[i]-65 vertaald het ascii char naar een bruikbare int, an *3 omdat de chars in de letterarray 3bytes groot zijn

	}
	else if (txt[i] >= 48)//is t een getal
	{
		printthingy(&NumberArray[(txt[i]-48)*3],i*4+ofset, 3, 1);
	}
	if (txt[i] == 37)//%teken
	{
		printthingy(&SpecialCharArray[3],i*4+ofset, 3, 1);
	}
	if (txt[i] == 42)//*teken
	{
		printthingy(&SpecialCharArray[0],i*4+ofset, 3, 1);
	}
  }
}

///initialiseer de SPI connectie met de LEDMatrix
void spiInit(void)
{
	for(int i=0;i<64;i++) status[i]=0x00;//set de status array op 0, er is niks te zien
    for(int i=0;i<maxDevices;i++) {
        spiTransfer(i,OP_DISPLAYTEST,0);
        //scanlimit is set to max on startup
        setScanLimit(i,7);
        //decode is done in source
        spiTransfer(i,OP_DECODEMODE,0);
        clearDisplay(i);
        //we go into shutdown-mode on startup
        shutdown(i,0);
    }
}


char* MsgPointer = NULL;
char WelkomMsg[8] = "WELKOM[[";
char interuptstring[8] = "[[[[[[[[";
char TempMsg[8] = "T[[*V[[%";
char RejectMsg[8] = "ONBEKEND";
char RejectMsg2[8] = "PASJE[[[";

///ongebruikte buiten temperatuur/vocht, kan nog geimplementeerd worden
int BTemp = 0;
int BVocht = 0;



int MsgSize = 8;
///Dit wordt elke 2 seconden in een interrupt aangeroepen
///Mode -1/ geen Mode = niks
///Mode 0 = innit van de timer
///Mode 1 = Welkom, gaat de volgende keer naar 2
///Mode 2 = wie er welkom geheten wordt, gaat terug naar 5
///Mode 3 = ONBEKEND, gaat naar 4
///Mode 4 = PASJE, gaat terug naar 5
///Mode 5 = Temperatuur + Luchtvocht
///Mode 6 = een delay voor het welkombericht, doet verder niks, gaat naar 1
void TimerMsg(int TempMode)
{
	static Mode;
	if (TempMode != -1)Mode = TempMode;
	switch(Mode)
	{
	case 0:
		__HAL_TIM_SET_AUTORELOAD(&htim2,2000);//2 seconden tussen elke Matrix update
		HAL_TIM_Base_Start_IT(&htim2);
		Mode = 5;//terug naar de temperatuur
		break;
	case 1:
		__HAL_TIM_SET_COUNTER(&htim2,1950);//zorg ervoor dat ie direct welkom zegt
		MsgPointer = &WelkomMsg;//msg wordt WELKOM
		Mode = 2;//nu de naam van de welkom gehetene
		break;
	case 2:
		MsgPointer = &interuptstring;//msg wordt hetgeen wat is ontvangen over de bus
		Mode = 5;//terug naar de temperatuur
		break;
	case 3:
		__HAL_TIM_SET_COUNTER(&htim2,1950);//zorg ervoor dat ie direct wegwezen zegt
		MsgPointer = &RejectMsg;//msg wordt ONBEKEND
		Mode = 4;//Op naar PASJE
		break;
	case 4:
		MsgPointer = &RejectMsg2;//msg wordt PASJE
		Mode = 5;//terug naar de temperatuur
		break;
	case 5:
		MsgPointer = &TempMsg;//
		break;
	case 6:
		//even wachten
		static int wait = 0;
		wait++;
		if (wait > 2)Mode = 1,wait = 0;
		break;
	}
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

///init alle sensoren + lichtkrant en run elke 2 seconden de temperatuur sensor
int main(void)
{

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
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_I2C3_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  char msg[] = "Begin matrix powah!!";
  HAL_UART_Transmit(&huart2, msg, sizeof(msg),HAL_MAX_DELAY);


  HAL_UART_Receive_IT(&huart1, InteruptData, 1);//zet de intterupt voor t onvangen van de bus aan

  TempVochtSensor.device_address = 0x44;//vult de struct met de juiste data
  TempVochtSensor.i2c_handle = &hi2c3;


  spiInit();//init led matrix
  //Pins:
  //CLK D11, MOSI D5, CS D4

  sht3x_init(&TempVochtSensor);//init vochtemp sensoren

  for (int i = 0; i < 4; i++)//basis instellingen van alle vier matrixen
  {
	  shutdown(i,1);
  	  setIntensity(i,4);
  	  clearDisplay(i);
  }

  TimerMsg(0);
//  HAL_TIM_Base_Start_IT(&htim2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uint16_t TEMP = 0;
	  uint16_t VOCHT = 0;
	  if (sht3x_read_temperature_and_humidity(&TempVochtSensor,&TEMP,&VOCHT))
	  {
	  TempMsg[1] = TEMP/10%10 + 48;
	  TempMsg[2] = TEMP%10 + 48;
	  TempMsg[5] = VOCHT/10%10 + 48;
	  TempMsg[6] = VOCHT%10 + 48;
	  }
	  else TempMsg[6] = 'E';
	  HAL_Delay(2000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/* USER CODE BEGIN 4 */

///de interupt waar elke 2 sec TimerMsg word aangeroepen
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)//update het display elke seconde
{
	if (htim == &htim2)
	{
		displayTekst(MsgPointer, MsgSize, 0);
		TimerMsg(-1);
	}
}




#define WAITFORCOMMAND 0//had ook gwn een enumm kunnen zijn
#define WELKOM 1
#define TVBUITEN 2
#define WEGWEZEN 3

int COMMAND = 0;
///de interrupt van de seriele Bus
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	static int input = 0;
	if (huart == &huart1)
	{

		switch(COMMAND)
		{
			case WELKOM:// als er over de bus ACCEPT is gelezen


				if (InteruptData[0] == '\r')//stop met lezen
				{
					TimerMsg(6);//gooi de tekst op de matrix na een kleine delay
					COMMAND = WAITFORCOMMAND;//t bericht is voorbij terug naar de default
					input = 0;
				}
				else if (input < 8)//zo niet stop, dan zet de char in de tekst array
				{
					if (InteruptData[0] >= 48)//checken of t een getal/letter is
					{
						interuptstring[input] = InteruptData[0];
						input++;
					}
				}
				break;

			case WEGWEZEN:
				TimerMsg(3);
				COMMAND = WAITFORCOMMAND;
				break;

			case TVBUITEN://dit is logica om de buitentemperatuur te ontvangen, wordt in de demo niet gebruikt
				if (input < 2)
				{
					if (input == 0) BTemp = InteruptData[0];
					if (input == 1) BVocht = InteruptData[0];
					input++;
				}
				else COMMAND = WAITFORCOMMAND, input = 0;;
				break;
			case WAITFORCOMMAND:
				if (InteruptData[0] == '!')//!
				{
					COMMAND = WELKOM;
					for (int i = 0; i < 8; i++)//reset de tekst array voor de WELKOM
					{
						interuptstring[i] = '[';
					}
				}
				if (InteruptData[0] == '#') COMMAND = WEGWEZEN;
//				if (InteruptData[0] == 128) COMMAND = TVBUITEN;
				break;

		}

		HAL_UART_Transmit(&huart2, &InteruptData[0], sizeof(InteruptData[0]), HAL_MAX_DELAY);

		HAL_UART_Receive_IT(&huart1, InteruptData, 1);
	}
}


}
