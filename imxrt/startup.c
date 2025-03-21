#include "imxrt.h"
#include "wiring.h"
#include "usb_dev.h"
#include "avr/pgmspace.h"
#include "smalloc.h"
#include <string.h>

#if defined(ARDUINO_QUARTO)

#include "quarto_init.h"
#include "quarto_wdog.h"
#endif

#include "debug/printf.h"
// from the linker
extern unsigned long _stextload;
extern unsigned long _stext;
extern unsigned long _etext;
extern unsigned long _sdataload;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long _flexram_bank_config;
extern unsigned long _estack;
extern unsigned long _extram_start;
extern unsigned long _extram_end;

__attribute__ ((used, aligned(1024), section(".vectorsram")))
void (* volatile _VectorsRam[NVIC_NUM_INTERRUPTS+16])(void);

#ifndef PRINT_DEBUG_STUFF
int printf(const char *test,...) {
  return 1;
}
#endif


void data_init(unsigned int romstart, unsigned int start, unsigned int len);
void bss_init(unsigned int start, unsigned int len);
void approxMSdelay(unsigned int ms);
void foreverLoopAndToggle(void);

void configure_systick(void);
static void reset_PFD();
extern void systick_isr(void);
extern void pendablesrvreq_isr(void);
FLASHMEM void USB_SD_PowerOff(void);
void configure_cache(void);
void configure_external_ram(void);
void unused_interrupt_vector(void);
void usb_pll_start();
extern void analog_init(void); // analog.c
extern void pwm_init(void); // pwm.c
extern void tempmon_init(void);  //tempmon.c
extern float tempmonGetTemp(void);
extern unsigned long rtc_get(void);
uint32_t set_arm_clock(uint32_t frequency); // clockspeed.c
extern void __libc_init_array(void); // C++ standard library

uint8_t external_psram_size = 0;
#ifdef ARDUINO_TEENSY41
struct smalloc_pool extmem_smalloc_pool;
#endif

void init_memory(void);
void init_nvic(void);
void configure_pins(void);
extern unsigned int __data_section_table;
extern unsigned int __data_section_table_end;
extern unsigned int __bss_section_table;
extern unsigned int __bss_section_table_end;


extern uint32_t* keep_trick;

