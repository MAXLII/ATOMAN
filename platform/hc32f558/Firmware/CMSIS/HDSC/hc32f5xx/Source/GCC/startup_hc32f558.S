;/**
; ******************************************************************************
;  @file  startup_hc32f558.S
;  @brief Startup for GCC.
; verbatim
;  Change Logs:
;  Date             Author          Notes
;  2026-04-16       CDT             First version
; endverbatim
; *****************************************************************************
; * Copyright (C) 2022-2026, Xiaohua Semiconductor Co., Ltd. All rights reserved.
; *
; * This software component is licensed by XHSC under BSD 3-Clause license
; * (the "License"); You may not use this file except in compliance with the
; * License. You may obtain a copy of the License at:
; *                    opensource.org/licenses/BSD-3-Clause
; *
; ******************************************************************************
; */

/*
;//-------- <<< Use Configuration Wizard in Context Menu >>> ------------------
*/

                .syntax     unified
                .arch       armv7e-m
                .cpu        cortex-m4
                .fpu        softvfp
                .thumb

/*
;<h> Stack Configuration
;  <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
;</h>
*/
                .equ        Stack_Size, 0x00002000

                .section    .stack
                .align      3
                .globl      __StackTop
                .globl      __StackLimit
__StackLimit:
                .space      Stack_Size
                .size       __StackLimit, . - __StackLimit
__StackTop:
                .size       __StackTop, . - __StackTop


/*
;<h> Heap Configuration
;  <o> Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
;</h>
*/
                .equ        Heap_Size, 0x00002000

                .if         Heap_Size != 0                     /* Heap is provided */
                .section    .heap
                .align      3
                .globl      __HeapBase
                .globl      __HeapLimit
__HeapBase:
                .space      Heap_Size
                .size       __HeapBase, . - __HeapBase
__HeapLimit:
                .size       __HeapLimit, . - __HeapLimit
                .endif

/*
;<h> Interrupt vector table start.
*/
                .section    .vectors, "a", %progbits
                .align      2
                .type       __Vectors, %object
                .globl      __Vectors
                .globl      __Vectors_End
                .globl      __Vectors_Size
