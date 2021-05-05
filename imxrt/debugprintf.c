#include "debug/printf.h"

#ifdef PRINT_DEBUG_STUFF

#include "avr/pgmspace.h"
#include <stdarg.h>
#include "imxrt.h"

void putchar_debug(char c);
static void puint_debug(unsigned int num);

#define printf_debug printf

static void puint_debug(unsigned int num)
{
	char buf[12];
	unsigned int i = sizeof(buf)-2;

	buf[sizeof(buf)-1] = 0;
	while (1) {
		buf[i] = (num % 10) + '0';
		num /= 10;
		if (num == 0) break;
		i--;
	}
	printf_debug(buf + i);
}

#define putchar_debug putchar


#define CoreDebug_BASE (0xE000EDF0UL) /*!< Core Debug Base Address */
#define CoreDebug ((CoreDebug_Type *) CoreDebug_BASE) /*!< Core Debug configuration struct */


typedef struct
{
	volatile uint32_t DHCSR;                  /*!< Offset: 0x000 (R/W)  Debug Halting Control and Status Register */
	volatile  uint32_t DCRSR;                  /*!< Offset: 0x004 ( /W)  Debug Core Register Selector Register */
	volatile uint32_t DCRDR;                  /*!< Offset: 0x008 (R/W)  Debug Core Register Data Register */
	volatile uint32_t DEMCR;                  /*!< Offset: 0x00C (R/W)  Debug Exception and Monitor Control Register */
} CoreDebug_Type;

#define CoreDebug_DHCSR_C_DEBUGEN_Pos       0U                                            /*!< CoreDebug DHCSR: C_DEBUGEN Position */
#define CoreDebug_DHCSR_C_DEBUGEN_Msk      (1UL /*<< CoreDebug_DHCSR_C_DEBUGEN_Pos*/)     /*!< CoreDebug DHCSR: C_DEBUGEN Mask */



FLASHMEM void printf_debug_init(void){
	GPIO8_GDIR = 0x07;
	GPIO8_DR_CLEAR = 0x07;
	if ((CoreDebug->DHCSR &  CoreDebug_DHCSR_C_DEBUGEN_Msk) != CoreDebug_DHCSR_C_DEBUGEN_Msk) {
	    //Debugging enabled, but not in debug mode, so halt and prevent crash
		GPIO8_DR_SET = 0x1;
		volatile int i,j;
		j=324;
	    while(1){
	        GPIO8_DR_TOGGLE = 0x1;
	        for(i=0;i<350000;i++){
	            j += j*((j+45)*i);
	        }
	    }
	}



};


#endif // PRINT_DEBUG_STUFF