extern int main (void);
FLASHMEM void startup_default_early_hook(void) {}
void startup_early_hook(void)	__attribute__ ((weak, alias("startup_default_early_hook")));
FLASHMEM void startup_default_middle_hook(void) {}
void startup_middle_hook(void)	__attribute__ ((weak, alias("startup_default_middle_hook")));
FLASHMEM void startup_default_late_hook(void) {}
void startup_late_hook(void)	__attribute__ ((weak, alias("startup_default_late_hook")));
__attribute__((section(".startup"), optimize("no-tree-loop-distribute-patterns")))
void ResetHandler(void)
{
	// Disable interrupts
	__asm volatile ("cpsid i");
	SCB_MPU_CTRL = 0; // turn off MPU
	SYST_CSR = 0; // Disable SysTick at boot
	WDOG3_CNT = 0xB480A602; //Feed wdog3

#if defined(__IMXRT1062__)
	IOMUXC_GPR_GPR17 = (uint32_t)&_flexram_bank_config;
	IOMUXC_GPR_GPR14 = 0x00AA0000;
	IOMUXC_GPR_GPR16 = 0x00200007;
	__asm__ volatile("mov sp, %0" : : "r" ((uint32_t)&_estack) : );
	__asm__ volatile("dsb":::"memory");
	__asm__ volatile("isb":::"memory");
#endif
	startup_early_hook(); // must be in FLASHMEM, as ITCM is not yet initialized!
	PMU_MISC0_SET = 1<<3; //Use bandgap-based bias currents for best performance (Page 1175)
	// pin 13 - if startup crashes, use this to turn on the LED early for troubleshooting
	//IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_03 = 5;
	//IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_03 = IOMUXC_PAD_DSE(7);
	//IOMUXC_GPR_GPR27 = 0xFFFFFFFF;
	//GPIO7_GDIR |= (1<<3);
	//GPIO7_DR_SET = (1<<3); // digitalWrite(13, HIGH);

	// Initialize memory

	init_memory();
	configure_cache();

#ifdef ARDUINO_QUARTO
	#ifdef PROG_BOOTLOADER //if bootloader, sync clocks
		// initialize RTC
		if (!(SNVS_LPCR & SNVS_LPCR_SRTC_ENV)) {
			// if SRTC isn't running, start it with default Jan 1, 2019
			SNVS_LPSRTCLR = 1546300800u << 15;
			SNVS_LPSRTCMR = 1546300800u >> 17;
			SNVS_LPCR |= SNVS_LPCR_SRTC_ENV;
			while (!(SNVS_LPCR & SNVS_LPCR_SRTC_ENV)); // wait
		}

	// stop the RTC
	  SNVS_HPCR &= ~(SNVS_HPCR_RTC_EN | SNVS_HPCR_HP_TS);
	  while (SNVS_HPCR & SNVS_HPCR_RTC_EN) ; // wait
	  // start the RTC and sync it to the SRTC
	  SNVS_HPCR |= SNVS_HPCR_RTC_EN | SNVS_HPCR_HP_TS;

	#else  //Set if not bootloader (app)
		SRC_GPR5 = CRASHREPORT_APPLICATION_BOOTING; //Reset register on boot so registered on USB reboot can be detected
	#endif

	//Disable LPSPI if previously used
	LPSPI1_CR = 0;
	LPSPI2_CR = 0;
	LPSPI3_CR = 0;
	LPSPI4_CR = 0;

	//Use Wakeup pin as reset to FPGA and set low (reset)
	IOMUXC_SNVS_SW_PAD_CTL_PAD_WAKEUP = 0xB888u;
	IOMUXC_SNVS_SW_MUX_CTL_PAD_WAKEUP = 0x15;

	GPIO5_GDIR |= 0x01;
	GPIO5_DR_CLEAR = 0x01;

	#ifdef QUARTO_PROTOTYPE
		#define PERCLK_SOURCE CCM_CSCMR1_PERCLK_CLK_SEL
		#define PLL_BYPASS_TO_EXTERNAL_LVDS 0
		#define PERCLK_DIVIDER 0
		CCM_ANALOG_MISC1 &= ~CCM_ANALOG_MISC1_LVDSCLK1_IBEN; //Turn off LVDS input
		CCM_ANALOG_MISC1 &= ~0x01F; //clear LVDS CLK_SEL bits (0-4)
		CCM_ANALOG_MISC1 |= 0x12 | CCM_ANALOG_MISC1_LVDSCLK1_OBEN; //Turn on LVDS output, select XTAL as source

	#else
		#define PLL_BYPASS_TO_EXTERNAL_LVDS (1<<14)
		#define PERCLK_SOURCE 0
		#define PERCLK_DIVIDER 1
		CCM_ANALOG_MISC1 &= ~CCM_ANALOG_MISC1_LVDSCLK1_OBEN; //Turn off LVDS output
		CCM_ANALOG_MISC1 |= CCM_ANALOG_MISC1_LVDSCLK1_IBEN; //Turn on LVDS input

	#endif
#else
	#define PLL_BYPASS_TO_EXTERNAL_LVDS 0
	#define PERCLK_SOURCE CCM_CSCMR1_PERCLK_CLK_SEL
	#define PERCLK_DIVIDER 0
#endif
	// enable FPU
	SCB_CPACR = 0x00F00000;

	// set up blank interrupt & exception vector table
	init_nvic();
	reset_PFD();

	// enable exception handling
	SCB_SHCSR |= SCB_SHCSR_MEMFAULTENA | SCB_SHCSR_BUSFAULTENA | SCB_SHCSR_USGFAULTENA;

	// Configure clocks
	// TODO: make sure all affected peripherals are turned off!
	// PIT & GPT timers to run from 24 MHz clock (independent of CPU speed)
	CCM_CSCMR1 &= ~ ( CCM_CSCMR1_PERCLK_CLK_SEL | CCM_CSCMR1_PERCLK_PODF(0x3F)); //Clear PERCLK_CLK_SEL bit and PODF divider
	CCM_CSCMR1 |= PERCLK_SOURCE | CCM_CSCMR1_PERCLK_PODF(PERCLK_DIVIDER); //Set PERCLK_SOURCE and PERCLK_CLK_SEL based on Hardware
	//CCM_CSCMR1 &= ~CCM_CSCMR1_PERCLK_PODF(0x3F); //Clear PERCLK_PODF dividers
	//CCM_CSCMR1 |=

	// UARTs run from 24 MHz clock (works if PLL3 off or bypassed)
	CCM_CSCDR1 = (CCM_CSCDR1 & ~CCM_CSCDR1_UART_CLK_PODF(0x3F)) | CCM_CSCDR1_UART_CLK_SEL;

#if defined(__IMXRT1062__)
	// Use fast GPIO6, GPIO7, GPIO8, GPIO9
	IOMUXC_GPR_GPR26 = 0xFFFFFFFF;
	#if defined(ARDUINO_QUARTO)
		IOMUXC_GPR_GPR27 = 0xFFF00000; //keep DATA lines on slow GPIO
	#else
		IOMUXC_GPR_GPR27 = 0xFFFFFFFF;
	#endif
	IOMUXC_GPR_GPR28 = 0xFFFFFFFF;
	IOMUXC_GPR_GPR29 = 0xFFFFFFFF;
#endif
#if defined(ARDUINO_QUARTO)
	//Set Wakeup pin high to enable FPGA
	GPIO2_DR = 0; //clear all the write pins
	GPIO2_GDIR = 0xFFFFF; //set GPIO2 as output
	GPIO5_DR_SET = 0x01;
	approxMSdelay(5);
	quarto_wdog_disable(); // turn off wdog to reinit with new values
	quarto_wdog_init(635); // turn on wdog
#endif

	// must enable PRINT_DEBUG_STUFF in debug/print.h
	printf_debug_init();
	printf("\n***********IMXRT Startup**********\n");
	printf("test %d %d %d\n", 1, -1234567, 3);

	//configure_cache();
	configure_systick();
	usb_pll_start();
	reset_PFD(); //TODO: is this really needed?
#ifdef F_CPU
	set_arm_clock(F_CPU);
#endif
#if !defined(PROG_BOOTLOADER) && !defined(INTERNAL_CLOCK) //bootloader should not change PLL SYS state
	if ((CCM_ANALOG_PLL_SYS & (1<<14) ) != PLL_BYPASS_TO_EXTERNAL_LVDS) {
		//CCM_ANALOG_PLL_SYS |= CCM_ANALOG_PLL_SYS_POWERDOWN;
		CCM_ANALOG_PLL_SYS &= ~(1<<14); //14bit is bypass source, clear it
		CCM_ANALOG_PLL_SYS |= PLL_BYPASS_TO_EXTERNAL_LVDS;
		//CCM_ANALOG_PLL_SYS &= ~CCM_ANALOG_PLL_SYS_POWERDOWN; //power back up
		while (!(CCM_ANALOG_PLL_SYS & CCM_ANALOG_PLL_SYS_LOCK)) ; // wait for lock
  }
		CCM_CCR &= ~(CCM_CCR_COSC_EN); //Turn off on-chip oscillator, as not longer needed
#endif

	//Reenable interrupts
	__asm volatile ("cpsie i");

	// Undo PIT timer usage by ROM startup
	CCM_CCGR1 |= CCM_CCGR1_PIT(CCM_CCGR_ON);
	PIT_MCR = 0;
	PIT_TCTRL0 = 0;
	PIT_TCTRL1 = 0;
	PIT_TCTRL2 = 0;
	PIT_TCTRL3 = 0;

#ifdef ARDUINO_TEENSY41
	configure_external_ram();
#endif

	analog_init();
	pwm_init();
	tempmon_init();
	startup_middle_hook();

#if !defined(TEENSY_INIT_USB_DELAY_BEFORE)
        #define TEENSY_INIT_USB_DELAY_BEFORE 20
#endif
#if !defined(TEENSY_INIT_USB_DELAY_AFTER)
        #define TEENSY_INIT_USB_DELAY_AFTER 280
#endif
	// for background about this startup delay, please see these conversations
	// https://forum.pjrc.com/threads/36606?p=113980&viewfull=1#post113980
	// https://forum.pjrc.com/threads/31290?p=87273&viewfull=1#post87273

	while (millis() < TEENSY_INIT_USB_DELAY_BEFORE) ; // wait
	usb_init();
	while (millis() < TEENSY_INIT_USB_DELAY_AFTER + TEENSY_INIT_USB_DELAY_BEFORE) ; // wait
#if defined(ARDUINO_QUARTO)
	configure_pins();
	quarto_init();
#endif

	//printf("before C++ constructors\n");
	startup_late_hook();
	__libc_init_array();
	//printf("after C++ constructors\n");
	//printf("before setup\n");
	main();
	
	while (1) asm("WFI");
}




// ARM SysTick is used for most Ardiuno timing functions, delay(), millis(),
// micros().  SysTick can run from either the ARM core clock, or from an
// "external" clock.  NXP documents it as "24 MHz XTALOSC can be the external
// clock source of SYSTICK" (RT1052 ref manual, rev 1, page 411).  However,
// NXP actually hid an undocumented divide-by-240 circuit in the hardware, so
// the external clock is really 100 kHz.  We use this clock rather than the
// ARM clock, to allow SysTick to maintain correct timing even when we change
// the ARM clock to run at different speeds.
#define SYSTICK_EXT_FREQ 100000

