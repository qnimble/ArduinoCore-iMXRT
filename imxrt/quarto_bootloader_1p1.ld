MEMORY
{
	ITCM (rwx) : ORIGIN = 0x0, LENGTH = 0x80000      
	DTCM (rwx) : ORIGIN = 0x20000000, LENGTH = 0x80000
	RAM (rwx) : ORIGIN = 0x20200000, LENGTH = 0x20000
	FLASH (rx) : ORIGIN = DEFINED(QUARTO_1064) ? 0x70000000 : 0x60000000, LENGTH = 0x40000
	FLASH2 (rx) : ORIGIN = DEFINED(QUARTO_1064) ? 0x70040000 : 0x60040000, LENGTH = 0x400000
	ERAM (rwx) : ORIGIN = 0x80000000, LENGTH = 32M
}
/* 
GROUP (
  "libm.a"
  "libgcc.a"
  "libc_nano.a"
  "libstdc++.a"
  "libcr_newlib_semihost.a"
)
*/

ENTRY(ResetHandler)

SECTIONS
{
     /* Image Vector Table and Boot Data for booting from external flash */
    .boot_hdr : ALIGN(4)
    {
        FILL(0xff)
        __boot_hdr_start__ = ABSOLUTE(.) ;
        KEEP(*(.boot_hdr.conf))
        . = 0x0800 ;
		KEEP(*(.lookup_table))
        . = 0x1000 ;
        KEEP(*(.boot_hdr.ivt))
        . = 0x1020 ;
        KEEP(*(.boot_hdr.boot_data))
        . = 0x1030 ;
        KEEP(*(.boot_hdr.dcd_data))
        __boot_hdr_end__ = ABSOLUTE(.) ;
    } >FLASH

    .text.flash2 : ALIGN(4)
    {
       *(.extraflash*)
        . = ALIGN(16);

    } > FLASH2
}

INCLUDE "quarto_common.ld"

