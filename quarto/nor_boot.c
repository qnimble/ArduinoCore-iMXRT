/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "nor_boot.h"

extern void _vStackTop(void);
extern void ResetHandler(void);

__attribute__ ((used, section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    &_vStackTop,                       // The initial stack pointer
    ResetHandler,                      // The reset handler (boot startup)
    NMI_Handler,                       // The NMI handler
    HardFault_Handler,                 // The hard fault handler
};

__attribute__((section(".boot_hdr.ivt")))
const ivt image_vector_table = {
  IVT_HEADER,                         /* IVT Header */
  IMAGE_ENTRY_ADDRESS,                /* Image Entry Function */
  IVT_RSVD,                           /* Reserved = 0 */
  (uint32_t)DCD_ADDRESS,              /* Address where DCD information is stored */
  (uint32_t)BOOT_DATA_ADDRESS,        /* Address where BOOT Data Structure is stored */
  (uint32_t)&image_vector_table,      /* Pointer to IVT Self (absolute address */
  (uint32_t)CSF_ADDRESS,              /* Address where CSF file is stored */
  IVT_RSVD                            /* Reserved = 0 */
};


__attribute__((section(".boot_hdr.boot_data")))
const BOOT_DATA_T boot_data = {
  FLASH_BASE,                 /* boot start location */
  BOARD_FLASH_SIZE,                 /* size */
  PLUGIN_FLAG,                /* Plugin flag*/
  0xFFFFFFFF  				  /* empty - extra data word */
};

__attribute__((section(".boot_hdr.conf")))
const flexspi_nor_config_t qspiflash_config = {
	.memConfig = {
		.tag              = FLEXSPI_CFG_BLK_TAG,
		.version          = FLEXSPI_CFG_BLK_VERSION,
		.readSampleClkSrc = kFlexSPIReadSampleClk_LoopbackFromDqsPad,
		.csHoldTime       = 3u,
		.csSetupTime      = 3u,
		// Enable DDR mode, Wordaddassable, Safe configuration, Differential clock
		.sflashPadType = kSerialFlash_4Pads,
		.serialClkFreq = kFlexSpiSerialClk_100MHz,
		.sflashA1Size  = 8u * 1024u * 1024u,
		.lookupTable = 	{
			// Read LUTs
			FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xEB, RADDR_SDR, FLEXSPI_4PAD, 0x18),
			FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD, 0x06, READ_SDR, FLEXSPI_4PAD, 0x04),
		},
	},

	.pageSize           = 256u,
	.sectorSize         = 4u * 1024u,
	.blockSize          = 256u * 1024u,
	.isUniformBlockSize = false,
    };


__attribute__ ((weak, section(".startup"))) void NMI_Handler(void) {
	while(1) {}
}

__attribute__ ((weak, section(".startup"))) void HardFault_Handler(void) {
	while(1) {}
}