extern volatile uint32_t systick_cycle_count;
void configure_systick(void)
{
	_VectorsRam[14] = pendablesrvreq_isr;
	_VectorsRam[15] = systick_isr;
#if ARDUINO_QUARTO
	//Quarto uses external 24 MHz clock that is not derived from 24 MHz osc, so
	//switch CPU clock instead and set divider based on CPU frequency
	SYST_RVR = (F_CPU_ACTUAL / 1000) - 1;
	SYST_CVR = 0;
	SYST_CSR = SYST_CSR_CLKSOURCE| SYST_CSR_TICKINT | SYST_CSR_ENABLE;
	SCB_SHPR3 = 0x40400000;  // Systick, pendablesrvreq_isr = priority 64;
#else
	SYST_RVR = (SYSTICK_EXT_FREQ / 1000) - 1;
	SYST_CVR = 0;
	SYST_CSR = SYST_CSR_TICKINT | SYST_CSR_ENABLE;
	SCB_SHPR3 = 0x20200000;  // Systick, pendablesrvreq_isr = priority 32;
#endif
	ARM_DEMCR |= ARM_DEMCR_TRCENA;
	ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA; // turn on cycle counter
	systick_cycle_count = ARM_DWT_CYCCNT; // compiled 0, corrected w/1st systick
}


FLASHMEM void init_nvic(void) {
       unsigned int i;
       // set up blank interrupt & exception vector table
       for (i=0; i < NVIC_NUM_INTERRUPTS + 16; i++) _VectorsRam[i] = &unused_interrupt_vector;
       for (i=0; i < NVIC_NUM_INTERRUPTS; i++) {
    	   NVIC_SET_PRIORITY(i, 128);
    	   NVIC_DISABLE_IRQ(i);
       }
       SCB_VTOR = (uint32_t)_VectorsRam;
}


FLASHMEM void init_memory(void) {
       unsigned int LoadAddr, ExeAddr, SectionLen;
       unsigned int *SectionTableAddr;

       // Load base address of Global Section Table
       SectionTableAddr = &__data_section_table;

       // Copy the data sections from flash to SRAM.
       while (SectionTableAddr < &__data_section_table_end) {
               LoadAddr = *SectionTableAddr++;
               ExeAddr = *SectionTableAddr++;
               SectionLen = *SectionTableAddr++;
               data_init(LoadAddr, ExeAddr, SectionLen);
       }

       // At this point, SectionTableAddr = &__bss_section_table;
       // Zero fill the bss segment
       while (SectionTableAddr < &__bss_section_table_end) {
               ExeAddr = *SectionTableAddr++;
               SectionLen = *SectionTableAddr++;
               bss_init(ExeAddr, SectionLen);
       }
}

FLASHMEM void configure_pins(void) {

        /* GPIO_SD_B1_05 as FLEXSPIA_DQS */
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_05     = 0x11;
        IOMUXC_FLEXSPIA_DQS_SELECT_INPUT = 0;

        /* GPIO_SD_B1_06 as FLEXSPIA_SS0_B */
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_06     = 0x11;

        /* GPIO_SD_B1_07 as FLEXSPIA_SCLK */
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_07     = 0x11;
        IOMUXC_FLEXSPIA_SCK_SELECT_INPUT = 0;

        /* GPIO_SD_B1_08 as FLEXSPIA_DATA00 */
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_08     = 0x11;
        IOMUXC_FLEXSPIA_DATA0_SELECT_INPUT = 0;

        /* GPIO_SD_B1_09 as FLEXSPIA_DATA01 */
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_09     = 0x11;
        IOMUXC_FLEXSPIA_DATA1_SELECT_INPUT = 0;

        /* GPIO_SD_B1_10 as FLEXSPIA_DATA02 */
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_10     = 0x11;
        IOMUXC_FLEXSPIA_DATA2_SELECT_INPUT = 0;

        /* GPIO_SD_B1_11 as FLEXSPIA_DATA03 */
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_11     = 0x11;
        IOMUXC_FLEXSPIA_DATA3_SELECT_INPUT = 0;


         /* Set the following pins to:
                 Drive Strength Field: R0/6
                 Speed Field: max(200MHz)
                 Open Drain Enable Field: Open Drain Disabled
                 Pull / Keep Enable Field: Pull/Keeper Enabled
                 Pull / Keep Select Field: Keeper
                 Pull Up / Down Config. Field: 100K Ohm Pull Down
                 Hyst. Enable Field: Hysteresis Disabled */

        IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B1_05 = 0x10F1u;
        IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B1_06 = 0x10F1u;
        IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B1_07 = 0x10F1u;
        IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B1_08 = 0x10F1u;
        IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B1_09 = 0x10F1u;
        IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B1_10 = 0x10F1u;
        IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B1_11 = 0x10F1u;

        IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_00 = 0x15;
		IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_01 = 0x15;
		IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02 = 0x15;
		IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03 = 0x15;
		IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_04 = 0x15;
		IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_05 = 0x15;
        IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_09 = 0x15;

#ifdef ARDUINO_QUARTO
        //Configure GPIO pins as GPIO with default speed, etc
        for(unsigned int i=1; i<9;i++) {
            *digital_pin_to_info_PGM[i].pad = 0x10b0;
            *digital_pin_to_info_PGM[i].mux = 0x05;
        }

        IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_10 = 0x15; // I2C Pullup pin
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_00 = 0x15; //set LED pins as GPIO, in case set to PWM previously
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_01 = 0x15;
        IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_02 = 0x15;

        //FLEXSPI_MCR0 &= ~FLEXSPI_MCR0_RXCLKSRC_MASK; //Set FLEXSPI_MCRO's RXCLKSRC to 00 internal loopback
        //IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B1_05 = 0x15; //on HW 2.1, enable SD power enable pin as GPIO
        //On HW 2.1, turn on power to SD Card
        //GPIO8_GDIR |= 0x20; //set SD Card power enable as output
        //GPIO8_DR_SET = 0x20; //turn on SD Card power

        IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_13 = 0x15; //set SD Card detect pin as GPIO
        GPIO7_GDIR &= ~0x20000000; //set SD Card detect as input

        IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_40 = 0x15; // Set EMC40 / ADC3 IRQ as GPIO
        IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_41 = 0x15; // Set EMC41 / MCU_E0 as input

        GPIO8_GDIR = 0x07; //Set LEDs as outputs
        GPIO8_DR_CLEAR = 0x07; //Turn off LED
        GPIO2_GDIR = 0xFFFFF; //Set DAC Update pins as outputs
        GPIO6_GDIR &= ~(0x03); //Set triggers as inputs
        GPIO6_DR_CLEAR = 0x03; //Set trigger to low.

        GPIO6_GDIR |= 0x400; //Set pull up enable pin as output
        GPIO6_DR_SET = 0x400; //Set pull up enable pin high to disable pullups

        //GPIO6_GDIR = 0x30; //Set BM as outputs for Read / ADC ACK
        GPIO6_GDIR |= ADC_ACK_PIN;
        GPIO6_GDIR |= READDATA_ACK_PIN;


        //Set USB & SD Power Enable pin to GPIO and set high if application, low if bootloader
        IOMUXC_SNVS_SW_MUX_CTL_PAD_PMIC_STBY_REQ = 0x05;
        GPIO5_GDIR |= 0x04;

		#ifdef PROG_BOOTLOADER //if bootloader, set low (off)
            GPIO5_DR_CLEAR = 0x04;
		#else
            GPIO5_DR_SET = 0x04; //turn on in application
		#endif

		//Fire interrupt on power loss to turn off USB/SD power
		 attachInterruptVector(IRQ_BROWNOUT0, USB_SD_PowerOff);
		 NVIC_SET_PRIORITY(IRQ_BROWNOUT0, 0);
		 NVIC_ENABLE_IRQ(IRQ_BROWNOUT0);


#endif
	/* Keep boot related data from being stripped from binary */
	GPIO6_PSR = (uint32_t) keep_trick; //PSR is read only, this does nothing


}






