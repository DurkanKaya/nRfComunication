/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Register_Address.h"
//static uint64_t pipe0_reading_address;
//static uint8_t ack_payload_length;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define CMD_TX_ADDR 0x10
#define CMD_RX_ADDR_P5 0x0F
uint8_t PWR_PRIM[2];
uint8_t ReceiveData[4];
uint8_t nRFReceiveDataBuffer[10];

#define W_TX_PAYLOAD 0xA0
#define REUSE_TX_PL 0xE3
#define NOP 0xFF
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

/* USER CODE BEGIN PV */
uint8_t rxdata;
uint8_t BufferCnt = 0;
uint8_t NrfBuffer[255];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == &hspi2)
	{
		NrfBuffer[BufferCnt++] = rxdata;
		if(BufferCnt>254)
			BufferCnt = 0;
	}
	HAL_SPI_Receive_IT(&hspi2, &rxdata, 1);
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void NRF24_DelayMicroSeconds(uint32_t uSec)
{
	uint32_t uSecVar = uSec;
	uSecVar = uSecVar* ((SystemCoreClock/1000000)/3);
	while(uSecVar--);
}
//SMD TRANSM??T.
void PinStatusSetter(uint16_t gpio_pin,int pin_state)
{
	HAL_GPIO_WritePin(GPIOA,gpio_pin , pin_state);
}

