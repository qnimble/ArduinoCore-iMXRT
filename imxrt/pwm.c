#include "imxrt.h"
#include "core_pins.h"
#include "debug/printf.h"


struct pwm_pin_info_struct {
	uint8_t type;    // 0=no pwm, 1=flexpwm, 2=quad
	uint8_t module;  // 0-3, 0-3
	uint8_t channel; // 0=X, 1=A, 2=B
	uint8_t muxval;  //
};

uint8_t analog_write_res = 8;

#define M(a, b) ((((a) - 1) << 4) | (b))

#if defined(__IMXRT1062__)

const struct pwm_pin_info_struct pwm_pin_info[] = {
#if defined(ARDUINO_QUARTO)
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{1, M(1, 2), 0, 4},  // FlexPWM1_2_X   5  // AD_B0_12
	{1, M(1, 3), 0, 4},  // FlexPWM1_3_X   6  // AD_B0_13
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{1, M(1, 3), 1, 2},  // FlexPWM1_3_A  9 // LED Red    // SD_B1_00
	{1, M(1, 3), 2, 2},  // FlexPWM1_3_B  10 // LED Blue   // SD_B1_01
	{1, M(2, 3), 1, 2},  // FlexPWM2_3_A  11 // LED Green  // SD_B1_02
	{1, M(2, 3), 2, 0},  // FlexPWM2_3_B  12 // Trigger1   // AD_B0_01
	{1, M(2, 3), 1, 0},  // FlexPWM2_3_A  13 // Trigger2   // AD_B0_00
#else
	{1, M(1, 1), 0, 4},  // FlexPWM1_1_X   0  // AD_B0_03
	{1, M(1, 0), 0, 4},  // FlexPWM1_0_X   1  // AD_B0_02
	{1, M(4, 2), 1, 1},  // FlexPWM4_2_A   2  // EMC_04
	{1, M(4, 2), 2, 1},  // FlexPWM4_2_B   3  // EMC_05
	{1, M(2, 0), 1, 1},  // FlexPWM2_0_A   4  // EMC_06
	{1, M(2, 1), 1, 1},  // FlexPWM2_1_A   5  // EMC_08
	{1, M(2, 2), 1, 2},  // FlexPWM2_2_A   6  // B0_10
	{1, M(1, 3), 2, 6},  // FlexPWM1_3_B   7  // B1_01
	{1, M(1, 3), 1, 6},  // FlexPWM1_3_A   8  // B1_00
	{1, M(2, 2), 2, 2},  // FlexPWM2_2_B   9  // B0_11
	{2, M(1, 0), 0, 1},  // QuadTimer1_0  10  // B0_00
	{2, M(1, 2), 0, 1},  // QuadTimer1_2  11  // B0_02
	{2, M(1, 1), 0, 1},  // QuadTimer1_1  12  // B0_01
	{2, M(2, 0), 0, 1},  // QuadTimer2_0  13  // B0_03
	{2, M(3, 2), 0, 1},  // QuadTimer3_2  14  // AD_B1_02
	{2, M(3, 3), 0, 1},  // QuadTimer3_3  15  // AD_B1_03
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{2, M(3, 1), 0, 1},  // QuadTimer3_1  18  // AD_B1_01
	{2, M(3, 0), 0, 1},  // QuadTimer3_0  19  // AD_B1_00
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{1, M(4, 0), 1, 1},  // FlexPWM4_0_A  22  // AD_B1_08
	{1, M(4, 1), 1, 1},  // FlexPWM4_1_A  23  // AD_B1_09
	{1, M(1, 2), 0, 4},  // FlexPWM1_2_X  24  // AD_B0_12
	{1, M(1, 3), 0, 4},  // FlexPWM1_3_X  25  // AD_B0_13
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{1, M(3, 1), 2, 1},  // FlexPWM3_1_B  28  // EMC_32
	{1, M(3, 1), 1, 1},  // FlexPWM3_1_A  29  // EMC_31
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{1, M(2, 0), 2, 1},  // FlexPWM2_0_B  33  // EMC_07
#ifdef ARDUINO_TEENSY40
	{1, M(1, 1), 2, 1},  // FlexPWM1_1_B  34  // SD_B0_03
	{1, M(1, 1), 1, 1},  // FlexPWM1_1_A  35  // SD_B0_02
	{1, M(1, 0), 2, 1},  // FlexPWM1_0_B  36  // SD_B0_01
	{1, M(1, 0), 1, 1},  // FlexPWM1_0_A  37  // SD_B0_00
	{1, M(1, 2), 2, 1},  // FlexPWM1_2_B  38  // SD_B0_05
	{1, M(1, 2), 1, 1},  // FlexPWM1_2_A  39  // SD_B0_04
#endif
#ifdef ARDUINO_TEENSY41
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{1, M(2, 3), 1, 6},  // FlexPWM2_3_A  36  // B1_00
	{1, M(2, 3), 2, 6},  // FlexPWM2_3_B  37  // B1_01
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{0, M(1, 0), 0, 0},
	{1, M(1, 1), 2, 1},  // FlexPWM1_1_B  42  // SD_B0_03
	{1, M(1, 1), 1, 1},  // FlexPWM1_1_A  43  // SD_B0_02
	{1, M(1, 0), 2, 1},  // FlexPWM1_0_B  44  // SD_B0_01
	{1, M(1, 0), 1, 1},  // FlexPWM1_0_A  45  // SD_B0_00
	{1, M(1, 2), 2, 1},  // FlexPWM1_2_B  46  // SD_B0_05
	{1, M(1, 2), 1, 1},  // FlexPWM1_2_A  47  // SD_B0_04
	{0, M(1, 0), 0, 0},  // duplicate FlexPWM1_0_B
	{0, M(1, 0), 0, 0},  // duplicate FlexPWM1_2_A
	{0, M(1, 0), 0, 0},  // duplicate FlexPWM1_2_B
	{1, M(3, 3), 2, 1},  // FlexPWM3_3_B  51  // EMC_22
	{0, M(1, 0), 0, 0},  // duplicate FlexPWM1_1_B
	{0, M(1, 0), 0, 0},  // duplicate FlexPWM1_1_A
	{1, M(3, 0), 1, 1},  // FlexPWM3_0_A  54  // EMC_29
#endif
#ifdef ARDUINO_TEENSY_MICROMOD
	{1, M(1, 1), 2, 1},  // FlexPWM1_1_B  34  // SD_B0_03
	{1, M(1, 1), 1, 1},  // FlexPWM1_1_A  35  // SD_B0_02
	{1, M(1, 0), 2, 1},  // FlexPWM1_0_B  36  // SD_B0_01
	{1, M(1, 0), 1, 1},  // FlexPWM1_0_A  37  // SD_B0_00
	{1, M(1, 2), 1, 1},  // FlexPWM1_2_A  38  // SD_B0_04
	{1, M(1, 2), 2, 1},  // FlexPWM1_2_B  39  // SD_B0_05
	{2, M(2, 1), 0, 1},  // QuadTimer2_1  40  // B0_04
	{2, M(2, 2), 0, 1},  // QuadTimer2_2  41  // B0_05
	{0, M(1, 0), 0, 0},  // duplicate QuadTimer3_0
	{0, M(1, 0), 0, 0},  // duplicate QuadTimer3_1
	{0, M(1, 0), 0, 0},  // duplicate QuadTimer3_2
	{2, M(4, 0), 0, 1},  // QuadTimer4_0  45  // B0_09
#endif
#endif
};