// concise defines for SCB_MPU_RASR and SCB_MPU_RBAR, ARM DDI0403E, pg 696
#define NOEXEC		SCB_MPU_RASR_XN
#define READONLY	SCB_MPU_RASR_AP(7)
#define READWRITE	SCB_MPU_RASR_AP(3)
#define NOACCESS	SCB_MPU_RASR_AP(0)
#define MEM_CACHE_WT	SCB_MPU_RASR_TEX(0) | SCB_MPU_RASR_C
#define MEM_CACHE_WB	SCB_MPU_RASR_TEX(0) | SCB_MPU_RASR_C | SCB_MPU_RASR_B
#define MEM_CACHE_WBWA	SCB_MPU_RASR_TEX(1) | SCB_MPU_RASR_C | SCB_MPU_RASR_B
#define MEM_NOCACHE	SCB_MPU_RASR_TEX(1)
#define DEV_NOCACHE	SCB_MPU_RASR_TEX(2)
#define SIZE_32B	(SCB_MPU_RASR_SIZE(4) | SCB_MPU_RASR_ENABLE)
#define SIZE_64B	(SCB_MPU_RASR_SIZE(5) | SCB_MPU_RASR_ENABLE)
#define SIZE_128B	(SCB_MPU_RASR_SIZE(6) | SCB_MPU_RASR_ENABLE)
#define SIZE_256B	(SCB_MPU_RASR_SIZE(7) | SCB_MPU_RASR_ENABLE)
#define SIZE_512B	(SCB_MPU_RASR_SIZE(8) | SCB_MPU_RASR_ENABLE)
#define SIZE_1K		(SCB_MPU_RASR_SIZE(9) | SCB_MPU_RASR_ENABLE)
#define SIZE_2K		(SCB_MPU_RASR_SIZE(10) | SCB_MPU_RASR_ENABLE)
#define SIZE_4K		(SCB_MPU_RASR_SIZE(11) | SCB_MPU_RASR_ENABLE)
#define SIZE_8K		(SCB_MPU_RASR_SIZE(12) | SCB_MPU_RASR_ENABLE)
#define SIZE_16K	(SCB_MPU_RASR_SIZE(13) | SCB_MPU_RASR_ENABLE)
#define SIZE_32K	(SCB_MPU_RASR_SIZE(14) | SCB_MPU_RASR_ENABLE)
#define SIZE_64K	(SCB_MPU_RASR_SIZE(15) | SCB_MPU_RASR_ENABLE)
#define SIZE_128K	(SCB_MPU_RASR_SIZE(16) | SCB_MPU_RASR_ENABLE)
#define SIZE_256K	(SCB_MPU_RASR_SIZE(17) | SCB_MPU_RASR_ENABLE)
#define SIZE_512K	(SCB_MPU_RASR_SIZE(18) | SCB_MPU_RASR_ENABLE)
#define SIZE_1M		(SCB_MPU_RASR_SIZE(19) | SCB_MPU_RASR_ENABLE)
#define SIZE_2M		(SCB_MPU_RASR_SIZE(20) | SCB_MPU_RASR_ENABLE)
#define SIZE_4M		(SCB_MPU_RASR_SIZE(21) | SCB_MPU_RASR_ENABLE)
#define SIZE_8M		(SCB_MPU_RASR_SIZE(22) | SCB_MPU_RASR_ENABLE)
#define SIZE_16M	(SCB_MPU_RASR_SIZE(23) | SCB_MPU_RASR_ENABLE)
#define SIZE_32M	(SCB_MPU_RASR_SIZE(24) | SCB_MPU_RASR_ENABLE)
#define SIZE_64M	(SCB_MPU_RASR_SIZE(25) | SCB_MPU_RASR_ENABLE)
#define SIZE_128M	(SCB_MPU_RASR_SIZE(26) | SCB_MPU_RASR_ENABLE)
#define SIZE_256M	(SCB_MPU_RASR_SIZE(27) | SCB_MPU_RASR_ENABLE)
#define SIZE_512M	(SCB_MPU_RASR_SIZE(28) | SCB_MPU_RASR_ENABLE)
#define SIZE_1G		(SCB_MPU_RASR_SIZE(29) | SCB_MPU_RASR_ENABLE)
#define SIZE_2G		(SCB_MPU_RASR_SIZE(30) | SCB_MPU_RASR_ENABLE)
#define SIZE_4G		(SCB_MPU_RASR_SIZE(31) | SCB_MPU_RASR_ENABLE)
#define REGION(n)	(SCB_MPU_RBAR_REGION(n) | SCB_MPU_RBAR_VALID)

FLASHMEM void configure_cache(void)
{
	//printf("MPU_TYPE = %08lX\n", SCB_MPU_TYPE);
	//printf("CCR = %08lX\n", SCB_CCR);

	// TODO: check if caches already active - skip?

	SCB_MPU_CTRL = 0; // turn off MPU

	uint32_t i = 0;
	SCB_MPU_RBAR = 0x00000000 | REGION(i++); //https://developer.arm.com/docs/146793866/10/why-does-the-cortex-m7-initiate-axim-read-accesses-to-memory-addresses-that-do-not-fall-under-a-defined-mpu-region
	SCB_MPU_RASR = SCB_MPU_RASR_TEX(0) | NOACCESS | NOEXEC | SIZE_4G;
	
	SCB_MPU_RBAR = 0x00000000 | REGION(i++); // ITCM
	SCB_MPU_RASR = MEM_NOCACHE | READWRITE | SIZE_512K;

	// TODO: trap regions should be created last, because the hardware gives
	//  priority to the higher number ones.
	SCB_MPU_RBAR = 0x00000000 | REGION(i++); // trap NULL pointer deref
	SCB_MPU_RASR =  DEV_NOCACHE | NOACCESS | SIZE_32B;

	SCB_MPU_RBAR = 0x00200000 | REGION(i++); // Boot ROM
	SCB_MPU_RASR = MEM_CACHE_WT | READONLY | SIZE_128K;

	SCB_MPU_RBAR = 0x20000000 | REGION(i++); // DTCM
	SCB_MPU_RASR = MEM_NOCACHE | READWRITE | NOEXEC | SIZE_512K;
	
	//SCB_MPU_RBAR = ((uint32_t)&_ebss) | REGION(i++); // trap stack overflow
	//SCB_MPU_RASR = SCB_MPU_RASR_TEX(0) | NOACCESS | NOEXEC | SIZE_32B;

	SCB_MPU_RBAR = 0x20200000 | REGION(i++); // RAM (AXI bus)
	SCB_MPU_RASR = MEM_CACHE_WBWA | READWRITE | NOEXEC | SIZE_1M;

	SCB_MPU_RBAR = 0x40000000 | REGION(i++); // Peripherals
	SCB_MPU_RASR = DEV_NOCACHE | READWRITE | NOEXEC | SIZE_64M;


#ifdef ARDUINO_QUARTO
	SCB_MPU_RBAR = 0x60000000 | REGION(i++); // QSPI Flash
	SCB_MPU_RASR = MEM_CACHE_WBWA | READONLY | SIZE_8M;

	#if defined(QUARTO_PROTOTYPE) || defined(QUARTO_1064)
	SCB_MPU_RBAR = 0x70000000 | REGION(i++); // FlexSPI2
	SCB_MPU_RASR = MEM_CACHE_WBWA | READONLY | /*NOEXEC |*/ SIZE_4M;
	#endif

	SCB_MPU_RBAR = 0x80000000 | REGION(i++); // External SDRAM
	SCB_MPU_RASR = MEM_CACHE_WBWA | READWRITE | SIZE_32M;

#else
	SCB_MPU_RBAR = 0x60000000 | REGION(i++); // QSPI Flash
	SCB_MPU_RASR = MEM_CACHE_WBWA | READONLY | SIZE_16M;

	SCB_MPU_RBAR = 0x70000000 | REGION(i++); // FlexSPI2
	SCB_MPU_RASR = MEM_CACHE_WBWA | READWRITE | NOEXEC | SIZE_16M;
#endif

	// TODO: protect access to power supply config

	SCB_MPU_CTRL = SCB_MPU_CTRL_ENABLE;

	// cache enable, ARM DDI0403E, pg 628
	asm("dsb");
	asm("isb");
	SCB_CACHE_ICIALLU = 0;

	asm("dsb");
	asm("isb");
	SCB_CCR |= (SCB_CCR_IC | SCB_CCR_DC);
}