__Vectors:
                .long       __StackTop                         /*     Top of Stack */
                .long       Reset_Handler                      /*     Reset Handler */
                .long       NMI_Handler                        /* -14 NMI Handler */
                .long       HardFault_Handler                  /* -13 Hard Fault Handler */
                .long       MemManage_Handler                  /* -12 MPU Fault Handler */
                .long       BusFault_Handler                   /* -11 Bus Fault Handler */
                .long       UsageFault_Handler                 /* -10 Usage Fault Handler */
                .long       0                                  /*     Reserved */
                .long       0                                  /*     Reserved */
                .long       0                                  /*     Reserved */
                .long       0                                  /*     Reserved */
                .long       SVC_Handler                        /*  -5 SVCall Handler */
                .long       DebugMon_Handler                   /*  -4 Debug Monitor Handler */
                .long       0                                  /*     Reserved */
                .long       PendSV_Handler                     /*  -2 PendSV Handler */
                .long       SysTick_Handler                    /*  -1 SysTick Handler */

                /* Interrupts */
                .long       IRQ000_Handler
                .long       IRQ001_Handler
                .long       IRQ002_Handler
                .long       IRQ003_Handler
                .long       IRQ004_Handler
                .long       IRQ005_Handler
                .long       IRQ006_Handler
                .long       IRQ007_Handler
                .long       IRQ008_Handler
                .long       IRQ009_Handler
                .long       IRQ010_Handler
                .long       IRQ011_Handler
                .long       IRQ012_Handler
                .long       IRQ013_Handler
                .long       IRQ014_Handler
                .long       IRQ015_Handler
                .long       EXTINT_PORT_EIRQ0_Handler
                .long       EXTINT_PORT_EIRQ1_Handler
                .long       EXTINT_PORT_EIRQ2_Handler
                .long       EXTINT_PORT_EIRQ3_Handler
                .long       EXTINT_PORT_EIRQ4_Handler
                .long       EXTINT_PORT_EIRQ5_Handler
                .long       EXTINT_PORT_EIRQ6_Handler
                .long       EXTINT_PORT_EIRQ7_Handler
                .long       EXTINT_PORT_EIRQ8_Handler
                .long       EXTINT_PORT_EIRQ9_Handler
                .long       EXTINT_PORT_EIRQ10_Handler
                .long       EXTINT_PORT_EIRQ11_Handler
                .long       EXTINT_PORT_EIRQ12_Handler
                .long       EXTINT_PORT_EIRQ13_Handler
                .long       EXTINT_PORT_EIRQ14_Handler
                .long       EXTINT_PORT_EIRQ15_Handler
                .long       DMA1_Error_Handler
                .long       DMA1_TC0_BTC0_Handler
                .long       DMA1_TC1_BTC1_Handler
                .long       DMA1_TC2_BTC2_Handler
                .long       DMA1_TC3_BTC3_Handler
                .long       DMA1_TC4_BTC4_Handler
                .long       DMA1_TC5_BTC5_Handler
                .long       DMA1_TC6_BTC6_Handler
                .long       DMA1_TC7_BTC7_Handler
                .long       EFM_PEError_ReadCol_Handler
                .long       EFM_OpEnd_Handler
                .long       FPU_Error_Handler
                .long       DMA2_Error_Handler
                .long       DMA2_TC0_BTC0_Handler
                .long       DMA2_TC1_BTC1_Handler
                .long       DMA2_TC2_BTC2_Handler
                .long       DMA2_TC3_BTC3_Handler
                .long       DMA2_TC4_BTC4_Handler
                .long       DMA2_TC5_BTC5_Handler
                .long       DMA2_TC6_BTC6_Handler
                .long       DMA2_TC7_BTC7_Handler
                .long       PID_CALC_OV_Handler
                .long       PID_CMP_Handler
                .long       FMAC_FIR_Handler
                .long       FMAC_IIR_Handler
                .long       FMAC_RAM_ECC_ERR_Handler
                .long       CORDIC_Handler
                .long       TMR0_1_Handler
                .long       TMR0_2_Handler
                .long       XTAL_Handler
                .long       SWDT_Handler
                .long       TMR6_1_GCmp_Handler
                .long       TMR6_1_Ovf_Udf_Handler
                .long       TMR6_1_Dte_Handler
                .long       TMR6_1_SCmp_Handler
                .long       TMR6_2_GCmp_Handler
                .long       TMR6_2_Ovf_Udf_Handler
                .long       TMR6_2_Dte_Handler
                .long       TMR6_2_SCmp_Handler
                .long       TMR6_3_GCmp_Handler
                .long       TMR6_3_Ovf_Udf_Handler
                .long       TMR6_3_Dte_Handler
                .long       TMR6_3_SCmp_Handler
                .long       TMR6_4_GCmp_Handler
                .long       TMR6_4_Ovf_Udf_Handler
                .long       TMR6_4_Dte_Handler
                .long       TMR6_4_SCmp_Handler
                .long       TMR6_5_GCmp_Handler
                .long       TMR6_5_Ovf_Udf_Handler
                .long       TMR6_5_Dte_Handler
                .long       TMR6_5_SCmp_Handler
                .long       TMR6_6_GCmp_Handler
                .long       TMR6_6_Ovf_Udf_Handler
                .long       TMR6_6_Dte_Handler
                .long       TMR6_6_SCmp_Handler
                .long       TRLPWM_Handler
                .long       EMB_GR0_Handler
                .long       EMB_GR1_Handler
                .long       EMB_GR2_Handler
                .long       EMB_GR3_Handler
                .long       EMB_GR4_Handler
                .long       EMB_GR5_Handler
                .long       HRPWM_1_Handler
                .long       HRPWM_2_Handler
                .long       HRPWM_3_Handler
                .long       HRPWM_4_Handler
                .long       HRPWM_5_Handler
                .long       HRPWM_6_Handler
                .long       HRPWM_7_Handler
                .long       HRPWM_8_Handler
                .long       HRPWM_EMB_Handler
                .long       SDFM_FILTER0_Handler
                .long       SDFM_FILTER1_Handler
                .long       SDFM_FILTER2_Handler
                .long       SDFM_FILTER3_Handler
                .long       CMP1_Handler
                .long       CMP2_Handler
                .long       CMP3_Handler
                .long       CMP4_Handler
                .long       CMP5_Handler
                .long       CMP6_Handler
                .long       CMP7_Handler
                .long       CMP8_Handler
                .long       SPI1_Handler
                .long       SPI2_Handler
                .long       SPI3_Handler
                .long       EVENT_PORT1_Handler
                .long       EVENT_PORT2_Handler
                .long       EVENT_PORT3_Handler
                .long       EVENT_PORT4_Handler
                .long       MCANRAM_ECCER_Handler
                .long       MCAN1_INT0_Handler
                .long       MCAN1_INT1_Handler
                .long       MCAN2_INT0_Handler
                .long       MCAN2_INT1_Handler
                .long       MCAN3_INT0_Handler
                .long       MCAN3_INT1_Handler
                .long       USART1_Handler
                .long       USART1_TxComplete_Handler
                .long       USART2_Handler
                .long       USART2_TxComplete_Handler
                .long       USART3_Handler
                .long       USART3_TxComplete_Handler
                .long       USART4_Handler
                .long       USART4_TxComplete_Handler
                .long       HASH_Handler
                .long       SKE_Handler
                .long       SOGI_Handler
                .long       I2C1_Handler
                .long       I2C2_Handler
                .long       I2C3_Handler
                .long       SPI_SS0_WUPI_Handler
                .long       USART1_WUPI_Handler
                .long       LVD1_Handler
                .long       LVD2_Handler
                .long       FCM_Handler
                .long       WDT_Handler
                .long       CTC_Handler
                .long       ERMU_LPI_Handler
                .long       ADC1_Handler
                .long       ADC2_Handler
                .long       ADC3_Handler
                .long       ADC4_Handler
                .long       TRNG_Handler