// Known usage of FlexPWM and QuadTimers
// -------------------------------------
//   FlexPWM1_0    PWM pin 1, 36(T4.0), 37(T4.0), 44(T4.1), 45(T4.1)
//   FlexPWM1_1    PWM pin 0, 34(T4.0), 35(T4.0), 42(T4.1), 43(T4.1)
//   FlexPWM1_2    PWM pin 24, 38(T4.0), 39(T4.0), 46(T4.1), 47(T4.1)
//   FlexPWM1_3    PWM pin 7, 8, 25
//   FlexPWM2_0    PWM pin 4, 33
//   FlexPWM2_1    PWM pin 5, Tlc5940 library
//   FlexPWM2_2    PWM pin 6, 9
//   FlexPWM2_3    PWM pin 36(T4.1), 37(T4.1)
//   FlexPWM3_0    PWM pin 53(T4.1)
//   FlexPWM3_1    PWM pin 28, 29
//   FlexPWM3_2
//   FlexPWM3_3    PWM pin 51(T4.1)
//   FlexPWM4_0    PWM pin 22
//   FlexPWM4_1    PWM pin 23
//   FlexPWM4_2    PWM pin 2, 3, Tlc5940 library
//   FlexPWM4_3
//   QuadTimer1_0  PWM pin 10
//   QuadTimer1_1  PWM pin 12
//   QuadTimer1_2  PWM pin 11
//   QuadTimer1_3
//   QuadTimer2_0  PWM pin 13
//   QuadTimer2_1
//   QuadTimer2_2
//   QuadTimer2_3
//   QuadTimer3_0  PWM pin 19
//   QuadTimer3_1  PWM pin 18
//   QuadTimer3_2  PWM pin 14
//   QuadTimer3_3  PWM pin 15
//   QuadTimer4_0  OctoWS2811, ADC library
//   QuadTimer4_1  OctoWS2811
//   QuadTimer4_2  OctoWS2811
//   QuadTimer4_3  AudioInputAnalog, ADC library


