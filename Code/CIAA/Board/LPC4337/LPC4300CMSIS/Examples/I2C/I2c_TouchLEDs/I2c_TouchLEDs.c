/**********************************************************************
* $Id$		I2c_Master.c	2011-06-02
*//**
* @file		I2c_Master.c
* @brief	This example describes how to configure I2C as master device
* @version	1.0
* @date		02. June. 2011
* @author	NXP MCU SW Application Team
*
* Copyright(C) 2011, NXP Semiconductor
* All rights reserved.
*
***********************************************************************
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
* Permission to use, copy, modify, and distribute this software and its
* documentation is hereby granted, under NXP Semiconductors'
* relevant copyright in the software, without fee, provided that it
* is used in conjunction with NXP Semiconductors microcontrollers.  This
* copyright, permission, and disclaimer notice must appear in all copies of
* this code.
**********************************************************************/
#include "lpc43xx_i2c.h"
#include "lpc43xx_cgu.h"
#include "lpc43xx_gpio.h"
#include "lpc43xx_scu.h"
#include "lpc43xx_libcfg.h"
#include "debug_frmwrk.h"



/* Example group ----------------------------------------------------------- */
/** @defgroup I2c_Master	I2c_Master
 * @ingroup I2C_Examples
 * @{
 */


/************************** PRIVATE DEFINITIONS *************************/
#define USEDI2CDEV_M		0

#define I2CDEV_UDA1380_ADDR		(0x34>>1)
#define UDA1380_CLOCK_CONFIGURE	0x00
/** Max buffer length */
#define BUFFER_SIZE			0x10

#if (USEDI2CDEV_M == 0)
#define I2CDEV_M LPC_I2C0
#elif (USEDI2CDEV_M == 2)
#define I2CDEV_M LPC_I2C2
#else
#error "Master I2C device not defined!"
#endif

#define LED1_BIT			1 //LEDUSB
#define LED1_PORT			4


/************************** PRIVATE VARIABLES *************************/
uint8_t menu1[] =
"NXP LPC4350 Platform\n";

/** These global variables below used in interrupt mode - Slave device ----------------*/
uint8_t Master_Buf[BUFFER_SIZE];
uint8_t master_test[3];

/************************** PRIVATE FUNCTIONS *************************/
void print_menu(void);
void Buffer_Init(uint8_t type);

/*-------------------------PRIVATE FUNCTIONS-----------------------------*/
/*********************************************************************//**
 * @brief		Print Welcome menu
 * @param[in]	none
 * @return 		None
 **********************************************************************/
void print_menu(void)
{
	_DBG_(menu1);
}

/*********************************************************************//**
 * @brief		Initialize buffer
 * @param[in]	type:
 * 				- 0: Initialize Master_Buf with 0
 * 					Fill all member in Slave_Buf with 0
 * 				- 1: Initialize Slave_Buf with increment value from 0
 * 					Fill all member in Master_Buf with 0
 * @return 		None
 **********************************************************************/
void Buffer_Init(uint8_t type)
{
	uint8_t i;

	if (type)
	{
		for (i = 0; i < BUFFER_SIZE; i++) {
			Master_Buf[i] = i;
		}
	}
	else
	{
		for (i = 0; i < BUFFER_SIZE; i++) {
			Master_Buf[i] = 0;
		}
	}
}

unsigned char ReadHitexPCA9502(void)
{
	I2C_M_SETUP_Type transferMCfg;
	unsigned char input[2];

	// Read input state
	master_test[0] = 0x0B << 3; // Read IOState

	transferMCfg.sl_addr7bit = 0x9A >> 1; // A0&A1 are grounded = 0x9A
	transferMCfg.tx_data = master_test ;
	transferMCfg.tx_length = 1;
	transferMCfg.rx_data = input;
	transferMCfg.rx_length = 1;
	transferMCfg.retransmissions_max = 3;
	I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);

	return input[0];
}

void SetHitexLEDs(unsigned char leds)
{
	I2C_M_SETUP_Type transferMCfg;
	unsigned char led_state;

	led_state = 0;
	if(leds&1) {
		led_state |= 8;
	}
	if(leds&2) {
		led_state |= 4;
	}
	if(leds&4) {
		led_state |= 2;
	}
	if(leds&8) {
		led_state |= 1;
	}


	// Read input state
	master_test[0] = 0x0B << 3; // Write IOState
	master_test[1] = ~led_state;

	transferMCfg.sl_addr7bit = 0x9A >> 1; // A0&A1 are grounded = 0x9A
	transferMCfg.tx_data = master_test ;
	transferMCfg.tx_length = 2;
	transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
	transferMCfg.retransmissions_max = 3;


	LPC_GPIO_PIN_INT->CIENR |= 1; // Clear interrupt on high level
	I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);
	LPC_GPIO_PIN_INT->SIENR |= 1; // Set interrupt on high level
}

volatile unsigned char button_state;

void GPIO0_IRQHandler(void)
{
	unsigned char t;

	// Read buttons and clear interrupt
	t = ReadHitexPCA9502() >> 4;

	button_state = 0;
	if(t&1) {
		button_state |= 8;
	}
	if(t&2) {
		button_state |= 4;
	}
	if(t&4) {
		button_state |= 2;
	}
	if(t&8) {
		button_state |= 1;
	}
}