#ifdef ARDUINO_TEENSY41

#define LUT0(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)))
#define LUT1(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)) << 16)
#define CMD_SDR         FLEXSPI_LUT_OPCODE_CMD_SDR
#define ADDR_SDR        FLEXSPI_LUT_OPCODE_RADDR_SDR
#define READ_SDR        FLEXSPI_LUT_OPCODE_READ_SDR
#define WRITE_SDR       FLEXSPI_LUT_OPCODE_WRITE_SDR
#define DUMMY_SDR       FLEXSPI_LUT_OPCODE_DUMMY_SDR
#define PINS1           FLEXSPI_LUT_NUM_PADS_1
#define PINS4           FLEXSPI_LUT_NUM_PADS_4

FLASHMEM static void flexspi2_command(uint32_t index, uint32_t addr)
{
	FLEXSPI2_IPCR0 = addr;
	FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(index);
	FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
	while (!(FLEXSPI2_INTR & FLEXSPI_INTR_IPCMDDONE)); // wait
	FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE;
}

FLASHMEM static uint32_t flexspi2_psram_id(uint32_t addr)
{
	FLEXSPI2_IPCR0 = addr;
	FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(3) | FLEXSPI_IPCR1_IDATSZ(4);
	FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
	while (!(FLEXSPI2_INTR & FLEXSPI_INTR_IPCMDDONE)); // wait
	uint32_t id = FLEXSPI2_RFDR0;
	FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE | FLEXSPI_INTR_IPRXWA;
	return id & 0xFFFF;
}