#endif // __IMXRT1062__

void flexpwmWrite(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel, uint16_t val)
{
	uint16_t mask = 1 << submodule;
	uint32_t modulo = p->SM[submodule].VAL1;
	uint32_t cval = ((uint32_t)val * (modulo + 1)) >> analog_write_res;
	if (cval > modulo) cval = modulo; // TODO: is this check correct?

	//printf("flexpwmWrite, p=%08lX, sm=%d, ch=%c, cval=%ld\n",
		//(uint32_t)p, submodule, channel == 0 ? 'X' : (channel == 1 ? 'A' : 'B'), cval);
	p->MCTRL |= FLEXPWM_MCTRL_CLDOK(mask);
	switch (channel) {
	  case 0: // X
		p->SM[submodule].VAL0 = modulo - cval;
		p->OUTEN |= FLEXPWM_OUTEN_PWMX_EN(mask);
		//printf(" write channel X\n");
		break;
	  case 1: // A
		p->SM[submodule].VAL3 = cval;
		p->OUTEN |= FLEXPWM_OUTEN_PWMA_EN(mask);
		//printf(" write channel A\n");
		break;
	  case 2: // B
		p->SM[submodule].VAL5 = cval;
		p->OUTEN |= FLEXPWM_OUTEN_PWMB_EN(mask);
		//printf(" write channel B\n");
	}
	p->MCTRL |= FLEXPWM_MCTRL_LDOK(mask);
}

void flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel, float frequency)
{
	uint16_t mask = 1 << submodule;
	uint32_t olddiv = p->SM[submodule].VAL1;
	uint32_t newdiv = (uint32_t)((float)F_BUS_ACTUAL / frequency + 0.5f);
	uint32_t prescale = 0;
	//printf(" div=%lu\n", newdiv);
	while (newdiv > 65535 && prescale < 7) {
		newdiv = newdiv >> 1;
		prescale = prescale + 1;
	}
	if (newdiv > 65535) {
		newdiv = 65535;
	} else if (newdiv < 2) {
		newdiv = 2;
	}
	//printf(" div=%lu, scale=%lu\n", newdiv, prescale);
	p->MCTRL |= FLEXPWM_MCTRL_CLDOK(mask);
	p->SM[submodule].CTRL = FLEXPWM_SMCTRL_FULL | FLEXPWM_SMCTRL_PRSC(prescale);
	p->SM[submodule].VAL1 = newdiv - 1;
	p->SM[submodule].VAL0 = (p->SM[submodule].VAL0 * newdiv) / olddiv;
	p->SM[submodule].VAL3 = (p->SM[submodule].VAL3 * newdiv) / olddiv;
	p->SM[submodule].VAL5 = (p->SM[submodule].VAL5 * newdiv) / olddiv;
	p->MCTRL |= FLEXPWM_MCTRL_LDOK(mask);
}

int readflexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel)
{
	uint32_t divider = p->SM[submodule].VAL1 + 1;
	uint8_t prescale = 	 (p->SM[submodule].CTRL & FLEXPWM_SMCTRL_PRSC(15))>>4;
	return divider<<prescale;
}


void quadtimerWrite(IMXRT_TMR_t *p, unsigned int submodule, uint16_t val)
{
	uint32_t modulo = 65537 - p->CH[submodule].LOAD + p->CH[submodule].CMPLD1;
	uint32_t high = ((uint32_t)val * (modulo - 1)) >> analog_write_res;
	if (high >= modulo - 1) high = modulo - 2;

	//printf(" modulo=%lu\n", modulo);
	//printf(" high=%lu\n", high);
	uint32_t low = modulo - high; // low must 2 or higher
	//printf(" low=%lu\n", low);

	p->CH[submodule].LOAD = 65537 - low;
	p->CH[submodule].CMPLD1 = high;
}

void quadtimerFrequency(IMXRT_TMR_t *p, unsigned int submodule, float frequency)
{
	uint32_t newdiv = (uint32_t)((float)F_BUS_ACTUAL / frequency + 0.5f);
	uint32_t prescale = 0;
	//printf(" div=%lu\n", newdiv);
	while (newdiv > 65534 && prescale < 7) {
		newdiv = newdiv >> 1; 
		prescale = prescale + 1;
	}
	if (newdiv > 65534) {
		newdiv = 65534;
	} else if (newdiv < 2) {
		newdiv = 2;
	}
	//printf(" div=%lu, scale=%lu\n", newdiv, prescale);
	uint32_t oldhigh = p->CH[submodule].CMPLD1;
	uint32_t oldlow = 65537 - p->CH[submodule].LOAD;
	uint32_t high = (oldhigh * newdiv) / (oldhigh + oldlow);
	// TODO: low must never be less than 2 - can it happen with this?
	uint32_t low = newdiv - high;
	//printf(" high=%lu, low=%lu\n", high, low);
	p->CH[submodule].LOAD = 65537 - low;
	p->CH[submodule].CMPLD1 = high;
	p->CH[submodule].CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(8 + prescale) |
		TMR_CTRL_LENGTH | TMR_CTRL_OUTMODE(6);
}


int readquadtimerFrequency(IMXRT_TMR_t *p, unsigned int submodule)
{
	uint32_t high = p->CH[submodule].CMPLD1;
	uint32_t low = 65537 - p->CH[submodule].LOAD;
	uint32_t divider = low + high;
	uint8_t prescale = 	( (p->CH[submodule].CTRL & TMR_CTRL_PCS(15))>>9) - 8;
	return divider<<prescale;
}


