;/**
; ******************************************************************************
;  @file  startup_hc32f558.s
;  @brief Startup for IAR.
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

                MODULE  ?cstartup

                ;; Forward declaration of sections.
                SECTION CSTACK:DATA:NOROOT(3)

                SECTION .intvec:CODE:NOROOT(2)

                EXTERN  __iar_program_start
                EXTERN  SystemInit
                PUBLIC  __vector_table

                DATA
__vector_table
                DCD     sfe(CSTACK)               ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     MemManage_Handler         ; MPU Fault Handler
                DCD     BusFault_Handler          ; Bus Fault Handler
                DCD     UsageFault_Handler        ; Usage Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     DebugMon_Handler          ; Debug Monitor Handler
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; PendSV Handler
                DCD     SysTick_Handler           ; SysTick Handler

                ; Peripheral Interrupts
                DCD     IRQ000_Handler
                DCD     IRQ001_Handler
                DCD     IRQ002_Handler
                DCD     IRQ003_Handler
                DCD     IRQ004_Handler
                DCD     IRQ005_Handler
                DCD     IRQ006_Handler
                DCD     IRQ007_Handler
                DCD     IRQ008_Handler
                DCD     IRQ009_Handler
                DCD     IRQ010_Handler
                DCD     IRQ011_Handler
                DCD     IRQ012_Handler
                DCD     IRQ013_Handler
                DCD     IRQ014_Handler
                DCD     IRQ015_Handler
                DCD     EXTINT_PORT_EIRQ0_Handler
                DCD     EXTINT_PORT_EIRQ1_Handler
                DCD     EXTINT_PORT_EIRQ2_Handler
                DCD     EXTINT_PORT_EIRQ3_Handler
                DCD     EXTINT_PORT_EIRQ4_Handler
                DCD     EXTINT_PORT_EIRQ5_Handler
                DCD     EXTINT_PORT_EIRQ6_Handler
                DCD     EXTINT_PORT_EIRQ7_Handler
                DCD     EXTINT_PORT_EIRQ8_Handler
                DCD     EXTINT_PORT_EIRQ9_Handler
                DCD     EXTINT_PORT_EIRQ10_Handler
                DCD     EXTINT_PORT_EIRQ11_Handler
                DCD     EXTINT_PORT_EIRQ12_Handler
                DCD     EXTINT_PORT_EIRQ13_Handler
                DCD     EXTINT_PORT_EIRQ14_Handler
                DCD     EXTINT_PORT_EIRQ15_Handler
                DCD     DMA1_Error_Handler
                DCD     DMA1_TC0_BTC0_Handler
                DCD     DMA1_TC1_BTC1_Handler
                DCD     DMA1_TC2_BTC2_Handler
                DCD     DMA1_TC3_BTC3_Handler
                DCD     DMA1_TC4_BTC4_Handler
                DCD     DMA1_TC5_BTC5_Handler
                DCD     DMA1_TC6_BTC6_Handler
                DCD     DMA1_TC7_BTC7_Handler
                DCD     EFM_PEError_ReadCol_Handler
                DCD     EFM_OpEnd_Handler
                DCD     FPU_Error_Handler
                DCD     DMA2_Error_Handler
                DCD     DMA2_TC0_BTC0_Handler
                DCD     DMA2_TC1_BTC1_Handler
                DCD     DMA2_TC2_BTC2_Handler
                DCD     DMA2_TC3_BTC3_Handler
                DCD     DMA2_TC4_BTC4_Handler
                DCD     DMA2_TC5_BTC5_Handler
                DCD     DMA2_TC6_BTC6_Handler
                DCD     DMA2_TC7_BTC7_Handler
                DCD     PID_CALC_OV_Handler
                DCD     PID_CMP_Handler
                DCD     FMAC_FIR_Handler
                DCD     FMAC_IIR_Handler
                DCD     FMAC_RAM_ECC_ERR_Handler
                DCD     CORDIC_Handler
                DCD     TMR0_1_Handler
                DCD     TMR0_2_Handler
                DCD     XTAL_Handler
                DCD     SWDT_Handler
                DCD     TMR6_1_GCmp_Handler
                DCD     TMR6_1_Ovf_Udf_Handler
                DCD     TMR6_1_Dte_Handler
                DCD     TMR6_1_SCmp_Handler
                DCD     TMR6_2_GCmp_Handler
                DCD     TMR6_2_Ovf_Udf_Handler
                DCD     TMR6_2_Dte_Handler
                DCD     TMR6_2_SCmp_Handler
                DCD     TMR6_3_GCmp_Handler
                DCD     TMR6_3_Ovf_Udf_Handler
                DCD     TMR6_3_Dte_Handler
                DCD     TMR6_3_SCmp_Handler
                DCD     TMR6_4_GCmp_Handler
                DCD     TMR6_4_Ovf_Udf_Handler
                DCD     TMR6_4_Dte_Handler
                DCD     TMR6_4_SCmp_Handler
                DCD     TMR6_5_GCmp_Handler
                DCD     TMR6_5_Ovf_Udf_Handler
                DCD     TMR6_5_Dte_Handler
                DCD     TMR6_5_SCmp_Handler
                DCD     TMR6_6_GCmp_Handler
                DCD     TMR6_6_Ovf_Udf_Handler
                DCD     TMR6_6_Dte_Handler
                DCD     TMR6_6_SCmp_Handler
                DCD     TRLPWM_Handler
                DCD     EMB_GR0_Handler
                DCD     EMB_GR1_Handler
                DCD     EMB_GR2_Handler
                DCD     EMB_GR3_Handler
                DCD     EMB_GR4_Handler
                DCD     EMB_GR5_Handler
                DCD     HRPWM_1_Handler
                DCD     HRPWM_2_Handler
                DCD     HRPWM_3_Handler
                DCD     HRPWM_4_Handler
                DCD     HRPWM_5_Handler
                DCD     HRPWM_6_Handler
                DCD     HRPWM_7_Handler
                DCD     HRPWM_8_Handler
                DCD     HRPWM_EMB_Handler
                DCD     SDFM_FILTER0_Handler
                DCD     SDFM_FILTER1_Handler
                DCD     SDFM_FILTER2_Handler
                DCD     SDFM_FILTER3_Handler
                DCD     CMP1_Handler
                DCD     CMP2_Handler
                DCD     CMP3_Handler
                DCD     CMP4_Handler
                DCD     CMP5_Handler
                DCD     CMP6_Handler
                DCD     CMP7_Handler
                DCD     CMP8_Handler
                DCD     SPI1_Handler
                DCD     SPI2_Handler
                DCD     SPI3_Handler
                DCD     EVENT_PORT1_Handler
                DCD     EVENT_PORT2_Handler
                DCD     EVENT_PORT3_Handler
                DCD     EVENT_PORT4_Handler
                DCD     MCANRAM_ECCER_Handler
                DCD     MCAN1_INT0_Handler
                DCD     MCAN1_INT1_Handler
                DCD     MCAN2_INT0_Handler
                DCD     MCAN2_INT1_Handler
                DCD     MCAN3_INT0_Handler
                DCD     MCAN3_INT1_Handler
                DCD     USART1_Handler
                DCD     USART1_TxComplete_Handler
                DCD     USART2_Handler
                DCD     USART2_TxComplete_Handler
                DCD     USART3_Handler
                DCD     USART3_TxComplete_Handler
                DCD     USART4_Handler
                DCD     USART4_TxComplete_Handler
                DCD     HASH_Handler
                DCD     SKE_Handler
                DCD     SOGI_Handler
                DCD     I2C1_Handler
                DCD     I2C2_Handler
                DCD     I2C3_Handler
                DCD     SPI_SS0_WUPI_Handler
                DCD     USART1_WUPI_Handler
                DCD     LVD1_Handler
                DCD     LVD2_Handler
                DCD     FCM_Handler
                DCD     WDT_Handler
                DCD     CTC_Handler
                DCD     ERMU_LPI_Handler
                DCD     ADC1_Handler
                DCD     ADC2_Handler
                DCD     ADC3_Handler
                DCD     ADC4_Handler
                DCD     TRNG_Handler


                THUMB
; Dummy Exception Handlers (infinite loops which can be modified)

                PUBWEAK Reset_Handler
                SECTION .text:CODE:NOROOT:REORDER(2)
Reset_Handler
;ClrSramSR
                LDR     R0, =0x40050810
                LDR     R1, =0x7FEF0
                STR     R1, [R0]

                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__iar_program_start
                BX      R0

                PUBWEAK NMI_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
NMI_Handler
                B       NMI_Handler

                PUBWEAK HardFault_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HardFault_Handler
                B       HardFault_Handler

                PUBWEAK MemManage_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
MemManage_Handler
                B       MemManage_Handler

                PUBWEAK BusFault_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
BusFault_Handler
                B       BusFault_Handler

                PUBWEAK UsageFault_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
UsageFault_Handler
                B       UsageFault_Handler

                PUBWEAK SVC_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SVC_Handler
                B       SVC_Handler

                PUBWEAK DebugMon_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DebugMon_Handler
                B       DebugMon_Handler

                PUBWEAK PendSV_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
PendSV_Handler
                B       PendSV_Handler

                PUBWEAK SysTick_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SysTick_Handler
                B       SysTick_Handler

                PUBWEAK IRQ000_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ000_Handler
                B       IRQ000_Handler

                PUBWEAK IRQ001_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ001_Handler
                B       IRQ001_Handler

                PUBWEAK IRQ002_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ002_Handler
                B       IRQ002_Handler

                PUBWEAK IRQ003_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ003_Handler
                B       IRQ003_Handler

                PUBWEAK IRQ004_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ004_Handler
                B       IRQ004_Handler

                PUBWEAK IRQ005_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ005_Handler
                B       IRQ005_Handler

                PUBWEAK IRQ006_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ006_Handler
                B       IRQ006_Handler

                PUBWEAK IRQ007_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ007_Handler
                B       IRQ007_Handler

                PUBWEAK IRQ008_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ008_Handler
                B       IRQ008_Handler

                PUBWEAK IRQ009_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ009_Handler
                B       IRQ009_Handler

                PUBWEAK IRQ010_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ010_Handler
                B       IRQ010_Handler

                PUBWEAK IRQ011_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ011_Handler
                B       IRQ011_Handler

                PUBWEAK IRQ012_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ012_Handler
                B       IRQ012_Handler

                PUBWEAK IRQ013_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ013_Handler
                B       IRQ013_Handler

                PUBWEAK IRQ014_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ014_Handler
                B       IRQ014_Handler

                PUBWEAK IRQ015_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
IRQ015_Handler
                B       IRQ015_Handler

                PUBWEAK EXTINT_PORT_EIRQ0_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ0_Handler
                B       EXTINT_PORT_EIRQ0_Handler

                PUBWEAK EXTINT_PORT_EIRQ1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ1_Handler
                B       EXTINT_PORT_EIRQ1_Handler

                PUBWEAK EXTINT_PORT_EIRQ2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ2_Handler
                B       EXTINT_PORT_EIRQ2_Handler

                PUBWEAK EXTINT_PORT_EIRQ3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ3_Handler
                B       EXTINT_PORT_EIRQ3_Handler

                PUBWEAK EXTINT_PORT_EIRQ4_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ4_Handler
                B       EXTINT_PORT_EIRQ4_Handler

                PUBWEAK EXTINT_PORT_EIRQ5_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ5_Handler
                B       EXTINT_PORT_EIRQ5_Handler

                PUBWEAK EXTINT_PORT_EIRQ6_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ6_Handler
                B       EXTINT_PORT_EIRQ6_Handler

                PUBWEAK EXTINT_PORT_EIRQ7_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ7_Handler
                B       EXTINT_PORT_EIRQ7_Handler

                PUBWEAK EXTINT_PORT_EIRQ8_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ8_Handler
                B       EXTINT_PORT_EIRQ8_Handler

                PUBWEAK EXTINT_PORT_EIRQ9_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ9_Handler
                B       EXTINT_PORT_EIRQ9_Handler

                PUBWEAK EXTINT_PORT_EIRQ10_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ10_Handler
                B       EXTINT_PORT_EIRQ10_Handler

                PUBWEAK EXTINT_PORT_EIRQ11_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ11_Handler
                B       EXTINT_PORT_EIRQ11_Handler

                PUBWEAK EXTINT_PORT_EIRQ12_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ12_Handler
                B       EXTINT_PORT_EIRQ12_Handler

                PUBWEAK EXTINT_PORT_EIRQ13_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ13_Handler
                B       EXTINT_PORT_EIRQ13_Handler

                PUBWEAK EXTINT_PORT_EIRQ14_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ14_Handler
                B       EXTINT_PORT_EIRQ14_Handler

                PUBWEAK EXTINT_PORT_EIRQ15_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EXTINT_PORT_EIRQ15_Handler
                B       EXTINT_PORT_EIRQ15_Handler

                PUBWEAK DMA1_Error_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA1_Error_Handler
                B       DMA1_Error_Handler

                PUBWEAK DMA1_TC0_BTC0_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA1_TC0_BTC0_Handler
                B       DMA1_TC0_BTC0_Handler

                PUBWEAK DMA1_TC1_BTC1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA1_TC1_BTC1_Handler
                B       DMA1_TC1_BTC1_Handler

                PUBWEAK DMA1_TC2_BTC2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA1_TC2_BTC2_Handler
                B       DMA1_TC2_BTC2_Handler

                PUBWEAK DMA1_TC3_BTC3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA1_TC3_BTC3_Handler
                B       DMA1_TC3_BTC3_Handler

                PUBWEAK DMA1_TC4_BTC4_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA1_TC4_BTC4_Handler
                B       DMA1_TC4_BTC4_Handler

                PUBWEAK DMA1_TC5_BTC5_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA1_TC5_BTC5_Handler
                B       DMA1_TC5_BTC5_Handler

                PUBWEAK DMA1_TC6_BTC6_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA1_TC6_BTC6_Handler
                B       DMA1_TC6_BTC6_Handler

                PUBWEAK DMA1_TC7_BTC7_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA1_TC7_BTC7_Handler
                B       DMA1_TC7_BTC7_Handler

                PUBWEAK EFM_PEError_ReadCol_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EFM_PEError_ReadCol_Handler
                B       EFM_PEError_ReadCol_Handler

                PUBWEAK EFM_OpEnd_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EFM_OpEnd_Handler
                B       EFM_OpEnd_Handler

                PUBWEAK FPU_Error_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
FPU_Error_Handler
                B       FPU_Error_Handler

                PUBWEAK DMA2_Error_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA2_Error_Handler
                B       DMA2_Error_Handler

                PUBWEAK DMA2_TC0_BTC0_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA2_TC0_BTC0_Handler
                B       DMA2_TC0_BTC0_Handler

                PUBWEAK DMA2_TC1_BTC1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA2_TC1_BTC1_Handler
                B       DMA2_TC1_BTC1_Handler

                PUBWEAK DMA2_TC2_BTC2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA2_TC2_BTC2_Handler
                B       DMA2_TC2_BTC2_Handler

                PUBWEAK DMA2_TC3_BTC3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA2_TC3_BTC3_Handler
                B       DMA2_TC3_BTC3_Handler

                PUBWEAK DMA2_TC4_BTC4_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA2_TC4_BTC4_Handler
                B       DMA2_TC4_BTC4_Handler

                PUBWEAK DMA2_TC5_BTC5_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA2_TC5_BTC5_Handler
                B       DMA2_TC5_BTC5_Handler

                PUBWEAK DMA2_TC6_BTC6_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA2_TC6_BTC6_Handler
                B       DMA2_TC6_BTC6_Handler

                PUBWEAK DMA2_TC7_BTC7_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
DMA2_TC7_BTC7_Handler
                B       DMA2_TC7_BTC7_Handler

                PUBWEAK PID_CALC_OV_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
PID_CALC_OV_Handler
                B       PID_CALC_OV_Handler

                PUBWEAK PID_CMP_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
PID_CMP_Handler
                B       PID_CMP_Handler

                PUBWEAK FMAC_FIR_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
FMAC_FIR_Handler
                B       FMAC_FIR_Handler

                PUBWEAK FMAC_IIR_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
FMAC_IIR_Handler
                B       FMAC_IIR_Handler

                PUBWEAK FMAC_RAM_ECC_ERR_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
FMAC_RAM_ECC_ERR_Handler
                B       FMAC_RAM_ECC_ERR_Handler

                PUBWEAK CORDIC_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CORDIC_Handler
                B       CORDIC_Handler

                PUBWEAK TMR0_1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR0_1_Handler
                B       TMR0_1_Handler

                PUBWEAK TMR0_2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR0_2_Handler
                B       TMR0_2_Handler

                PUBWEAK XTAL_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
XTAL_Handler
                B       XTAL_Handler

                PUBWEAK SWDT_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SWDT_Handler
                B       SWDT_Handler

                PUBWEAK TMR6_1_GCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_1_GCmp_Handler
                B       TMR6_1_GCmp_Handler

                PUBWEAK TMR6_1_Ovf_Udf_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_1_Ovf_Udf_Handler
                B       TMR6_1_Ovf_Udf_Handler

                PUBWEAK TMR6_1_Dte_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_1_Dte_Handler
                B       TMR6_1_Dte_Handler

                PUBWEAK TMR6_1_SCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_1_SCmp_Handler
                B       TMR6_1_SCmp_Handler

                PUBWEAK TMR6_2_GCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_2_GCmp_Handler
                B       TMR6_2_GCmp_Handler

                PUBWEAK TMR6_2_Ovf_Udf_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_2_Ovf_Udf_Handler
                B       TMR6_2_Ovf_Udf_Handler

                PUBWEAK TMR6_2_Dte_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_2_Dte_Handler
                B       TMR6_2_Dte_Handler

                PUBWEAK TMR6_2_SCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_2_SCmp_Handler
                B       TMR6_2_SCmp_Handler

                PUBWEAK TMR6_3_GCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_3_GCmp_Handler
                B       TMR6_3_GCmp_Handler

                PUBWEAK TMR6_3_Ovf_Udf_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_3_Ovf_Udf_Handler
                B       TMR6_3_Ovf_Udf_Handler

                PUBWEAK TMR6_3_Dte_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_3_Dte_Handler
                B       TMR6_3_Dte_Handler

                PUBWEAK TMR6_3_SCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_3_SCmp_Handler
                B       TMR6_3_SCmp_Handler

                PUBWEAK TMR6_4_GCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_4_GCmp_Handler
                B       TMR6_4_GCmp_Handler

                PUBWEAK TMR6_4_Ovf_Udf_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_4_Ovf_Udf_Handler
                B       TMR6_4_Ovf_Udf_Handler

                PUBWEAK TMR6_4_Dte_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_4_Dte_Handler
                B       TMR6_4_Dte_Handler

                PUBWEAK TMR6_4_SCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_4_SCmp_Handler
                B       TMR6_4_SCmp_Handler

                PUBWEAK TMR6_5_GCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_5_GCmp_Handler
                B       TMR6_5_GCmp_Handler

                PUBWEAK TMR6_5_Ovf_Udf_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_5_Ovf_Udf_Handler
                B       TMR6_5_Ovf_Udf_Handler

                PUBWEAK TMR6_5_Dte_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_5_Dte_Handler
                B       TMR6_5_Dte_Handler

                PUBWEAK TMR6_5_SCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_5_SCmp_Handler
                B       TMR6_5_SCmp_Handler

                PUBWEAK TMR6_6_GCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_6_GCmp_Handler
                B       TMR6_6_GCmp_Handler

                PUBWEAK TMR6_6_Ovf_Udf_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_6_Ovf_Udf_Handler
                B       TMR6_6_Ovf_Udf_Handler

                PUBWEAK TMR6_6_Dte_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_6_Dte_Handler
                B       TMR6_6_Dte_Handler

                PUBWEAK TMR6_6_SCmp_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TMR6_6_SCmp_Handler
                B       TMR6_6_SCmp_Handler

                PUBWEAK TRLPWM_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TRLPWM_Handler
                B       TRLPWM_Handler

                PUBWEAK EMB_GR0_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EMB_GR0_Handler
                B       EMB_GR0_Handler

                PUBWEAK EMB_GR1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EMB_GR1_Handler
                B       EMB_GR1_Handler

                PUBWEAK EMB_GR2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EMB_GR2_Handler
                B       EMB_GR2_Handler

                PUBWEAK EMB_GR3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EMB_GR3_Handler
                B       EMB_GR3_Handler

                PUBWEAK EMB_GR4_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EMB_GR4_Handler
                B       EMB_GR4_Handler

                PUBWEAK EMB_GR5_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EMB_GR5_Handler
                B       EMB_GR5_Handler

                PUBWEAK HRPWM_1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HRPWM_1_Handler
                B       HRPWM_1_Handler

                PUBWEAK HRPWM_2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HRPWM_2_Handler
                B       HRPWM_2_Handler

                PUBWEAK HRPWM_3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HRPWM_3_Handler
                B       HRPWM_3_Handler

                PUBWEAK HRPWM_4_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HRPWM_4_Handler
                B       HRPWM_4_Handler

                PUBWEAK HRPWM_5_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HRPWM_5_Handler
                B       HRPWM_5_Handler

                PUBWEAK HRPWM_6_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HRPWM_6_Handler
                B       HRPWM_6_Handler

                PUBWEAK HRPWM_7_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HRPWM_7_Handler
                B       HRPWM_7_Handler

                PUBWEAK HRPWM_8_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HRPWM_8_Handler
                B       HRPWM_8_Handler

                PUBWEAK HRPWM_EMB_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HRPWM_EMB_Handler
                B       HRPWM_EMB_Handler

                PUBWEAK SDFM_FILTER0_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SDFM_FILTER0_Handler
                B       SDFM_FILTER0_Handler

                PUBWEAK SDFM_FILTER1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SDFM_FILTER1_Handler
                B       SDFM_FILTER1_Handler

                PUBWEAK SDFM_FILTER2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SDFM_FILTER2_Handler
                B       SDFM_FILTER2_Handler

                PUBWEAK SDFM_FILTER3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SDFM_FILTER3_Handler
                B       SDFM_FILTER3_Handler

                PUBWEAK CMP1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CMP1_Handler
                B       CMP1_Handler

                PUBWEAK CMP2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CMP2_Handler
                B       CMP2_Handler

                PUBWEAK CMP3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CMP3_Handler
                B       CMP3_Handler

                PUBWEAK CMP4_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CMP4_Handler
                B       CMP4_Handler

                PUBWEAK CMP5_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CMP5_Handler
                B       CMP5_Handler

                PUBWEAK CMP6_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CMP6_Handler
                B       CMP6_Handler

                PUBWEAK CMP7_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CMP7_Handler
                B       CMP7_Handler

                PUBWEAK CMP8_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CMP8_Handler
                B       CMP8_Handler

                PUBWEAK SPI1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SPI1_Handler
                B       SPI1_Handler

                PUBWEAK SPI2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SPI2_Handler
                B       SPI2_Handler

                PUBWEAK SPI3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SPI3_Handler
                B       SPI3_Handler

                PUBWEAK EVENT_PORT1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EVENT_PORT1_Handler
                B       EVENT_PORT1_Handler

                PUBWEAK EVENT_PORT2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EVENT_PORT2_Handler
                B       EVENT_PORT2_Handler

                PUBWEAK EVENT_PORT3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EVENT_PORT3_Handler
                B       EVENT_PORT3_Handler

                PUBWEAK EVENT_PORT4_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
EVENT_PORT4_Handler
                B       EVENT_PORT4_Handler

                PUBWEAK MCANRAM_ECCER_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
MCANRAM_ECCER_Handler
                B       MCANRAM_ECCER_Handler

                PUBWEAK MCAN1_INT0_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
MCAN1_INT0_Handler
                B       MCAN1_INT0_Handler

                PUBWEAK MCAN1_INT1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
MCAN1_INT1_Handler
                B       MCAN1_INT1_Handler

                PUBWEAK MCAN2_INT0_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
MCAN2_INT0_Handler
                B       MCAN2_INT0_Handler

                PUBWEAK MCAN2_INT1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
MCAN2_INT1_Handler
                B       MCAN2_INT1_Handler

                PUBWEAK MCAN3_INT0_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
MCAN3_INT0_Handler
                B       MCAN3_INT0_Handler

                PUBWEAK MCAN3_INT1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
MCAN3_INT1_Handler
                B       MCAN3_INT1_Handler

                PUBWEAK USART1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
USART1_Handler
                B       USART1_Handler

                PUBWEAK USART1_TxComplete_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
USART1_TxComplete_Handler
                B       USART1_TxComplete_Handler

                PUBWEAK USART2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
USART2_Handler
                B       USART2_Handler

                PUBWEAK USART2_TxComplete_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
USART2_TxComplete_Handler
                B       USART2_TxComplete_Handler

                PUBWEAK USART3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
USART3_Handler
                B       USART3_Handler

                PUBWEAK USART3_TxComplete_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
USART3_TxComplete_Handler
                B       USART3_TxComplete_Handler

                PUBWEAK USART4_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
USART4_Handler
                B       USART4_Handler

                PUBWEAK USART4_TxComplete_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
USART4_TxComplete_Handler
                B       USART4_TxComplete_Handler

                PUBWEAK HASH_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
HASH_Handler
                B       HASH_Handler

                PUBWEAK SKE_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SKE_Handler
                B       SKE_Handler

                PUBWEAK SOGI_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SOGI_Handler
                B       SOGI_Handler

                PUBWEAK I2C1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
I2C1_Handler
                B       I2C1_Handler

                PUBWEAK I2C2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
I2C2_Handler
                B       I2C2_Handler

                PUBWEAK I2C3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
I2C3_Handler
                B       I2C3_Handler

                PUBWEAK SPI_SS0_WUPI_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
SPI_SS0_WUPI_Handler
                B       SPI_SS0_WUPI_Handler

                PUBWEAK USART1_WUPI_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
USART1_WUPI_Handler
                B       USART1_WUPI_Handler

                PUBWEAK LVD1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
LVD1_Handler
                B       LVD1_Handler

                PUBWEAK LVD2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
LVD2_Handler
                B       LVD2_Handler

                PUBWEAK FCM_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
FCM_Handler
                B       FCM_Handler

                PUBWEAK WDT_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
WDT_Handler
                B       WDT_Handler

                PUBWEAK CTC_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
CTC_Handler
                B       CTC_Handler

                PUBWEAK ERMU_LPI_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
ERMU_LPI_Handler
                B       ERMU_LPI_Handler

                PUBWEAK ADC1_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
ADC1_Handler
                B       ADC1_Handler

                PUBWEAK ADC2_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
ADC2_Handler
                B       ADC2_Handler

                PUBWEAK ADC3_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
ADC3_Handler
                B       ADC3_Handler

                PUBWEAK ADC4_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
ADC4_Handler
                B       ADC4_Handler

                PUBWEAK TRNG_Handler
                SECTION .text:CODE:NOROOT:REORDER(1)
TRNG_Handler
                B       TRNG_Handler

                END
