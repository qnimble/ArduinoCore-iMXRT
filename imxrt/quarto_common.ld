
SECTIONS
{
     /* Image Vector Table and Boot Data for booting from external flash */
     
    "/DISCARD/" : ALIGN(4)
    {
        *(.boot_hdr.conf)
        *(.boot_hdr.ivt)
        *(.boot_hdr.boot_data)
        *(.boot_hdr.dcd_data)
       
    } > RAM

    /* MAIN TEXT SECTION */
    .text.bootinfo : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.) ;
        KEEP(*(.isr_vector))
        /* Global Section Table */
        . = ALIGN(4) ;
        __section_table_start = .;
        __data_section_table = .;
        
        
        LONG(LOADADDR(.data.DTCM));
        LONG(    ADDR(.data.DTCM));
        LONG(  SIZEOF(.data.DTCM));
        LONG(LOADADDR(.data.ITCM));
        LONG(    ADDR(.data.ITCM));
        LONG(  SIZEOF(.data.ITCM) + SIZEOF(.ARM.exidx));
        LONG(LOADADDR(.data.RAM));
        LONG(    ADDR(.data.RAM));
        LONG(  SIZEOF(.data.RAM));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss.DTCM));
        LONG(  SIZEOF(.bss.DTCM));
        LONG(    ADDR(.bss.ITCM));
        LONG(  SIZEOF(.bss.ITCM));
        LONG(   ADDR(.bss.RAM));
        LONG(  SIZEOF(.bss.RAM));
        LONG(   ADDR(.bss.ERAM));
        LONG(  SIZEOF(.bss.ERAM));
         __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */
				
        *(.after_vectors*)
        *(.startup*)

    } > FLASH

    .text.flash : ALIGN(4)
    {       
       *(.progmem*)   
       *(.flashmem*)           
       . = ALIGN(4);
       
        KEEP(*(.init))
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP (*(.init_array))
        __init_array_end = .;
        . = ALIGN(16);
              
    } > FLASH
    /*
     * for exception handling/unwind - some Newlib functions (in common
     * with C++ and STDC++) use this.
     */
     
    .bss.RAM (NOLOAD) : {
       PROVIDE(__start_bss_OC = .) ;
     	PROVIDE(__start_bss_RAM3 = .) ;
       PROVIDE(__start_bss_SRAM_OC = .) ;
     	
     	*(.dmabuffers)
       *(.bss.$RAM3)
       *(.bss.$SRAM_OC)
       *(.bss.$RAM3.*)
       *(.bss.$SRAM_OC.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
     	. = ALIGN(16);
       PROVIDE(__end_bss_RAM3 = .) ;
       PROVIDE(__end_bss_SRAM_OC = .) ; 	
     PROVIDE(__stop_bss_OC = .) ;
     	
     } > RAM AT >RAM
     
     .bss.ERAM (NOLOAD) : {
        *(.externalram)
     	. = ALIGN(32);
     } > ERAM
     
  

     
    _etext = .;
        
    /* DATA section for SRAM_ITC */

    .data.ITCM : ALIGN(4)
    {
   		. = . + 32; /* MPU to trap NULL pointer deref */
        PROVIDE(__start_data_RAM2 = .) ;
        PROVIDE(__start_data_SRAM_ITC = .) ;       
        *(.fastrun)		
		*(.text*)		
        *(.ramfunc.$RAM2)
        *(.ramfunc.$SRAM_ITC)        
        *(.data.$RAM2)      
        *(.data.$SRAM_ITC)      
        *(.data.$RAM2.*)        
        *(.data.$SRAM_ITC.*)       
        . = ALIGN(16) ;
        PROVIDE(__end_data_RAM2 = .) ;
        PROVIDE(__end_data_SRAM_ITC = .) ;
      } > ITCM AT>FLASH 
     
     .ARM.exidx : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx* .ARM.extab.text*  .gnu.linkonce.armexidx.*)
        __exidx_end = .;
     } > ITCM AT >FLASH 
 	

    /* DATA section for SRAM_OC */