void InitHitexPCA9502(void)
{
	I2C_M_SETUP_Type transferMCfg;

	// Write to PCA9502 and set up pin mode
	master_test[0] = 0x0A << 3; // Write IODir
	master_test[1] = 0x0F; // Set GPIO0-GPIO3 as output

	transferMCfg.sl_addr7bit = 0x9A >> 1; // A0&A1 are grounded = 0x9A
	transferMCfg.tx_data = master_test ;
	transferMCfg.tx_length = 2;
	transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
	transferMCfg.retransmissions_max = 3;
	I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);

	// Write to PCA9502 and set up interrupt
	master_test[0] = 0x0C << 3; // Write IOIntEna
	master_test[1] = 0xF0; // Set GPIO0-GPIO3 as interrupts
	I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);

	// Write to PCA9502 and set up interrupt latch
	master_test[0] = 0x0E << 3; // Write IOControl
	master_test[1] = 1<<0; // IOLatch
	I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);

	// Set interrupt pinmux (PA_1 / GPIO4[8])
	scu_pinmux(0xA,1,MD_PLN | MD_ZI | MD_EZI, FUNC0); // set up as GPIO
	GPIO_SetDir(4, 8, 0);			 // set as input

	LPC_SCU->PINTSEL0 = 4<<5 | 8<<0;	// Select GPIO pin 8, GPIO port 4 for pin interrupt 0

	LPC_GPIO_PIN_INT->ISEL |= 1; // Set interrupt 0 to be level sensitive
	LPC_GPIO_PIN_INT->SIENR |= 1; // Set interrupt on high level

	NVIC_EnableIRQ(PIN_INT0_IRQn);
	NVIC_SetPriority(PIN_INT0_IRQn, 7);
//	GPIO_ClearValue(4, 1<<2);
}

#define BLINK_MODULO 4

void SysTick_Handler(void)
{
	static uint32_t BlinkModulo;
	static uint32_t LEDon;

	if(BlinkModulo >= BLINK_MODULO)
	{
		if(LEDon) {
			GPIO_ClearValue(LED1_PORT,(1<<LED1_BIT));
			LEDon = 0;
		} else {
			GPIO_SetValue(LED1_PORT,(1<<LED1_BIT));
			LEDon = 1;
		}
		BlinkModulo = 0;
	} else {
		BlinkModulo++;
	}
}

/*-------------------------MAIN FUNCTION------------------------------*/
/*********************************************************************//**
 * @brief		Main program body
 * @param[in]	None
 * @return 		int
 **********************************************************************/
int c_entry(void)
{
	uint32_t Clk;
	unsigned char last_button_state;

	// Configure GPIO on USB LED for heartbeat
	scu_pinmux(0x8 ,1 , MD_PDN, FUNC0); 	// P8.1 : USB0_IND1 LED
	GPIO_SetDir(LED1_PORT,(1<<LED1_BIT), 1);

	SystemInit();
	CGU_Init();

	Clk = CGU_GetPCLKFrequency(CGU_PERIPHERAL_M4CORE);

	if(SysTick_Config(Clk / 20)) { // Set 20 Hz timer interrupt
		// Looks like reload value is too high (24 bit counter)
		while(1);
	}


	/* Initialize debug via UART1
	 * � 115200bps
	 * � 8 data bit
	 * � No parity
	 * � 1 stop bit
	 * � No flow control
	 */
	debug_frmwrk_init();

	print_menu();

	/* Reset UDA1380 on board Hitex */
	scu_pinmux(8,2,MD_PUP, FUNC0);
	GPIO_SetDir(4, 1<<2, 1);
	GPIO_ClearValue(4, 1<<2);

	/* I2C block ------------------------------------------------------------------- */
	// Initialize Slave I2C peripheral
	I2C_Init(I2CDEV_M, 100000);
	/* Enable Slave I2C operation */
	I2C_Cmd(I2CDEV_M, ENABLE);

	/* Transmit -------------------------------------------------------- */
	_DBG_("Press the touch buttons  to light the LEDs");
	InitHitexPCA9502();

	while(1)
	{
		if(button_state != last_button_state)
		{
			last_button_state = button_state;
			SetHitexLEDs(last_button_state);
		}
		__WFI();
	}

}

/* With ARM and GHS toolsets, the entry point is main() - this will
   allow the linker to generate wrapper code to setup stacks, allocate
   heap area, and initialize and copy code and data segments. For GNU
   toolsets, the entry point is through __start() in the crt0_gnu.asm
   file, and that startup code will setup stacks and data */
int main(void)
{
    return c_entry();
}


#ifdef  DEBUG
/*******************************************************************************
* @brief		Reports the name of the source file and the source line number
* 				where the CHECK_PARAM error has occurred.
* @param[in]	file Pointer to the source file name
* @param[in]    line assert_param error line source number
* @return		None
*******************************************************************************/
void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}
#endif

/**
 * @}
 */