uint8_t Nrf24ReadRegisterSpi1(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t spiBuf[3];
	uint8_t retData;
	//Put CSN low
	PinStatusSetter(DIP_CSN_Pin , 0);
	//Transmit register address
	spiBuf[0] = reg&0x1F;
	HAL_SPI_Transmit(&hspix, spiBuf, 1, 100);
	//Receive data
	HAL_SPI_Receive(&hspix, &spiBuf[1], 1, 100);
	retData = spiBuf[1];
	//Bring CSN high
	PinStatusSetter(DIP_CSN_Pin , 1);
	spiBuf[1] =0;
	return retData;
}
void Nrf24WriteRegisterSpi1(SPI_HandleTypeDef hspix,uint8_t reg, uint8_t value)
{
	uint8_t spiBuf[3];
	//Put CSN low
	PinStatusSetter(DIP_CSN_Pin , 0);
	//Transmit register address and data
	spiBuf[0] = reg|0x20;
	spiBuf[1] = value;
	HAL_SPI_Transmit(&hspix, spiBuf, 2, 100);
	//Bring CSN high
	PinStatusSetter(DIP_CSN_Pin ,1);
	spiBuf[1] = 0;
}
//Spi1'e ba??lanan nRF24L01'in TX_ADDR Register??ndaki 5 bytelik adresi okuyoruz.
void TxAddrReadSpi1(SPI_HandleTypeDef hspix,uint8_t reg ,uint8_t len)
{
	uint8_t Buf[3];
	uint8_t address[5];
	Buf[0] = reg&0x1F;

	PinStatusSetter(DIP_CSN_Pin ,0);
	HAL_SPI_Transmit(&hspix, &Buf[0], 1,100);
	HAL_SPI_Receive(&hspix, address, len,100);

	PinStatusSetter(DIP_CSN_Pin , 1);
}
//Spi1'e ba??lanan nRF24L01'in TX_ADDR Register??na 5 bytelik de??er veriyoruz.
void TxAddrSpi1(SPI_HandleTypeDef hspix ,uint8_t reg  ,uint8_t len)
{
	uint8_t Buf[1];

	uint8_t REG_TX_ADDR_VALUE[5];
	REG_TX_ADDR_VALUE[0] = 0x11,REG_TX_ADDR_VALUE[1] =0x11,REG_TX_ADDR_VALUE[2] =0x11,
			REG_TX_ADDR_VALUE[3] =0x11,REG_TX_ADDR_VALUE[4] =0x11;
	Buf[0] = reg|0x20;

	PinStatusSetter(DIP_CSN_Pin , 0);
	HAL_SPI_Transmit(&hspix, Buf, 2, 100);
	HAL_SPI_Transmit(&hspix, (uint8_t*)REG_TX_ADDR_VALUE, len, 100);
	PinStatusSetter(DIP_CSN_Pin , 1);
}
//SPI1'e ba??lad??????m??z nRF24L01'in RX_ADDR_P0 adresini yaz??yoruz.
void RX_ADDR_P0_Write(SPI_HandleTypeDef hspix,uint8_t reg,uint8_t len)
{
	uint8_t Buf[1];
	Buf[0] = reg|0x20;
	uint8_t REG_RX_ADDR_P0_VALUE[5];

	REG_RX_ADDR_P0_VALUE[0] = 0x11,REG_RX_ADDR_P0_VALUE[1] =0x11,REG_RX_ADDR_P0_VALUE[2] =0x11,
			REG_RX_ADDR_P0_VALUE[3] =0x11,REG_RX_ADDR_P0_VALUE[4] =0x11;
	PinStatusSetter(DIP_CSN_Pin , 0);
	HAL_SPI_Transmit(&hspix, Buf, 1, 100);
	HAL_SPI_Transmit(&hspix,REG_RX_ADDR_P0_VALUE, len, 100);
	PinStatusSetter(DIP_CSN_Pin ,1);
}
//SPI1'e ba??lad??????m??z nRF24L01'in RX_ADDR_P0 adresini okuyoruz.
void RX_ADDR_P0_Read(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t ReadBuf[5];
	uint8_t Buf[1];
	Buf[0] = reg&0x1F;
	PinStatusSetter(DIP_CSN_Pin , 0);
	HAL_SPI_Transmit(&hspix, Buf, 1, 100);
	HAL_SPI_Receive(&hspix, ReadBuf, 5 , 100);
	PinStatusSetter(DIP_CSN_Pin ,1);
}
//SPI2'ye ba??l?? nRF24L01'in data rate ve ????k???? g??c??n?? ayarl??yoruz.
void RfSetupSp1(SPI_HandleTypeDef hspix,uint8_t reg,uint8_t value)
{
	uint8_t spiBuf[3];
	//Put CSN low
	PinStatusSetter(DIP_CSN_Pin , 0);
	//Transmit register address and data
	spiBuf[0] = reg|0x20;
	spiBuf[1] = value;
	HAL_SPI_Transmit(&hspix, spiBuf, 2, 100);
	//Bring CSN high
	PinStatusSetter(DIP_CSN_Pin ,1);
	spiBuf[1] = 0;
}
//SPI2'ye ba??l?? nRF24L01'in data rate ve RF ????k???? g??c??n?? okuyoruz.
uint8_t RfSetupReadSp1(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t spiBuf[3];
	uint8_t retData =0;
	//Put CSN low
	PinStatusSetter(DIP_CSN_Pin , 0);
	//Transmit register address
	spiBuf[0] = reg&0x1F;
	HAL_SPI_Transmit(&hspix, spiBuf, 1, 100);
	//Receive data
	HAL_SPI_Receive(&hspix, &spiBuf[1], 1, 100);
	retData = spiBuf[1];
	//Bring CSN high
	PinStatusSetter(DIP_CSN_Pin , 1);
	spiBuf[1] =0;
	return retData;
}
//Rf Sinyalini ayarl??yoruz.
void RfChSpi1(SPI_HandleTypeDef hspix,uint8_t reg,uint8_t value)
{
	uint8_t spiBuf[3];
	//Put CSN low
	PinStatusSetter(DIP_CSN_Pin , 0);
	//Transmit register address and data
	spiBuf[0] = reg|0x20;
	spiBuf[1] = value;
	HAL_SPI_Transmit(&hspix, spiBuf, 2, 100);
	//Bring CSN high
	PinStatusSetter(DIP_CSN_Pin ,1);
	spiBuf[1] = 0;
}
//Rf sinyalini okuyoruz.
uint8_t RfChReadSpi1(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t spiBuf[3];
	uint8_t retData =0;
	//Put CSN low
	PinStatusSetter(DIP_CSN_Pin , 0);
	//Transmit register address
	spiBuf[0] = reg&0x1F;
	HAL_SPI_Transmit(&hspix, spiBuf, 1, 100);
	//Receive data
	HAL_SPI_Receive(&hspix, &spiBuf[1], 1, 100);
	retData = spiBuf[1];
	//Bring CSN high
	PinStatusSetter(DIP_CSN_Pin , 1);
	spiBuf[1] =0;
	return retData;
}
//Payload edece??imiz datay?? yaz??yoruz.
void TxPayload(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t pld[32] = "ELEKTRONIKBILISIMSISTEMLUHMANN";

	PinStatusSetter(DIP_CSN_Pin , 0);
	HAL_SPI_Transmit(&hspix, &reg, 1, 100);
	HAL_SPI_Transmit(&hspix, pld, 32, 100);
	PinStatusSetter(DIP_CSN_Pin , 1);

	PinStatusSetter(DIP_CE_Pin, 1);
	NRF24_DelayMicroSeconds(15);
}
//Otomatik Yeniden ??letim Kurulumu
void SetupRetr(SPI_HandleTypeDef hspix ,uint8_t reg ,uint8_t deg)
{
	uint8_t Buf[3];
	Buf[0] = reg|0x20;
	Buf[1] = deg;
	PinStatusSetter(DIP_CSN_Pin , 0);
	HAL_SPI_Transmit(&hspix, Buf, 2, 100);
	PinStatusSetter(DIP_CSN_Pin , 1);
}
//Otomatik yeniden iletim durumu
void SetupRetrRead(SPI_HandleTypeDef hspix ,uint8_t reg)
{
	uint8_t Buf[2];
	Buf[0] = reg&0x1F;
	PinStatusSetter(DIP_CSN_Pin , 0);
	HAL_SPI_Transmit(&hspix, &Buf[0], 1, 100);
	HAL_SPI_Receive(&hspix, &Buf[1], 1, 100);
	PinStatusSetter(DIP_CSN_Pin , 1);
}
//Otomatik bilgilendimeyi aktif ediyoruz.
void EN_AA_Register(SPI_HandleTypeDef hspix,uint8_t reg ,uint8_t deg)
{
	uint8_t Buf[3];
	Buf[0] =  reg|0x20;
	Buf[1] = deg;
	PinStatusSetter(DIP_CSN_Pin ,0);
	HAL_SPI_Transmit(&hspix, Buf, 2, 100);
	PinStatusSetter(DIP_CSN_Pin ,1);
}
//Otomatik bilgilendirme registerini okuoyoruz.
void EN_AA_RegisterRead(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t Buf[2];
	Buf[0] = reg&0x1F;
	PinStatusSetter(DIP_CSN_Pin , 0);
	HAL_SPI_Transmit(&hspix, &Buf[0], 1, 100);
	HAL_SPI_Receive(&hspix, &Buf[1], 1, 100);
	PinStatusSetter(DIP_CSN_Pin , 1);
}
//SPI1 ye ba??l?? Nrf24l01 lerin Rx Adreslerini enable ediyoruz.
void En_RxAddressSpi1(SPI_HandleTypeDef hspix,uint8_t reg,uint8_t value)
{
	uint8_t spiBuf[3];
	//Put CSN low
	PinStatusSetter(DIP_CSN_Pin , 0);
	//Transmit register address and data
	spiBuf[0] = reg|0x20;
	spiBuf[1] = value;
	HAL_SPI_Transmit(&hspix, spiBuf, 2, 100);
	//Bring CSN high
	PinStatusSetter(DIP_CSN_Pin ,1);
	spiBuf[1] = 0;
}
//SPI1 ye ba??l?? Nrf24l01 lerin Rx Adreslerini okuyoruz.
uint8_t En_RxReadAddressSpi1(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t spiBuf[3];
	uint8_t retData =0;
	//Put CSN low
	PinStatusSetter(DIP_CSN_Pin , 0);
	//Transmit register address
	spiBuf[0] = reg&0x1F;
	HAL_SPI_Transmit(&hspix, spiBuf, 1, 100);
	//Receive data
	HAL_SPI_Receive(&hspix, &spiBuf[1], 1, 100);
	retData = spiBuf[1];
	//Bring CSN high
	PinStatusSetter(DIP_CSN_Pin , 1);
	spiBuf[1] =0;
	return retData;
}
//Ack de??erini okuyoruz.
void RxAdrP0AckRead(SPI_HandleTypeDef hspix)
{
	uint8_t ACK_ReadData[10];
	PinStatusSetter(SMD_CE_Pin, 1);
	NRF24_DelayMicroSeconds(150);
	uint8_t AckRead = CMD_R_RX_PAYLOAD;

	PinStatusSetter(DIP_CSN_Pin , 0);
	HAL_SPI_Transmit(&hspix, &AckRead, 1, 100);
	HAL_SPI_Receive(&hspix, ACK_ReadData, 10, 100);
	PinStatusSetter(DIP_CSN_Pin , 1);
	PinStatusSetter(SMD_CE_Pin, 0);

}
//Tx ??nitializasyon
void Nrf24InitTx()
{
	Nrf24WriteRegisterSpi1(hspi1,0x00,0x0A);
	Nrf24ReadRegisterSpi1(hspi1,0x00);
	TxAddrSpi1(hspi1 ,10 ,5);
	TxAddrReadSpi1(hspi1, 10 ,5);
	RX_ADDR_P0_Write(hspi1,0x0A,5);
	RX_ADDR_P0_Read(hspi1 ,0x0A);
	EN_AA_Register(hspi1,0x01,0x3F);
	EN_AA_RegisterRead(hspi1, 0x01);
	En_RxAddressSpi1(hspi1, 0x02, 0x3F);
	En_RxReadAddressSpi1(hspi1, 0x02);
	SetupRetr(hspi1 ,0x04 ,0x00);
	SetupRetrRead(hspi1 ,0x04);
	RfSetupSp1(hspi1, 0x06, 0x0E);
	RfSetupReadSp1(hspi1, 0x06);
	RfChSpi1(hspi1, 0x05, 0x02);
	RfChReadSpi1(hspi1, 0x05);

}
//Prim ve power pinlerini ayarl??yoruz.
void Nrf24PrimPowerRegisterSpi2(SPI_HandleTypeDef hspix,uint8_t reg ,uint8_t deg)
{
	uint8_t Buf[3];
	Buf[0] = reg|20;
	Buf[1] = deg;
	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, Buf, 2, 100);
	PinStatusSetter(SMD_CSN_Pin, 1);
}
//Prim ve power bitlerini okuyoruz.
uint8_t Nrf24PrinPowerRegisterReadSpi2(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t regdata;
	uint8_t Buf[2];
	Buf[0] = reg&0x1F;

	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, &Buf[0], 1, 100);
	HAL_SPI_Receive(&hspix, &Buf[1], 1, 100);
	regdata =Buf[1];
	PinStatusSetter(SMD_CSN_Pin, 1);

	return regdata;
}
//SPI2 ye ba??l?? Nrf24l01 lerin Rx Adreslerini enable ediyoruz.
void NRF24_EN_RXADDR_SMD(SPI_HandleTypeDef hspix,uint8_t reg,uint8_t deg)
{
	uint8_t Buf[3];
	Buf[0] = reg|20;
	Buf[1] = deg;
	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, Buf, 2, 100);
	PinStatusSetter(SMD_CSN_Pin, 1);
}
//SPI2 ye ba??l?? Nrf24l01 lerin Rx Adreslerini okuyoruz.
uint8_t NRF24_EN_RXADDR_READ_SMD(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t Buf[2];
	Buf[0] = reg&0x1F;
	uint8_t retdata;

	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, &Buf[0], 1, 100);
	HAL_SPI_Receive(&hspix, &Buf[1], 1, 100);
	PinStatusSetter(SMD_CSN_Pin, 1);
	retdata = Buf[1];

	return retdata;
}
//Otomatik bilgilendirme registerini ayarl??yoruz.
void EN_AA_Smd(SPI_HandleTypeDef hspix,uint8_t reg,uint8_t deg)
{
	uint8_t Buf[3];
	Buf[0] = reg|20;
	Buf[1] = deg;
	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, Buf, 2, 100);
	PinStatusSetter(SMD_CSN_Pin, 1);
}
//Otomatik bilgilendirme registerini ayarl??yoruz.
void EN_AA_ReadSmd(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t Buf[2];
	Buf[0] = reg&0x1F;

	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, &Buf[0], 1, 100);
	HAL_SPI_Receive(&hspix, &Buf[1], 1, 100);
	PinStatusSetter(SMD_CSN_Pin, 1);

}
//Y??k geni??liklerini ayarl??yoruz.
void RxPwP0Spi2(SPI_HandleTypeDef hspix,uint8_t reg,uint8_t deg)
{
	uint8_t Buf[3];
	Buf[0] = reg|20;
	Buf[1] = deg;
	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, Buf, 2, 100);
	PinStatusSetter(SMD_CSN_Pin, 1);
}
//Y??k geni??ligini ayarlad??ktan sonra test ediyoruz.
uint8_t RwPwP0ReadSpi2(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t Buf[2];
	Buf[0] = reg&0x1F;
	uint8_t retdata;

	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, &Buf[0], 1, 100);
	HAL_SPI_Receive(&hspix, &Buf[1], 1, 100);
	PinStatusSetter(SMD_CSN_Pin, 1);
	retdata = Buf[1];

	return retdata;
}
//Otomatik bilgilendirme i??in Tx cihaz??n??n Rx pipe 5'i ayarl??yoruz.
void RX_ADDR_P5_SMD(SPI_HandleTypeDef hspix ,uint8_t reg  ,uint8_t len)
{
	uint8_t Buf[1];

	uint8_t RX_ADDR_P5_VALUE[5];
	RX_ADDR_P5_VALUE[0] = 0x11,RX_ADDR_P5_VALUE[1] =0x11,RX_ADDR_P5_VALUE[2] =0x11,
			RX_ADDR_P5_VALUE[3] =0x11,RX_ADDR_P5_VALUE[4] =0x11;
	Buf[0] = reg|0x20;

	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, Buf, 1, 100);
	HAL_SPI_Transmit(&hspix, (uint8_t*)RX_ADDR_P5_VALUE, len, 100);
	PinStatusSetter(SMD_CSN_Pin, 1);
}
//Otomatik bilgilendirme i??in Tx cihaz??n??n Rx pipe 5'i okuyoruz.
void RX_ADDR_P5_READ_SMD(SPI_HandleTypeDef hspix ,uint8_t reg)
{
	uint8_t Buf[3];
	Buf[0] = reg&0x1F;
	uint8_t retbuf[5];

	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, Buf, 1, 100);
	HAL_SPI_Receive(&hspix, retbuf, 5, 100);
	PinStatusSetter(SMD_CSN_Pin, 1);
}
//Spi2 ye ba??l?? nRF24L01'in frekans ayarlar??n?? yap??yoruz.
void RfChSpi2(SPI_HandleTypeDef hspix,uint8_t reg,uint8_t value)
{
	uint8_t spiBuf[3];
	//Put CSN low
	PinStatusSetter(SMD_CSN_Pin , 0);
	//Transmit register address and data
	spiBuf[0] = reg|0x20;
	spiBuf[1] = value;
	HAL_SPI_Transmit(&hspix, spiBuf, 2, 100);
	//Bring CSN high
	PinStatusSetter(SMD_CSN_Pin ,1);
	spiBuf[1] = 0;
}
//Spi2 ye ba??l?? nRF24L01'in frekans ayarlar??n?? okuyoruz.
uint8_t RfChReadSpi2(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t spiBu[3];
	uint8_t retData = 0;
	//Put CSN low
	PinStatusSetter(SMD_CSN_Pin , 0);
	//Transmit register address
	spiBu[0] = reg&0x1F;
	HAL_SPI_Transmit(&hspix, spiBu, 1, 100);
	//Receive data
	HAL_SPI_Receive(&hspix, &spiBu[1], 1, 100);
	retData = spiBu[1];
	//Bring CSN high
	PinStatusSetter(SMD_CSN_Pin , 1);
	spiBu[1] =0;
	return retData;
}
//SPI2'ye ba??l?? nRF24L01'in data rate ve ????k???? g??c??n?? ayarl??yoruz.
void RfSetupSp2(SPI_HandleTypeDef hspix,uint8_t reg,uint8_t value)
{
	uint8_t spiBuf[3];
	//Put CSN low
	PinStatusSetter(SMD_CSN_Pin , 0);
	//Transmit register address and data
	spiBuf[0] = reg|0x20;
	spiBuf[1] = value;
	HAL_SPI_Transmit(&hspix, spiBuf, 2, 100);
	//Bring CSN high
	PinStatusSetter(SMD_CSN_Pin ,1);
	spiBuf[1] = 0;
}
//SPI2'ye ba??l?? nRF24L01'in data rate ve RF ????k???? g??c??n?? okuyoruz.
uint8_t RfSetupReadSp2(SPI_HandleTypeDef hspix,uint8_t reg)
{
	uint8_t spiBu[3];
	uint8_t retData = 0;
	//Put CSN low
	PinStatusSetter(SMD_CSN_Pin , 0);
	//Transmit register address
	spiBu[0] = reg&0x1F;
	HAL_SPI_Transmit(&hspix, spiBu, 1, 100);
	//Receive data
	HAL_SPI_Receive(&hspix, &spiBu[1], 1, 100);
	retData = spiBu[1];
	//Bring CSN high
	PinStatusSetter(SMD_CSN_Pin , 1);
	spiBu[1] =0;
	return retData;
}
//R_RX_PAYLOAD registerine kaydedilen datay?? okuyoruz.
void R_RxPayload(SPI_HandleTypeDef hspix ,uint8_t reg)
{
	PinStatusSetter(SMD_CE_Pin, 1);
	NRF24_DelayMicroSeconds(150);
	uint8_t Data[32];

	PinStatusSetter(SMD_CSN_Pin, 0);
	HAL_SPI_Transmit(&hspix, &reg, 1, 100);
	HAL_SPI_Receive(&hspix, Data,32,100);
	PinStatusSetter(SMD_CSN_Pin, 1);
	PinStatusSetter(SMD_CE_Pin, 0);
}
//Rx ??nitializasyon
void nRF24_Init_Rx()
{
	Nrf24PrimPowerRegisterSpi2(hspi2 ,0x00 ,0x0B);
	Nrf24PrinPowerRegisterReadSpi2(hspi2, 0x00);
	NRF24_EN_RXADDR_SMD(hspi2, 0x02, 0x03);
	NRF24_EN_RXADDR_READ_SMD(hspi2, 0x02);
	EN_AA_Smd(hspi2, 0X01, 0x3F);
	EN_AA_ReadSmd(hspi2, 0X01);
	RxPwP0Spi2(hspi2, 0x11, 0x1F);
	RwPwP0ReadSpi2(hspi2, 0x11);
	RX_ADDR_P5_SMD(hspi2 ,0x0F ,5);
	RX_ADDR_P5_READ_SMD(hspi2 ,0x0F);
	RfChSpi2(hspi2,0x05, 0x02);
	RfChReadSpi2(hspi2, 0x05);
	RfSetupSp2(hspi2, 0x06, 0x0E);
	RfSetupReadSp2(hspi2, 0X06);
	TxPayload(hspi1,0xA0);
	R_RxPayload(hspi2 ,0x61);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
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
	PinStatusSetter(SMD_CSN_Pin, 1);
	PinStatusSetter(SMD_CE_Pin, 0);

	PinStatusSetter(DIP_CSN_Pin, 1);
	PinStatusSetter(DIP_CE_Pin ,0);

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
	//HAL_SPI_Receive_IT(&hspi2, &rxdata, 1);
	Nrf24InitTx();
	nRF24_Init_Rx();
    RxAdrP0AckRead( hspi1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{

		HAL_Delay(1);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_SLAVE;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SMD_CSN_Pin|DIP_CSN_Pin|SMD_CE_Pin|DIP_CE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DIPnRF_IRQ_Pin|SMDnRF_IRQ_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : SMD_CSN_Pin DIP_CSN_Pin SMD_CE_Pin DIP_CE_Pin */
  GPIO_InitStruct.Pin = SMD_CSN_Pin|DIP_CSN_Pin|SMD_CE_Pin|DIP_CE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DIPnRF_IRQ_Pin SMDnRF_IRQ_Pin */
  GPIO_InitStruct.Pin = DIPnRF_IRQ_Pin|SMDnRF_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
