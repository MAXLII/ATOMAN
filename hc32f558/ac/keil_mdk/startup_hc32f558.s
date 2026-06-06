;/**
; ******************************************************************************
;  @file  startup_hc32f558.s
;  @brief Startup for MDK.
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

; Stack Configuration
; Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>

Stack_Size      EQU     0x00002000

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


; Heap Configuration
;  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>

Heap_Size       EQU     0x00002000

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB

; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp              ; Top of Stack
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

__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  SystemInit
                IMPORT  __main
;ClrSramSR
                LDR     R0, =0x40050810
                LDR     R1, =0x7FEF0
                STR     R1, [R0]

                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP

; Dummy Exception Handlers (infinite loops which can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler         [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler          [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler        [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler          [WEAK]
                B       .
                ENDP
PendSV_Handler\
                PROC
                EXPORT  PendSV_Handler            [WEAK]
                B       .
                ENDP
SysTick_Handler\
                PROC
                EXPORT  SysTick_Handler           [WEAK]
                B       .
                ENDP

Default_Handler PROC
                EXPORT  IRQ000_Handler                  [WEAK]
                EXPORT  IRQ001_Handler                  [WEAK]
                EXPORT  IRQ002_Handler                  [WEAK]
                EXPORT  IRQ003_Handler                  [WEAK]
                EXPORT  IRQ004_Handler                  [WEAK]
                EXPORT  IRQ005_Handler                  [WEAK]
                EXPORT  IRQ006_Handler                  [WEAK]
                EXPORT  IRQ007_Handler                  [WEAK]
                EXPORT  IRQ008_Handler                  [WEAK]
                EXPORT  IRQ009_Handler                  [WEAK]
                EXPORT  IRQ010_Handler                  [WEAK]
                EXPORT  IRQ011_Handler                  [WEAK]
                EXPORT  IRQ012_Handler                  [WEAK]
                EXPORT  IRQ013_Handler                  [WEAK]
                EXPORT  IRQ014_Handler                  [WEAK]
                EXPORT  IRQ015_Handler                  [WEAK]
                EXPORT  EXTINT_PORT_EIRQ0_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ1_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ2_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ3_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ4_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ5_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ6_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ7_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ8_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ9_Handler       [WEAK]
                EXPORT  EXTINT_PORT_EIRQ10_Handler      [WEAK]
                EXPORT  EXTINT_PORT_EIRQ11_Handler      [WEAK]
                EXPORT  EXTINT_PORT_EIRQ12_Handler      [WEAK]
                EXPORT  EXTINT_PORT_EIRQ13_Handler      [WEAK]
                EXPORT  EXTINT_PORT_EIRQ14_Handler      [WEAK]
                EXPORT  EXTINT_PORT_EIRQ15_Handler      [WEAK]
                EXPORT  DMA1_Error_Handler              [WEAK]
                EXPORT  DMA1_TC0_BTC0_Handler           [WEAK]
                EXPORT  DMA1_TC1_BTC1_Handler           [WEAK]
                EXPORT  DMA1_TC2_BTC2_Handler           [WEAK]
                EXPORT  DMA1_TC3_BTC3_Handler           [WEAK]
                EXPORT  DMA1_TC4_BTC4_Handler           [WEAK]
                EXPORT  DMA1_TC5_BTC5_Handler           [WEAK]
                EXPORT  DMA1_TC6_BTC6_Handler           [WEAK]
                EXPORT  DMA1_TC7_BTC7_Handler           [WEAK]
                EXPORT  EFM_PEError_ReadCol_Handler     [WEAK]
                EXPORT  EFM_OpEnd_Handler               [WEAK]
                EXPORT  FPU_Error_Handler               [WEAK]
                EXPORT  DMA2_Error_Handler              [WEAK]
                EXPORT  DMA2_TC0_BTC0_Handler           [WEAK]
                EXPORT  DMA2_TC1_BTC1_Handler           [WEAK]
                EXPORT  DMA2_TC2_BTC2_Handler           [WEAK]
                EXPORT  DMA2_TC3_BTC3_Handler           [WEAK]
                EXPORT  DMA2_TC4_BTC4_Handler           [WEAK]
                EXPORT  DMA2_TC5_BTC5_Handler           [WEAK]
                EXPORT  DMA2_TC6_BTC6_Handler           [WEAK]
                EXPORT  DMA2_TC7_BTC7_Handler           [WEAK]
                EXPORT  PID_CALC_OV_Handler             [WEAK]
                EXPORT  PID_CMP_Handler                 [WEAK]
                EXPORT  FMAC_FIR_Handler                [WEAK]
                EXPORT  FMAC_IIR_Handler                [WEAK]
                EXPORT  FMAC_RAM_ECC_ERR_Handler        [WEAK]
                EXPORT  CORDIC_Handler                  [WEAK]
                EXPORT  TMR0_1_Handler                  [WEAK]
                EXPORT  TMR0_2_Handler                  [WEAK]
                EXPORT  XTAL_Handler                    [WEAK]
                EXPORT  SWDT_Handler                    [WEAK]
                EXPORT  TMR6_1_GCmp_Handler             [WEAK]
                EXPORT  TMR6_1_Ovf_Udf_Handler          [WEAK]
                EXPORT  TMR6_1_Dte_Handler              [WEAK]
                EXPORT  TMR6_1_SCmp_Handler             [WEAK]
                EXPORT  TMR6_2_GCmp_Handler             [WEAK]
                EXPORT  TMR6_2_Ovf_Udf_Handler          [WEAK]
                EXPORT  TMR6_2_Dte_Handler              [WEAK]
                EXPORT  TMR6_2_SCmp_Handler             [WEAK]
                EXPORT  TMR6_3_GCmp_Handler             [WEAK]
                EXPORT  TMR6_3_Ovf_Udf_Handler          [WEAK]
                EXPORT  TMR6_3_Dte_Handler              [WEAK]
                EXPORT  TMR6_3_SCmp_Handler             [WEAK]
                EXPORT  TMR6_4_GCmp_Handler             [WEAK]
                EXPORT  TMR6_4_Ovf_Udf_Handler          [WEAK]
                EXPORT  TMR6_4_Dte_Handler              [WEAK]
                EXPORT  TMR6_4_SCmp_Handler             [WEAK]
                EXPORT  TMR6_5_GCmp_Handler             [WEAK]
                EXPORT  TMR6_5_Ovf_Udf_Handler          [WEAK]
                EXPORT  TMR6_5_Dte_Handler              [WEAK]
                EXPORT  TMR6_5_SCmp_Handler             [WEAK]
                EXPORT  TMR6_6_GCmp_Handler             [WEAK]
                EXPORT  TMR6_6_Ovf_Udf_Handler          [WEAK]
                EXPORT  TMR6_6_Dte_Handler              [WEAK]
                EXPORT  TMR6_6_SCmp_Handler             [WEAK]
                EXPORT  TRLPWM_Handler                  [WEAK]
                EXPORT  EMB_GR0_Handler                 [WEAK]
                EXPORT  EMB_GR1_Handler                 [WEAK]
                EXPORT  EMB_GR2_Handler                 [WEAK]
                EXPORT  EMB_GR3_Handler                 [WEAK]
                EXPORT  EMB_GR4_Handler                 [WEAK]
                EXPORT  EMB_GR5_Handler                 [WEAK]
                EXPORT  HRPWM_1_Handler                 [WEAK]
                EXPORT  HRPWM_2_Handler                 [WEAK]
                EXPORT  HRPWM_3_Handler                 [WEAK]
                EXPORT  HRPWM_4_Handler                 [WEAK]
                EXPORT  HRPWM_5_Handler                 [WEAK]
                EXPORT  HRPWM_6_Handler                 [WEAK]
                EXPORT  HRPWM_7_Handler                 [WEAK]
                EXPORT  HRPWM_8_Handler                 [WEAK]
                EXPORT  HRPWM_EMB_Handler               [WEAK]
                EXPORT  SDFM_FILTER0_Handler            [WEAK]
                EXPORT  SDFM_FILTER1_Handler            [WEAK]
                EXPORT  SDFM_FILTER2_Handler            [WEAK]
                EXPORT  SDFM_FILTER3_Handler            [WEAK]
                EXPORT  CMP1_Handler                    [WEAK]
                EXPORT  CMP2_Handler                    [WEAK]
                EXPORT  CMP3_Handler                    [WEAK]
                EXPORT  CMP4_Handler                    [WEAK]
                EXPORT  CMP5_Handler                    [WEAK]
                EXPORT  CMP6_Handler                    [WEAK]
                EXPORT  CMP7_Handler                    [WEAK]
                EXPORT  CMP8_Handler                    [WEAK]
                EXPORT  SPI1_Handler                    [WEAK]
                EXPORT  SPI2_Handler                    [WEAK]
                EXPORT  SPI3_Handler                    [WEAK]
                EXPORT  EVENT_PORT1_Handler             [WEAK]
                EXPORT  EVENT_PORT2_Handler             [WEAK]
                EXPORT  EVENT_PORT3_Handler             [WEAK]
                EXPORT  EVENT_PORT4_Handler             [WEAK]
                EXPORT  MCANRAM_ECCER_Handler           [WEAK]
                EXPORT  MCAN1_INT0_Handler              [WEAK]
                EXPORT  MCAN1_INT1_Handler              [WEAK]
                EXPORT  MCAN2_INT0_Handler              [WEAK]
                EXPORT  MCAN2_INT1_Handler              [WEAK]
                EXPORT  MCAN3_INT0_Handler              [WEAK]
                EXPORT  MCAN3_INT1_Handler              [WEAK]
                EXPORT  USART1_Handler                  [WEAK]
                EXPORT  USART1_TxComplete_Handler       [WEAK]
                EXPORT  USART2_Handler                  [WEAK]
                EXPORT  USART2_TxComplete_Handler       [WEAK]
                EXPORT  USART3_Handler                  [WEAK]
                EXPORT  USART3_TxComplete_Handler       [WEAK]
                EXPORT  USART4_Handler                  [WEAK]
                EXPORT  USART4_TxComplete_Handler       [WEAK]
                EXPORT  HASH_Handler                    [WEAK]
                EXPORT  SKE_Handler                     [WEAK]
                EXPORT  SOGI_Handler                    [WEAK]
                EXPORT  I2C1_Handler                    [WEAK]
                EXPORT  I2C2_Handler                    [WEAK]
                EXPORT  I2C3_Handler                    [WEAK]
                EXPORT  SPI_SS0_WUPI_Handler            [WEAK]
                EXPORT  USART1_WUPI_Handler             [WEAK]
                EXPORT  LVD1_Handler                    [WEAK]
                EXPORT  LVD2_Handler                    [WEAK]
                EXPORT  FCM_Handler                     [WEAK]
                EXPORT  WDT_Handler                     [WEAK]
                EXPORT  CTC_Handler                     [WEAK]
                EXPORT  ERMU_LPI_Handler                [WEAK]
                EXPORT  ADC1_Handler                    [WEAK]
                EXPORT  ADC2_Handler                    [WEAK]
                EXPORT  ADC3_Handler                    [WEAK]
                EXPORT  ADC4_Handler                    [WEAK]
                EXPORT  TRNG_Handler                    [WEAK]

IRQ000_Handler
IRQ001_Handler
IRQ002_Handler
IRQ003_Handler
IRQ004_Handler
IRQ005_Handler
IRQ006_Handler
IRQ007_Handler
IRQ008_Handler
IRQ009_Handler
IRQ010_Handler
IRQ011_Handler
IRQ012_Handler
IRQ013_Handler
IRQ014_Handler
IRQ015_Handler
EXTINT_PORT_EIRQ0_Handler
EXTINT_PORT_EIRQ1_Handler
EXTINT_PORT_EIRQ2_Handler
EXTINT_PORT_EIRQ3_Handler
EXTINT_PORT_EIRQ4_Handler
EXTINT_PORT_EIRQ5_Handler
EXTINT_PORT_EIRQ6_Handler
EXTINT_PORT_EIRQ7_Handler
EXTINT_PORT_EIRQ8_Handler
EXTINT_PORT_EIRQ9_Handler
EXTINT_PORT_EIRQ10_Handler
EXTINT_PORT_EIRQ11_Handler
EXTINT_PORT_EIRQ12_Handler
EXTINT_PORT_EIRQ13_Handler
EXTINT_PORT_EIRQ14_Handler
EXTINT_PORT_EIRQ15_Handler
DMA1_Error_Handler
DMA1_TC0_BTC0_Handler
DMA1_TC1_BTC1_Handler
DMA1_TC2_BTC2_Handler
DMA1_TC3_BTC3_Handler
DMA1_TC4_BTC4_Handler
DMA1_TC5_BTC5_Handler
DMA1_TC6_BTC6_Handler
DMA1_TC7_BTC7_Handler
EFM_PEError_ReadCol_Handler
EFM_OpEnd_Handler
FPU_Error_Handler
DMA2_Error_Handler
DMA2_TC0_BTC0_Handler
DMA2_TC1_BTC1_Handler
DMA2_TC2_BTC2_Handler
DMA2_TC3_BTC3_Handler
DMA2_TC4_BTC4_Handler
DMA2_TC5_BTC5_Handler
DMA2_TC6_BTC6_Handler
DMA2_TC7_BTC7_Handler
PID_CALC_OV_Handler
PID_CMP_Handler
FMAC_FIR_Handler
FMAC_IIR_Handler
FMAC_RAM_ECC_ERR_Handler
CORDIC_Handler
TMR0_1_Handler
TMR0_2_Handler
XTAL_Handler
SWDT_Handler
TMR6_1_GCmp_Handler
TMR6_1_Ovf_Udf_Handler
TMR6_1_Dte_Handler
TMR6_1_SCmp_Handler
TMR6_2_GCmp_Handler
TMR6_2_Ovf_Udf_Handler
TMR6_2_Dte_Handler
TMR6_2_SCmp_Handler
TMR6_3_GCmp_Handler
TMR6_3_Ovf_Udf_Handler
TMR6_3_Dte_Handler
TMR6_3_SCmp_Handler
TMR6_4_GCmp_Handler
TMR6_4_Ovf_Udf_Handler
TMR6_4_Dte_Handler
TMR6_4_SCmp_Handler
TMR6_5_GCmp_Handler
TMR6_5_Ovf_Udf_Handler
TMR6_5_Dte_Handler
TMR6_5_SCmp_Handler
TMR6_6_GCmp_Handler
TMR6_6_Ovf_Udf_Handler
TMR6_6_Dte_Handler
TMR6_6_SCmp_Handler
TRLPWM_Handler
EMB_GR0_Handler
EMB_GR1_Handler
EMB_GR2_Handler
EMB_GR3_Handler
EMB_GR4_Handler
EMB_GR5_Handler
HRPWM_1_Handler
HRPWM_2_Handler
HRPWM_3_Handler
HRPWM_4_Handler
HRPWM_5_Handler
HRPWM_6_Handler
HRPWM_7_Handler
HRPWM_8_Handler
HRPWM_EMB_Handler
SDFM_FILTER0_Handler
SDFM_FILTER1_Handler
SDFM_FILTER2_Handler
SDFM_FILTER3_Handler
CMP1_Handler
CMP2_Handler
CMP3_Handler
CMP4_Handler
CMP5_Handler
CMP6_Handler
CMP7_Handler
CMP8_Handler
SPI1_Handler
SPI2_Handler
SPI3_Handler
EVENT_PORT1_Handler
EVENT_PORT2_Handler
EVENT_PORT3_Handler
EVENT_PORT4_Handler
MCANRAM_ECCER_Handler
MCAN1_INT0_Handler
MCAN1_INT1_Handler
MCAN2_INT0_Handler
MCAN2_INT1_Handler
MCAN3_INT0_Handler
MCAN3_INT1_Handler
USART1_Handler
USART1_TxComplete_Handler
USART2_Handler
USART2_TxComplete_Handler
USART3_Handler
USART3_TxComplete_Handler
USART4_Handler
USART4_TxComplete_Handler
HASH_Handler
SKE_Handler
SOGI_Handler
I2C1_Handler
I2C2_Handler
I2C3_Handler
SPI_SS0_WUPI_Handler
USART1_WUPI_Handler
LVD1_Handler
LVD2_Handler
FCM_Handler
WDT_Handler
CTC_Handler
ERMU_LPI_Handler
ADC1_Handler
ADC2_Handler
ADC3_Handler
ADC4_Handler
TRNG_Handler

                B .

                ENDP

                ALIGN


; User Initial Stack & Heap

                IF      :DEF:__MICROLIB

                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit

                ELSE

                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap

__user_initial_stackheap
                LDR     R0, =  Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR

                ALIGN

                ENDIF


                END