FLASHMEM void configure_external_ram()
{
	// initialize pins
	IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_22 = 0x1B0F9; // 100K pullup, strong drive, max speed, hyst
	IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_23 = 0x110F9; // keeper, strong drive, max speed, hyst
	IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_24 = 0x1B0F9; // 100K pullup, strong drive, max speed, hyst
	IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_25 = 0x100F9; // strong drive, max speed, hyst
	IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_26 = 0x170F9; // 47K pullup, strong drive, max speed, hyst
	IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_27 = 0x170F9; // 47K pullup, strong drive, max speed, hyst
	IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_28 = 0x170F9; // 47K pullup, strong drive, max speed, hyst
	IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_29 = 0x170F9; // 47K pullup, strong drive, max speed, hyst

	IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_22 = 8 | 0x10; // ALT1 = FLEXSPI2_A_SS1_B (Flash)
	IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_23 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DQS
	IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_24 = 8 | 0x10; // ALT1 = FLEXSPI2_A_SS0_B (RAM)
	IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_25 = 8 | 0x10; // ALT1 = FLEXSPI2_A_SCLK
	IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_26 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DATA0
	IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_27 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DATA1
	IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_28 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DATA2
	IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_29 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DATA3

	IOMUXC_FLEXSPI2_IPP_IND_DQS_FA_SELECT_INPUT = 1; // GPIO_EMC_23 for Mode: ALT8, pg 986
	IOMUXC_FLEXSPI2_IPP_IND_IO_FA_BIT0_SELECT_INPUT = 1; // GPIO_EMC_26 for Mode: ALT8
	IOMUXC_FLEXSPI2_IPP_IND_IO_FA_BIT1_SELECT_INPUT = 1; // GPIO_EMC_27 for Mode: ALT8
	IOMUXC_FLEXSPI2_IPP_IND_IO_FA_BIT2_SELECT_INPUT = 1; // GPIO_EMC_28 for Mode: ALT8
	IOMUXC_FLEXSPI2_IPP_IND_IO_FA_BIT3_SELECT_INPUT = 1; // GPIO_EMC_29 for Mode: ALT8
	IOMUXC_FLEXSPI2_IPP_IND_SCK_FA_SELECT_INPUT = 1; // GPIO_EMC_25 for Mode: ALT8

	// turn on clock  (TODO: increase clock speed later, slow & cautious for first release)
	CCM_CBCMR = (CCM_CBCMR & ~(CCM_CBCMR_FLEXSPI2_PODF_MASK | CCM_CBCMR_FLEXSPI2_CLK_SEL_MASK))
		| CCM_CBCMR_FLEXSPI2_PODF(5) | CCM_CBCMR_FLEXSPI2_CLK_SEL(3); // 88 MHz
	CCM_CCGR7 |= CCM_CCGR7_FLEXSPI2(CCM_CCGR_ON);

	FLEXSPI2_MCR0 |= FLEXSPI_MCR0_MDIS;
	FLEXSPI2_MCR0 = (FLEXSPI2_MCR0 & ~(FLEXSPI_MCR0_AHBGRANTWAIT_MASK
		 | FLEXSPI_MCR0_IPGRANTWAIT_MASK | FLEXSPI_MCR0_SCKFREERUNEN
		 | FLEXSPI_MCR0_COMBINATIONEN | FLEXSPI_MCR0_DOZEEN
		 | FLEXSPI_MCR0_HSEN | FLEXSPI_MCR0_ATDFEN | FLEXSPI_MCR0_ARDFEN
		 | FLEXSPI_MCR0_RXCLKSRC_MASK | FLEXSPI_MCR0_SWRESET))
		| FLEXSPI_MCR0_AHBGRANTWAIT(0xFF) | FLEXSPI_MCR0_IPGRANTWAIT(0xFF)
		| FLEXSPI_MCR0_RXCLKSRC(1) | FLEXSPI_MCR0_MDIS;
	FLEXSPI2_MCR1 = FLEXSPI_MCR1_SEQWAIT(0xFFFF) | FLEXSPI_MCR1_AHBBUSWAIT(0xFFFF);
	FLEXSPI2_MCR2 = (FLEXSPI_MCR2 & ~(FLEXSPI_MCR2_RESUMEWAIT_MASK
		 | FLEXSPI_MCR2_SCKBDIFFOPT | FLEXSPI_MCR2_SAMEDEVICEEN
		 | FLEXSPI_MCR2_CLRLEARNPHASE | FLEXSPI_MCR2_CLRAHBBUFOPT))
		| FLEXSPI_MCR2_RESUMEWAIT(0x20) /*| FLEXSPI_MCR2_SAMEDEVICEEN*/;

	FLEXSPI2_AHBCR = FLEXSPI2_AHBCR & ~(FLEXSPI_AHBCR_READADDROPT | FLEXSPI_AHBCR_PREFETCHEN
		| FLEXSPI_AHBCR_BUFFERABLEEN | FLEXSPI_AHBCR_CACHABLEEN);
	uint32_t mask = (FLEXSPI_AHBRXBUFCR0_PREFETCHEN | FLEXSPI_AHBRXBUFCR0_PRIORITY_MASK
		| FLEXSPI_AHBRXBUFCR0_MSTRID_MASK | FLEXSPI_AHBRXBUFCR0_BUFSZ_MASK);
	FLEXSPI2_AHBRXBUF0CR0 = (FLEXSPI2_AHBRXBUF0CR0 & ~mask)
		| FLEXSPI_AHBRXBUFCR0_PREFETCHEN | FLEXSPI_AHBRXBUFCR0_BUFSZ(64);
	FLEXSPI2_AHBRXBUF1CR0 = (FLEXSPI2_AHBRXBUF0CR0 & ~mask)
		| FLEXSPI_AHBRXBUFCR0_PREFETCHEN | FLEXSPI_AHBRXBUFCR0_BUFSZ(64);
	FLEXSPI2_AHBRXBUF2CR0 = mask;
	FLEXSPI2_AHBRXBUF3CR0 = mask;

	// RX watermark = one 64 bit line
	FLEXSPI2_IPRXFCR = (FLEXSPI_IPRXFCR & 0xFFFFFFC0) | FLEXSPI_IPRXFCR_CLRIPRXF;
	// TX watermark = one 64 bit line
	FLEXSPI2_IPTXFCR = (FLEXSPI_IPTXFCR & 0xFFFFFFC0) | FLEXSPI_IPTXFCR_CLRIPTXF;

	FLEXSPI2_INTEN = 0;
	FLEXSPI2_FLSHA1CR0 = 0x2000; // 8 MByte
	FLEXSPI2_FLSHA1CR1 = FLEXSPI_FLSHCR1_CSINTERVAL(2)
		| FLEXSPI_FLSHCR1_TCSH(3) | FLEXSPI_FLSHCR1_TCSS(3);
	FLEXSPI2_FLSHA1CR2 = FLEXSPI_FLSHCR2_AWRSEQID(6) | FLEXSPI_FLSHCR2_AWRSEQNUM(0)
		| FLEXSPI_FLSHCR2_ARDSEQID(5) | FLEXSPI_FLSHCR2_ARDSEQNUM(0);

	FLEXSPI2_FLSHA2CR0 = 0x2000; // 8 MByte
	FLEXSPI2_FLSHA2CR1 = FLEXSPI_FLSHCR1_CSINTERVAL(2)
		| FLEXSPI_FLSHCR1_TCSH(3) | FLEXSPI_FLSHCR1_TCSS(3);
	FLEXSPI2_FLSHA2CR2 = FLEXSPI_FLSHCR2_AWRSEQID(6) | FLEXSPI_FLSHCR2_AWRSEQNUM(0)
		| FLEXSPI_FLSHCR2_ARDSEQID(5) | FLEXSPI_FLSHCR2_ARDSEQNUM(0);

	FLEXSPI2_MCR0 &= ~FLEXSPI_MCR0_MDIS;

	FLEXSPI2_LUTKEY = FLEXSPI_LUTKEY_VALUE;
	FLEXSPI2_LUTCR = FLEXSPI_LUTCR_UNLOCK;
	volatile uint32_t *luttable = &FLEXSPI2_LUT0;
	for (int i=0; i < 64; i++) luttable[i] = 0;
	FLEXSPI2_MCR0 |= FLEXSPI_MCR0_SWRESET;
	while (FLEXSPI2_MCR0 & FLEXSPI_MCR0_SWRESET) ; // wait

	FLEXSPI2_LUTKEY = FLEXSPI_LUTKEY_VALUE;
	FLEXSPI2_LUTCR = FLEXSPI_LUTCR_UNLOCK;

	// cmd index 0 = exit QPI mode
	FLEXSPI2_LUT0 = LUT0(CMD_SDR, PINS4, 0xF5);
	// cmd index 1 = reset enable
	FLEXSPI2_LUT4 = LUT0(CMD_SDR, PINS1, 0x66);
	// cmd index 2 = reset
	FLEXSPI2_LUT8 = LUT0(CMD_SDR, PINS1, 0x99);
	// cmd index 3 = read ID bytes
	FLEXSPI2_LUT12 = LUT0(CMD_SDR, PINS1, 0x9F) | LUT1(DUMMY_SDR, PINS1, 24);
	FLEXSPI2_LUT13 = LUT0(READ_SDR, PINS1, 1);
	// cmd index 4 = enter QPI mode
	FLEXSPI2_LUT16 = LUT0(CMD_SDR, PINS1, 0x35);
	// cmd index 5 = read QPI
	FLEXSPI2_LUT20 = LUT0(CMD_SDR, PINS4, 0xEB) | LUT1(ADDR_SDR, PINS4, 24);
	FLEXSPI2_LUT21 = LUT0(DUMMY_SDR, PINS4, 6) | LUT1(READ_SDR, PINS4, 1);
	// cmd index 6 = write QPI
	FLEXSPI2_LUT24 = LUT0(CMD_SDR, PINS4, 0x38) | LUT1(ADDR_SDR, PINS4, 24);
	FLEXSPI2_LUT25 = LUT0(WRITE_SDR, PINS4, 1);

	// look for the first PSRAM chip
	flexspi2_command(0, 0); // exit quad mode
	flexspi2_command(1, 0); // reset enable
	flexspi2_command(2, 0); // reset (is this really necessary?)
	if (flexspi2_psram_id(0) == 0x5D0D) {
		// first PSRAM chip is present, look for a second PSRAM chip
		flexspi2_command(4, 0);
		flexspi2_command(0, 0x800000); // exit quad mode
		flexspi2_command(1, 0x800000); // reset enable
		flexspi2_command(2, 0x800000); // reset (is this really necessary?)
		if (flexspi2_psram_id(0x800000) == 0x5D0D) {
			flexspi2_command(4, 0x800000);
			// Two PSRAM chips are present, 16 MByte
			external_psram_size = 16;
		} else {
			// One PSRAM chip is present, 8 MByte
			external_psram_size = 8;
		}
		// TODO: zero uninitialized EXTMEM variables
		// TODO: copy from flash to initialize EXTMEM variables
		sm_set_pool(&extmem_smalloc_pool, &_extram_end,
			external_psram_size * 0x100000 -
			((uint32_t)&_extram_end - (uint32_t)&_extram_start),
			1, NULL);
	} else {
		// No PSRAM
		memset(&extmem_smalloc_pool, 0, sizeof(extmem_smalloc_pool));
	}
}

