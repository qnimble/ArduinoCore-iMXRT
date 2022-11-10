/*
 * quarto_init.cpp
 *
 *  Created on: May 25, 2022
 *      Author: bluey
 */

#if defined(ARDUINO_QUARTO)
#include "quarto_init.h"
#include "comm.h"
#include "adc.h"
#include "pins_arduino.h"
#include "core_pins.h"

FLASHMEM void quarto_init(void) {
	//Clear stale Data if available.
	READDATA_ACK_BANK_TOGGLE = READDATA_ACK_PIN; // Ack Read Data
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC Data

	uint16_t cal_data; // Load DAC calibration data
	for(int i =0; i<4; i++) {
		cal_data = readNVM(769*128 + i*2);
		setWriteAddress(0x50 + i);
		writeData(cal_data);
	}

	// Load ADC calibration data
	for(int i = 0; i<8; i++) {
		cal_data = readNVM(769*128 + (i+4)*2);
		setWriteAddress(0x60 + i);
		writeData(cal_data);
	}

	cal_data = readData(0x010) ; //read current analog settings
	cal_data |= 0x03; //enable Analog Power if not already on
	setWriteAddress(0x010); //Set Write address to 0x010 for Enabling Analog
	writeData(cal_data); //Enable Analog Clock, Analog,

	GPIO2_DR_TOGGLE = (0x000F0000 + 0x1000); //Do an FPGA read, seems to fix off by one on future reads
	delay(50);

	GPIO2_DR_TOGGLE = 0x00010000; //Set DAC1 to 0x0000 or 0V

	//Clear stale Data if available.
	READDATA_ACK_BANK_TOGGLE = READDATA_ACK_PIN; // Ack Read Data
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC Data

	GPIO2_DR_TOGGLE = 0x00030000; //Set DAC2 to 0x0000 or 0V

	//Clear stale Data if available.
	READDATA_ACK_BANK_TOGGLE = READDATA_ACK_PIN; // Ack Read Data
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC Data

	GPIO2_DR_TOGGLE = 0x00050000; //Set DAC3 to 0x0000 or 0V

	//Clear stale Data if available.
	READDATA_ACK_BANK_TOGGLE = READDATA_ACK_PIN; // Ack Read Data
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC Data

	GPIO2_DR_TOGGLE = 0x00070000; //Set DAC4 to 0x0000 or 0V

	//Clear stale Data if available.
	READDATA_ACK_BANK_TOGGLE = READDATA_ACK_PIN; // Ack Read Data
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC DataGPIO6_DR_TOGGLE = 0x00000020; // Toggle Read Data ACK

	delay(1);

	setWriteAddress(0x010); //Set Write address to 0x010 for Enabling Analog
	writeData(0x07); //Enable Vref.

	//Clear stale Data if available.
	READDATA_ACK_BANK_TOGGLE = READDATA_ACK_PIN; // Ack Read Data
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC Data

	// Configure ADC Interrupts to basic ISRs that ack data to keep stream going
	__asm volatile ("cpsid i");

	configureADC1(0,0,BIPOLAR_10V,&adc1_irq_ignoredata);
	configureADC2(0,0,BIPOLAR_10V,&adc2_irq_ignoredata);
	configureADC3(0,0,BIPOLAR_10V,&adc3_irq_ignoredata);
	configureADC4(0,0,BIPOLAR_10V,&adc4_irq_ignoredata);

	__asm volatile ("cpsie i");
}
#endif