__Vectors_End:
                .equ        __Vectors_Size, __Vectors_End - __Vectors
                .size       __Vectors, . - __Vectors
/*
;<h> Interrupt vector table end.
*/

/*
;<h> Reset handler start.
*/
                .section    .text.Reset_Handler
                .align      2
                .weak       Reset_Handler
                .type       Reset_Handler, %function
                .globl      Reset_Handler
Reset_Handler:
/* Single section scheme.
 *
 * The ranges of copy from/to are specified by following symbols
 *   __etext: LMA of start of the section to copy from. Usually end of text
 *   __data_start__: VMA of start of the section to copy to
 *   __data_end__: VMA of end of the section to copy to
 *
 * All addresses must be aligned to 4 bytes boundary.
 */
ClrSramSR:
                ldr         r0, =0x40050810
                ldr         r1, =0x7FEF0
                str         r1, [r0]

                /* Copy data from read only memory to RAM. */
CopyData:
                ldr         r1, =__etext
                ldr         r2, =__data_start__
                ldr         r3, =__data_end__
CopyLoop:
                cmp         r2, r3
                ittt        lt
                ldrlt       r0, [r1], #4
                strlt       r0, [r2], #4
                blt         CopyLoop

/* This part of work usually is done in C library startup code.
 * Otherwise, define this macro to enable it in this startup.
 *
 * There are two schemes too.
 * One can clear multiple BSS sections. Another can only clear one section.
 * The former is more size expensive than the latter.
 *
 * Define macro __STARTUP_CLEAR_BSS_MULTIPLE to choose the former.
 * Otherwise define macro __STARTUP_CLEAR_BSS to choose the later.
 */
/* Single BSS section scheme.
 *
 * The BSS section is specified by following symbols
 *   __bss_start__: start of the BSS section.
 *   __bss_end__: end of the BSS section.
 *
 * Both addresses must be aligned to 4 bytes boundary.
 */
                /* Clear BSS section. */
ClearBss:
                ldr         r1, =__bss_start__
                ldr         r2, =__bss_end__

                movs        r0, 0
ClearLoop:
                cmp         r1, r2
                itt         lt
                strlt       r0, [r1], #4
                blt         ClearLoop

                /* Call static constructors */
                bl          __libc_init_array
                /* Call the clock system initialization function. */
                bl          SystemInit
                /* Call the application's entry point. */
                bl          main
                bx          lr
                .size       Reset_Handler, . - Reset_Handler
/*
;<h> Reset handler end.
*/

/*
;<h> Default handler start.
*/
                .section    .text.Default_Handler, "ax", %progbits
                .align      2
Default_Handler:
                b           .
                .size       Default_Handler, . - Default_Handler
/*
;<h> Default handler end.
*/

/* Macro to define default exception/interrupt handlers.
 * Default handler are weak symbols with an endless loop.
 * They can be overwritten by real handlers.
 */
                .macro      Set_Default_Handler  Handler_Name
                .weak       \Handler_Name
                .set        \Handler_Name, Default_Handler
                .endm