#elif defined(ARDUINO_QUARTO)

void adc1_irq_ignoredata(void){
	ADC1_ISR = ADC1_BM; // Clear Interrupt
	GPIO6_DR_TOGGLE = 0x100; // Dummy GPIO write - necessary delay to avoid double firing
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC Data
	GPIO6_DR_TOGGLE = 0x100; // Dummy GPIO write - necessary delay to avoid double firing
	return;
}

void adc2_irq_ignoredata(void) {
	ADC2_ISR = ADC2_BM; // Clear Interrupt
	GPIO6_DR_TOGGLE = 0x100; // Dummy GPIO write - necessary delay to avoid double firing
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC Data
	GPIO6_DR_TOGGLE = 0x100; // Dummy GPIO write - necessary delay to avoid double firing
}

void adc3_irq_ignoredata(void) {
	ADC3_ISR = ADC3_BM; // Clear Interrupt
	GPIO6_DR_TOGGLE = 0x100; // Dummy GPIO write - necessary delay to avoid double firing
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC Data
	GPIO6_DR_TOGGLE = 0x100; // Dummy GPIO write - necessary delay to avoid double firing

}

void adc4_irq_ignoredata(void) {
	ADC4_ISR = ADC4_BM; // Clear Interrupt
	GPIO6_DR_TOGGLE = 0x100; // Dummy GPIO write - necessary delay to avoid double firing
	ADC_ACK_BANK_TOGGLE = ADC_ACK_PIN; // ACK ADC Data
	GPIO6_DR_TOGGLE = 0x100; // Dummy GPIO write - necessary delay to avoid double firing
}




FLASHMEM void USB_SD_PowerOff(void) { //Turn on Volt regs to USB and SD
	IOMUXC_SNVS_SW_MUX_CTL_PAD_PMIC_STBY_REQ = 0x05;
	GPIO5_GDIR |= 0x04;
	GPIO5_DR_CLEAR = 0x04;
}


#endif


FLASHMEM void usb_pll_start()
{
	while (1) {
		uint32_t n = CCM_ANALOG_PLL_USB1; // pg 759
		printf("CCM_ANALOG_PLL_USB1=%08lX\n", n);
		if (n & CCM_ANALOG_PLL_USB1_DIV_SELECT) {
			printf("  ERROR, 528 MHz mode!\n"); // never supposed to use this mode!
			#if defined(PROG_BOOTLOADER) || defined(INTERNAL_CLOCK) //leave 24MHz source unchanged
				CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_BYPASS;	// bypass
			#else
				CCM_ANALOG_PLL_USB1_CLR = 0xC000;			// bypass 24 MHz, then set
				CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_BYPASS | PLL_BYPASS_TO_EXTERNAL_LVDS;	// bypass
			#endif

			CCM_ANALOG_PLL_USB1_CLR = CCM_ANALOG_PLL_USB1_POWER |   // power down
				CCM_ANALOG_PLL_USB1_DIV_SELECT |		// use 480 MHz
				CCM_ANALOG_PLL_USB1_ENABLE |			// disable
				CCM_ANALOG_PLL_USB1_EN_USB_CLKS;		// disable usb
			continue;
		}
#if !defined(PROG_BOOTLOADER) && !defined(INTERNAL_CLOCK)
		if ((n & 0xC000) != PLL_BYPASS_TO_EXTERNAL_LVDS) {
			n &= ~(0xC000); //14bit is bypass source, clear it
			n |= PLL_BYPASS_TO_EXTERNAL_LVDS; //set PLL source bits
			CCM_ANALOG_PLL_USB1 = n; //update PLL
			//CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_BYPASS | PLL_BYPASS_TO_EXTERNAL_LVDS;
			//CCM_ANALOG_PLL_USB1_CLR = CCM_ANALOG_PLL_USB1_POWER | CCM_ANALOG_PLL_USB1_ENABLE |  CCM_ANALOG_PLL_USB1_EN_USB_CLKS;
			continue;
		}
#endif
		if (!(n & CCM_ANALOG_PLL_USB1_ENABLE)) {
			printf("  enable PLL\n");
			// TODO: should this be done so early, or later??
			CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_ENABLE;
			continue;
		}
		if (!(n & CCM_ANALOG_PLL_USB1_POWER)) {
			printf("  power up PLL\n");
			CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_POWER;
			continue;
		}
		if (!(n & CCM_ANALOG_PLL_USB1_LOCK)) {
			printf("  wait for lock\n");
			continue;
		}
		if (n & CCM_ANALOG_PLL_USB1_BYPASS) {
			printf("  turn off bypass\n");
			CCM_ANALOG_PLL_USB1_CLR = CCM_ANALOG_PLL_USB1_BYPASS;
			continue;
		}
		if (!(n & CCM_ANALOG_PLL_USB1_EN_USB_CLKS)) {
			printf("  enable USB clocks\n");
			CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_EN_USB_CLKS;
			continue;
		}
		return; // everything is as it should be  :-)
	}
}