void analogWrite(uint8_t pin, int val)
{
	const struct pwm_pin_info_struct *info;

	if (pin >= CORE_NUM_DIGITAL) return;
	//printf("analogWrite, pin %d, val %d\n", pin, val);
	info = pwm_pin_info + pin;
	if (info->type == 1) {
		// FlexPWM pin
		IMXRT_FLEXPWM_t *flexpwm;
		switch ((info->module >> 4) & 3) {
		  case 0: flexpwm = &IMXRT_FLEXPWM1; break;
		  case 1: flexpwm = &IMXRT_FLEXPWM2; break;
		  case 2: flexpwm = &IMXRT_FLEXPWM3; break;
		  default: flexpwm = &IMXRT_FLEXPWM4;
		}
		flexpwmWrite(flexpwm, info->module & 0x03, info->channel, val);
	} else if (info->type == 2) {
		// QuadTimer pin
		IMXRT_TMR_t *qtimer;
		switch ((info->module >> 4) & 3) {
		  case 0: qtimer = &IMXRT_TMR1; break;
		  case 1: qtimer = &IMXRT_TMR2; break;
		  case 2: qtimer = &IMXRT_TMR3; break;
		  default: qtimer = &IMXRT_TMR4;
		}
		quadtimerWrite(qtimer, info->module & 0x03, val);
	} else {
		return;
	}
	*(portConfigRegister(pin)) = info->muxval;
	// TODO: pad config register
}

void analogWriteFrequency(uint8_t pin, float frequency)
{
	const struct pwm_pin_info_struct *info;

	if (pin >= CORE_NUM_DIGITAL) return;
	//printf("analogWriteFrequency, pin %d, freq %d\n", pin, (int)frequency);
	info = pwm_pin_info + pin;
	if (info->type == 1) {
		// FlexPWM pin
		IMXRT_FLEXPWM_t *flexpwm;
		switch ((info->module >> 4) & 3) {
		  case 0: flexpwm = &IMXRT_FLEXPWM1; break;
		  case 1: flexpwm = &IMXRT_FLEXPWM2; break;
		  case 2: flexpwm = &IMXRT_FLEXPWM3; break;
		  default: flexpwm = &IMXRT_FLEXPWM4;
		}
		flexpwmFrequency(flexpwm, info->module & 0x03, info->channel, frequency);
	} else if (info->type == 2) {
		// QuadTimer pin
		IMXRT_TMR_t *qtimer;
		switch ((info->module >> 4) & 3) {
		  case 0: qtimer = &IMXRT_TMR1; break;
		  case 1: qtimer = &IMXRT_TMR2; break;
		  case 2: qtimer = &IMXRT_TMR3; break;
		  default: qtimer = &IMXRT_TMR4;
		}
		quadtimerFrequency(qtimer, info->module & 0x03, frequency);
	}
}

float readAnalogWriteFrequency(uint8_t pin) {
	const struct pwm_pin_info_struct *info;
	uint32_t divider = 1;
	if (pin >= CORE_NUM_DIGITAL) return 0 ;

	info = pwm_pin_info + pin;
	if (info->type == 1) {
		// FlexPWM pin
		IMXRT_FLEXPWM_t *flexpwm;
		switch ((info->module >> 4) & 3) {
		  case 0: flexpwm = &IMXRT_FLEXPWM1; break;
		  case 1: flexpwm = &IMXRT_FLEXPWM2; break;
		  case 2: flexpwm = &IMXRT_FLEXPWM3; break;
		  default: flexpwm = &IMXRT_FLEXPWM4;
		}
		divider =  readflexpwmFrequency(flexpwm, info->module & 0x03, info->channel);
	} else if (info->type == 2) {
		// QuadTimer pin
		IMXRT_TMR_t *qtimer;
		switch ((info->module >> 4) & 3) {
		  case 0: qtimer = &IMXRT_TMR1; break;
		  case 1: qtimer = &IMXRT_TMR2; break;
		  case 2: qtimer = &IMXRT_TMR3; break;
		  default: qtimer = &IMXRT_TMR4;
		}
		divider = readquadtimerFrequency(qtimer, info->module & 0x03);
	}
	return ((float)F_BUS_ACTUAL / divider);

}