/*
 	.data.ITCM.padding  (NOLOAD) : {
		. = ALIGN(32768);		
	} > ITCM
 */	


    .data.RAM : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM3 = .) ;
        PROVIDE(__start_data_SRAM_OC = .) ;
        *(.ramfunc.$RAM3)
        *(.ramfunc.$SRAM_OC)
        *(.data.$RAM3)
        *(.data.$SRAM_OC)
        *(.data.$RAM3.*)
        *(.data.$SRAM_OC.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM3 = .) ;
        PROVIDE(__end_data_SRAM_OC = .) ;
     } > RAM AT>FLASH

    /* MAIN DATA SECTION */
       /* Main DATA section (SRAM_DTC) */
    .data.DTCM : ALIGN(4)
    {
       FILL(0xff)
       _data = . ;
       PROVIDE(__start_data_RAM = .) ;
       PROVIDE(__start_data_SRAM_DTC = .) ;
       *(.vectorsram)
       *(.vectors)
       *(vtable)
       . = 0x2C0;
       *(.data.usb_serial_number)
       . = 0x2E2;
       *(.ramfunc*)
       KEEP(*(CodeQuickAccess))
       KEEP(*(DataQuickAccess))
       *(RamFunction)
       *(NonCacheable.init)       
       *(.data*)
       *(.rodata .rodata.* .constdata .constdata.*)     
       *(.endpoint_queue)
       . = ALIGN(4) ;
       _edata = . ;
       PROVIDE(__end_data_RAM = .) ;
       PROVIDE(__end_data_SRAM_DTC = .) ;       	
       PROVIDE(__end_data_SRAM_DTC_PADDING = .) ;
    } > DTCM AT>FLASH

    /*     
    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > DTCM AT >FLASH 
    */


    /* BSS section for SRAM_ITC */
    .bss.ITCM : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM2 = .) ;
       PROVIDE(__start_bss_SRAM_ITC = .) ;
       *(.bss.$RAM2)
       *(.bss.$SRAM_ITC)
       *(.bss.$RAM2.*)
       *(.bss.$SRAM_ITC.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM2 = .) ;
       PROVIDE(__end_bss_SRAM_ITC = .) ;
    } > ITCM AT> ITCM

    
    /* MAIN BSS SECTION */
    .bss.DTCM (NOLOAD) : ALIGN(4)
    {
        _bss = .;
        __bss_start__ = .;
        PROVIDE(__start_bss_RAM = .) ;
        PROVIDE(__start_bss_SRAM_DTC = .) ;
       *(NonCacheable)
        *(.bss*)
        *(COMMON)
        . = ALIGN(32);
		. = . + 32; /* MPU to trap stack overflow */        
        _ebss = .;
        __bss_end__ = .;
        PROVIDE(__end_bss_RAM = .) ;
        PROVIDE(__end_bss_SRAM_DTC = .) ;
        PROVIDE(end = .);
        /* . +=  _itcm_block_count*0x8000; *//* increase by DTCM size since available DTCM smaller by this amount */
        PROVIDE(__end_bss_SRAM_DTC_plusCutout = .) ;
    } > DTCM AT> DTCM

	.bss.DTCM.ITCMcutout (NOLOAD): ALIGN(4)
	{
		. +=  _itcm_block_count*0x8000; /* increase by DTCM size since available DTCM smaller by this amount */
        
	} > DTCM AT >DTCM

 .extra _binary_load_addr : ALIGN(1024)
    {
        *(.binary.load)
    } > FLASH

 
    _itcm_block_count = (SIZEOF(.data.ITCM) + SIZEOF(.bss.ITCM) + SIZEOF(.ARM.exidx) + 0x7FFF) >> 15;
	_flexram_bank_config = 0xAAAAAAAA | ((1 << (_itcm_block_count * 2)) - 1);
	_estack = ORIGIN(DTCM) + ((16 - _itcm_block_count) << 15) - 32;
    _vStackTop = 0x2001f000; /* Initial Stack pointer */
     _heap_start = ADDR(.data.RAM) + SIZEOF(.data.RAM);
    _heap_end = ORIGIN(RAM) + LENGTH(RAM);
    _binary_load_addr = 0x60080000;
}