FLASHMEM void foreverLoopAndToggle(void) {
	GPIO6_GDIR = 0x03;
	GPIO1_GDIR = 0x03;
	while(1) {
		GPIO6_DR_SET = 0x03;
		GPIO1_DR_SET = 0x03;
		asm volatile("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
		asm volatile("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
		GPIO6_DR_CLEAR = 0x03;
		GPIO1_DR_CLEAR = 0x03;
		asm volatile("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
		asm volatile("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	}
}


FLASHMEM void approxMSdelay(unsigned int ms) {
	volatile unsigned int i;
	unsigned int loops = ms*12500;
	for(i=0;i<loops;i++);
}


#define PFDX_STABLE_MASK 0x40404040
FLASHMEM uint32_t generateMask(uint32_t current_setting, uint32_t target_setting) {
	target_setting &= ~PFDX_STABLE_MASK; //mask off stable bits
	uint32_t pins = 0;
	uint8_t current_pll;
	uint8_t target_pll;
	if ( (current_setting & ~PFDX_STABLE_MASK) != target_setting ) {
		//PFD not configured right, must change
		for(int i=0; i<4; i++) {
			current_pll = current_setting>>(8*i);
			target_pll = target_setting>>(8*i);
			if ((current_pll & ~0x40 ) != target_pll) {
				//will make change, expect stable mask to toggle
				if ( (current_pll & 0x40) == 0) {
					//starts low, so now high
					pins |= (0x40)<<(8*i);
				}
			} else {
				// no change, pin should stay the same
				pins |= (current_pll  & 0x40)<<(8*i);
			}
		}
	} else {
		//no change,
		pins = current_setting & PFDX_STABLE_MASK;
	}
	return pins;
}



FLASHMEM  void reset_PFD()
{
	uint32_t pins = 0;

	//PLL2 PFDs:
	pins = generateMask(CCM_ANALOG_PFD_528,0x2018101B);
	CCM_ANALOG_PFD_528 = 0x2018101B;
	while ( (CCM_ANALOG_PFD_528 & PFDX_STABLE_MASK) != pins){
		approxMSdelay(1);
	}

	//PLL3:
	pins = generateMask(CCM_ANALOG_PFD_480,0x13110D0C);
	CCM_ANALOG_PFD_480 = 0x13110D0C; // PFD0:720, PFD1:664, PFD2:508, PFD3:454 MHz
	while ( (CCM_ANALOG_PFD_480 & PFDX_STABLE_MASK) != pins) {
		approxMSdelay(1);
	}
}

extern void usb_isr(void);

// Stack frame
//  xPSR
//  ReturnAddress
//  LR (R14) - typically FFFFFFF9 for IRQ or Exception
//  R12
//  R3
//  R2
//  R1
//  R0
// Code from :: https://community.nxp.com/thread/389002


__attribute__((naked))
void unused_interrupt_vector(void)
{
	uint32_t i, ipsr, crc;
	const uint32_t *stack;
	struct arm_fault_info_struct *info;
	const uint32_t *p, *end;

	// disallow any nested interrupts
	__disable_irq();
	// store crash report info
	asm volatile("mrs %0, ipsr\n" : "=r" (ipsr) :: "memory");
	info = (struct arm_fault_info_struct *)0x2027FF80;
	info->ipsr = ipsr;
	asm volatile("mrs %0, msp\n" : "=r" (stack) :: "memory");
	info->cfsr = SCB_CFSR;
	info->hfsr = SCB_HFSR;
	info->mmfar = SCB_MMFAR;
	info->bfar = SCB_BFAR;
	info->ret = stack[6];
	info->xpsr = stack[7];
	info->temp = tempmonGetTemp();
	info->time = rtc_get();
	info->len = sizeof(*info) / 4;
	// add CRC to crash report
	crc = 0xFFFFFFFF;
	p = (uint32_t *)info;
	end = p + (sizeof(*info) / 4 - 1);
	while (p < end) {
		crc ^= *p++;
		for (i=0; i < 32; i++) crc = (crc >> 1) ^ (crc & 1)*0xEDB88320;
	}
	info->crc = crc;
	arm_dcache_flush_delete(info, sizeof(*info));

	// LED blink can show fault mode - by default we don't mess with pin 13
	//IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_03 = 5; // pin 13
	//IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_03 = IOMUXC_PAD_DSE(7);
	//GPIO7_GDIR |= (1 << 3);

	// reinitialize PIT timer and CPU clock
	CCM_CCGR1 |= CCM_CCGR1_PIT(CCM_CCGR_ON);
	PIT_MCR = PIT_MCR_MDIS;
	CCM_CSCMR1 = (CCM_CSCMR1 & ~CCM_CSCMR1_PERCLK_PODF(0x3F)) | CCM_CSCMR1_PERCLK_CLK_SEL;
  	if (F_CPU_ACTUAL > 198000000) set_arm_clock(198000000);
	PIT_MCR = 0;
	PIT_TCTRL0 = 0;
	PIT_LDVAL0 = 2400000; // 2400000 = 100ms
	PIT_TCTRL0 = PIT_TCTRL_TEN;
	// disable all NVIC interrupts, as usb_isr() might use __enable_irq()
	NVIC_ICER0 = 0xFFFFFFFF;
	NVIC_ICER1 = 0xFFFFFFFF;
	NVIC_ICER2 = 0xFFFFFFFF;
	NVIC_ICER3 = 0xFFFFFFFF;
	NVIC_ICER4 = 0xFFFFFFFF;

#ifdef ARDUINO_QUARTO
	SCB_AIRCR = 0x05FA0004; //reboot device
	while (1) ;
#else
	// keep USB running, so any unsent Serial.print() actually arrives in
	// the Arduino Serial Monitor, and we remain responsive to Upload
	// without requiring manual press of Teensy's pushbutton

	uint32_t count = 0;

	while (1) {
		if (PIT_TFLG0) {
			//GPIO7_DR_TOGGLE = (1 << 3); // blink LED
			PIT_TFLG0 = 1;
			if (++count >= 80) break;  // reboot after 8 seconds
		}
		usb_isr();
		// TODO: should other data flush / cleanup tasks be done here?
		//   Transmit Serial1 - Serial8 data
		//   Complete writes to SD card
		//   Flush/sync LittleFS
	}
	// turn off USB
	USB1_USBCMD = USB_USBCMD_RST;
	USBPHY1_CTRL_SET = USBPHY_CTRL_SFTRST;
	while (PIT_TFLG0 == 0); /* wait 0.1 second for PC to know USB unplugged */
	// reboot
#endif
}

__attribute__((section(".startup"), optimize("O1")))
void data_init(unsigned int romstart, unsigned int start, unsigned int len) {
    unsigned int *pulDest = (unsigned int*) start;
    unsigned int *pulSrc = (unsigned int*) romstart;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = *pulSrc++;
}

__attribute__((section(".startup"), optimize("O1")))
void bss_init(unsigned int start, unsigned int len) {
    unsigned int *pulDest = (unsigned int*) start;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = 0;
}


// syscall functions need to be in the same C file as the entry point "ResetVector"
// otherwise the linker will discard them in some cases.

#include <errno.h>

// from the linker script
extern unsigned long _heap_start;
extern unsigned long _heap_end;

char *__brkval = (char *)&_heap_start;

void * _sbrk(int incr)
{
        char *prev = __brkval;
        if (incr != 0) {
                if (prev + incr > (char *)&_heap_end) {
                        errno = ENOMEM;
                        return (void *)-1;
                }
                __brkval = prev + incr;
        }
        return prev;
}

__attribute__((weak))
int _read(int file, char *ptr, int len)
{
	return 0;
}

__attribute__((weak))
int _close(int fd)
{
	return -1;
}

#include <sys/stat.h>

__attribute__((weak))
int _fstat(int fd, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

__attribute__((weak))
int _isatty(int fd)
{
	return 1;
}

__attribute__((weak))
int _lseek(int fd, long long offset, int whence)
{
	return -1;
}

__attribute__((weak))
void _exit(int status)
{
	while (1) asm ("WFI");
}

__attribute__((weak))
void __cxa_pure_virtual()
{
	while (1) asm ("WFI");
}

__attribute__((weak))
int __cxa_guard_acquire (char *g)
{
	return !(*g);
}

__attribute__((weak))
void __cxa_guard_release(char *g)
{
	*g = 1;
}

__attribute__((weak))
void abort(void)
{
	while (1) asm ("WFI");
}