void flexpwm_init(IMXRT_FLEXPWM_t *p)
{
	int i;

	p->FCTRL0 = FLEXPWM_FCTRL0_FLVL(15); // logic high = fault
	p->FSTS0 = 0x000F; // clear fault status
	p->FFILT0 = 0;
	p->MCTRL |= FLEXPWM_MCTRL_CLDOK(15);
	for (i=0; i < 4; i++) {
		p->SM[i].CTRL2 = FLEXPWM_SMCTRL2_INDEP | FLEXPWM_SMCTRL2_WAITEN
			| FLEXPWM_SMCTRL2_DBGEN;
		p->SM[i].CTRL = FLEXPWM_SMCTRL_FULL;
		p->SM[i].OCTRL = 0;
		p->SM[i].DTCNT0 = 0;
		p->SM[i].INIT = 0;
		p->SM[i].VAL0 = 0;
		p->SM[i].VAL1 = 33464;
		p->SM[i].VAL2 = 0;
		p->SM[i].VAL3 = 0;
		p->SM[i].VAL4 = 0;
		p->SM[i].VAL5 = 0;
	}
	p->MCTRL |= FLEXPWM_MCTRL_LDOK(15);
	p->MCTRL |= FLEXPWM_MCTRL_RUN(15);
}

void quadtimer_init(IMXRT_TMR_t *p)
{
	int i;

	for (i=0; i < 4; i++) {
		p->CH[i].CTRL = 0; // stop timer
		p->CH[i].CNTR = 0;
		p->CH[i].SCTRL = TMR_SCTRL_OEN | TMR_SCTRL_OPS | TMR_SCTRL_VAL | TMR_SCTRL_FORCE;
		p->CH[i].CSCTRL = TMR_CSCTRL_CL1(1) | TMR_CSCTRL_ALT_LOAD;
		// COMP must be less than LOAD - otherwise output is always low
		p->CH[i].LOAD = 24000;   // low time  (65537 - x) - 
		p->CH[i].COMP1 = 0;  // high time (0 = always low, max = LOAD-1)
		p->CH[i].CMPLD1 = 0;
		p->CH[i].CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(8) |
			TMR_CTRL_LENGTH | TMR_CTRL_OUTMODE(6);
	}
}

void pwm_init(void)
{
	//printf("pwm init\n");
	CCM_CCGR4 |= CCM_CCGR4_PWM1(CCM_CCGR_ON) | CCM_CCGR4_PWM2(CCM_CCGR_ON) |
		CCM_CCGR4_PWM3(CCM_CCGR_ON) | CCM_CCGR4_PWM4(CCM_CCGR_ON);
	CCM_CCGR6 |= CCM_CCGR6_QTIMER1(CCM_CCGR_ON) | CCM_CCGR6_QTIMER2(CCM_CCGR_ON) |
		CCM_CCGR6_QTIMER3(CCM_CCGR_ON) | CCM_CCGR6_QTIMER4(CCM_CCGR_ON);
	flexpwm_init(&IMXRT_FLEXPWM1);
	flexpwm_init(&IMXRT_FLEXPWM2);
	flexpwm_init(&IMXRT_FLEXPWM3);
	flexpwm_init(&IMXRT_FLEXPWM4);
	quadtimer_init(&IMXRT_TMR1);
	quadtimer_init(&IMXRT_TMR2);
	quadtimer_init(&IMXRT_TMR3);
}



void xbar_connect(unsigned int input, unsigned int output)
{
	if (input >= 88) return;
	if (output >= 132) return;
#if 1
	volatile uint16_t *xbar = &XBARA1_SEL0 + (output / 2);
	uint16_t val = *xbar;
	if (!(output & 1)) {
		val = (val & 0xFF00) | input;
	} else {
		val = (val & 0x00FF) | (input << 8);
	}
	*xbar = val;
#else
	// does not work, seems 8 bit access is not allowed
	volatile uint8_t *xbar = (volatile uint8_t *)XBARA1_SEL0;
	xbar[output] = input;
#endif
}

uint32_t analogWriteRes(uint32_t bits)
{
	uint32_t prior;
	if (bits < 1) {
		bits = 1;
	} else if (bits > 16) {
		bits = 16;
	}
	prior = analog_write_res;
	analog_write_res = bits;
	return prior;
}