/* Default exception/interrupt handler */

                Set_Default_Handler    NMI_Handler
                Set_Default_Handler    HardFault_Handler
                Set_Default_Handler    MemManage_Handler
                Set_Default_Handler    BusFault_Handler
                Set_Default_Handler    UsageFault_Handler
                Set_Default_Handler    SVC_Handler
                Set_Default_Handler    DebugMon_Handler
                Set_Default_Handler    PendSV_Handler
                Set_Default_Handler    SysTick_Handler

                Set_Default_Handler    IRQ000_Handler
                Set_Default_Handler    IRQ001_Handler
                Set_Default_Handler    IRQ002_Handler
                Set_Default_Handler    IRQ003_Handler
                Set_Default_Handler    IRQ004_Handler
                Set_Default_Handler    IRQ005_Handler
                Set_Default_Handler    IRQ006_Handler
                Set_Default_Handler    IRQ007_Handler
                Set_Default_Handler    IRQ008_Handler
                Set_Default_Handler    IRQ009_Handler
                Set_Default_Handler    IRQ010_Handler
                Set_Default_Handler    IRQ011_Handler
                Set_Default_Handler    IRQ012_Handler
                Set_Default_Handler    IRQ013_Handler
                Set_Default_Handler    IRQ014_Handler
                Set_Default_Handler    IRQ015_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ0_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ1_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ2_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ3_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ4_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ5_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ6_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ7_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ8_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ9_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ10_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ11_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ12_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ13_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ14_Handler
                Set_Default_Handler    EXTINT_PORT_EIRQ15_Handler
                Set_Default_Handler    DMA1_Error_Handler
                Set_Default_Handler    DMA1_TC0_BTC0_Handler
                Set_Default_Handler    DMA1_TC1_BTC1_Handler
                Set_Default_Handler    DMA1_TC2_BTC2_Handler
                Set_Default_Handler    DMA1_TC3_BTC3_Handler
                Set_Default_Handler    DMA1_TC4_BTC4_Handler
                Set_Default_Handler    DMA1_TC5_BTC5_Handler
                Set_Default_Handler    DMA1_TC6_BTC6_Handler
                Set_Default_Handler    DMA1_TC7_BTC7_Handler
                Set_Default_Handler    EFM_PEError_ReadCol_Handler
                Set_Default_Handler    EFM_OpEnd_Handler
                Set_Default_Handler    FPU_Error_Handler
                Set_Default_Handler    DMA2_Error_Handler
                Set_Default_Handler    DMA2_TC0_BTC0_Handler
                Set_Default_Handler    DMA2_TC1_BTC1_Handler
                Set_Default_Handler    DMA2_TC2_BTC2_Handler
                Set_Default_Handler    DMA2_TC3_BTC3_Handler
                Set_Default_Handler    DMA2_TC4_BTC4_Handler
                Set_Default_Handler    DMA2_TC5_BTC5_Handler
                Set_Default_Handler    DMA2_TC6_BTC6_Handler
                Set_Default_Handler    DMA2_TC7_BTC7_Handler
                Set_Default_Handler    PID_CALC_OV_Handler
                Set_Default_Handler    PID_CMP_Handler
                Set_Default_Handler    FMAC_FIR_Handler
                Set_Default_Handler    FMAC_IIR_Handler
                Set_Default_Handler    FMAC_RAM_ECC_ERR_Handler
                Set_Default_Handler    CORDIC_Handler
                Set_Default_Handler    TMR0_1_Handler
                Set_Default_Handler    TMR0_2_Handler
                Set_Default_Handler    XTAL_Handler
                Set_Default_Handler    SWDT_Handler
                Set_Default_Handler    TMR6_1_GCmp_Handler
                Set_Default_Handler    TMR6_1_Ovf_Udf_Handler
                Set_Default_Handler    TMR6_1_Dte_Handler
                Set_Default_Handler    TMR6_1_SCmp_Handler
                Set_Default_Handler    TMR6_2_GCmp_Handler
                Set_Default_Handler    TMR6_2_Ovf_Udf_Handler
                Set_Default_Handler    TMR6_2_Dte_Handler
                Set_Default_Handler    TMR6_2_SCmp_Handler
                Set_Default_Handler    TMR6_3_GCmp_Handler
                Set_Default_Handler    TMR6_3_Ovf_Udf_Handler
                Set_Default_Handler    TMR6_3_Dte_Handler
                Set_Default_Handler    TMR6_3_SCmp_Handler
                Set_Default_Handler    TMR6_4_GCmp_Handler
                Set_Default_Handler    TMR6_4_Ovf_Udf_Handler
                Set_Default_Handler    TMR6_4_Dte_Handler
                Set_Default_Handler    TMR6_4_SCmp_Handler
                Set_Default_Handler    TMR6_5_GCmp_Handler
                Set_Default_Handler    TMR6_5_Ovf_Udf_Handler
                Set_Default_Handler    TMR6_5_Dte_Handler
                Set_Default_Handler    TMR6_5_SCmp_Handler
                Set_Default_Handler    TMR6_6_GCmp_Handler
                Set_Default_Handler    TMR6_6_Ovf_Udf_Handler
                Set_Default_Handler    TMR6_6_Dte_Handler
                Set_Default_Handler    TMR6_6_SCmp_Handler
                Set_Default_Handler    TRLPWM_Handler
                Set_Default_Handler    EMB_GR0_Handler
                Set_Default_Handler    EMB_GR1_Handler
                Set_Default_Handler    EMB_GR2_Handler
                Set_Default_Handler    EMB_GR3_Handler
                Set_Default_Handler    EMB_GR4_Handler
                Set_Default_Handler    EMB_GR5_Handler
                Set_Default_Handler    HRPWM_1_Handler
                Set_Default_Handler    HRPWM_2_Handler
                Set_Default_Handler    HRPWM_3_Handler
                Set_Default_Handler    HRPWM_4_Handler
                Set_Default_Handler    HRPWM_5_Handler
                Set_Default_Handler    HRPWM_6_Handler
                Set_Default_Handler    HRPWM_7_Handler
                Set_Default_Handler    HRPWM_8_Handler
                Set_Default_Handler    HRPWM_EMB_Handler
                Set_Default_Handler    SDFM_FILTER0_Handler
                Set_Default_Handler    SDFM_FILTER1_Handler
                Set_Default_Handler    SDFM_FILTER2_Handler
                Set_Default_Handler    SDFM_FILTER3_Handler
                Set_Default_Handler    CMP1_Handler
                Set_Default_Handler    CMP2_Handler
                Set_Default_Handler    CMP3_Handler
                Set_Default_Handler    CMP4_Handler
                Set_Default_Handler    CMP5_Handler
                Set_Default_Handler    CMP6_Handler
                Set_Default_Handler    CMP7_Handler
                Set_Default_Handler    CMP8_Handler
                Set_Default_Handler    SPI1_Handler
                Set_Default_Handler    SPI2_Handler
                Set_Default_Handler    SPI3_Handler
                Set_Default_Handler    EVENT_PORT1_Handler
                Set_Default_Handler    EVENT_PORT2_Handler
                Set_Default_Handler    EVENT_PORT3_Handler
                Set_Default_Handler    EVENT_PORT4_Handler
                Set_Default_Handler    MCANRAM_ECCER_Handler
                Set_Default_Handler    MCAN1_INT0_Handler
                Set_Default_Handler    MCAN1_INT1_Handler
                Set_Default_Handler    MCAN2_INT0_Handler
                Set_Default_Handler    MCAN2_INT1_Handler
                Set_Default_Handler    MCAN3_INT0_Handler
                Set_Default_Handler    MCAN3_INT1_Handler
                Set_Default_Handler    USART1_Handler
                Set_Default_Handler    USART1_TxComplete_Handler
                Set_Default_Handler    USART2_Handler
                Set_Default_Handler    USART2_TxComplete_Handler
                Set_Default_Handler    USART3_Handler
                Set_Default_Handler    USART3_TxComplete_Handler
                Set_Default_Handler    USART4_Handler
                Set_Default_Handler    USART4_TxComplete_Handler
                Set_Default_Handler    HASH_Handler
                Set_Default_Handler    SKE_Handler
                Set_Default_Handler    SOGI_Handler
                Set_Default_Handler    I2C1_Handler
                Set_Default_Handler    I2C2_Handler
                Set_Default_Handler    I2C3_Handler
                Set_Default_Handler    SPI_SS0_WUPI_Handler
                Set_Default_Handler    USART1_WUPI_Handler
                Set_Default_Handler    LVD1_Handler
                Set_Default_Handler    LVD2_Handler
                Set_Default_Handler    FCM_Handler
                Set_Default_Handler    WDT_Handler
                Set_Default_Handler    CTC_Handler
                Set_Default_Handler    ERMU_LPI_Handler
                Set_Default_Handler    ADC1_Handler
                Set_Default_Handler    ADC2_Handler
                Set_Default_Handler    ADC3_Handler
                Set_Default_Handler    ADC4_Handler
                Set_Default_Handler    TRNG_Handler


                .end
