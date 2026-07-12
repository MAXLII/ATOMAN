.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

.section .vectors, "a", %progbits
.align 2
.global __Vectors
.global __Vectors_End
.global __Vectors_Size
.type __Vectors, %object
__Vectors:
    .word __StackTop
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    .word IRQ000_Handler
    .word IRQ001_Handler
    .word IRQ002_Handler
    .word IRQ003_Handler
    .word IRQ004_Handler
    .word IRQ005_Handler
    .word IRQ006_Handler
    .word IRQ007_Handler
    .word IRQ008_Handler
    .word IRQ009_Handler
    .word IRQ010_Handler
    .word IRQ011_Handler
    .word IRQ012_Handler
    .word IRQ013_Handler
    .word IRQ014_Handler
    .word IRQ015_Handler
    .word EXTINT00_SWINT16_Handler
    .word EXTINT01_SWINT17_Handler
    .word EXTINT02_SWINT18_Handler
    .word EXTINT03_SWINT19_Handler
    .word EXTINT04_SWINT20_Handler
    .word EXTINT05_SWINT21_Handler
    .word EXTINT06_SWINT22_Handler
    .word EXTINT07_SWINT23_Handler
    .word EXTINT08_SWINT24_Handler
    .word EXTINT09_SWINT25_Handler
    .word EXTINT10_SWINT26_Handler
    .word EXTINT11_SWINT27_Handler
    .word EXTINT12_SWINT28_Handler
    .word EXTINT13_SWINT29_Handler
    .word EXTINT14_SWINT30_Handler
    .word EXTINT15_SWINT31_Handler
    .word DMA_Error_Handler
    .word DMA_TC0_BTC0_Handler
    .word DMA_TC1_BTC1_Handler
    .word DMA_TC2_BTC2_Handler
    .word DMA_TC3_BTC3_Handler
    .word DMA_TC4_BTC4_Handler
    .word DMA_TC5_BTC5_Handler
    .word DMA_TC6_BTC6_Handler
    .word DMA_TC7_BTC7_Handler
    .word EFM_PEError_ReadCol_Handler
    .word EFM_OpEnd_Handler
    .word FPU_Error_Handler
    .word TMR0_1_Handler
    .word TMR0_2_Handler
    .word RTC_Handler
    .word CLK_XtalStop_Handler
    .word PWC_WKTM_Handler
    .word SWDT_Handler
    .word TMR6_1_GCmp_Handler
    .word TMR6_1_Ovf_Udf_Handler
    .word TMR6_1_Dte_Handler
    .word TMR6_1_SCmp_Handler
    .word TMRA_1_Ovf_Udf_Handler
    .word TMRA_1_Cmp_Handler
    .word TMR6_2_GCmp_Handler
    .word TMR6_2_Ovf_Udf_Handler
    .word TMR6_2_Dte_Handler
    .word TMR6_2_SCmp_Handler
    .word TMRA_2_Ovf_Udf_Handler
    .word TMRA_2_Cmp_Handler
    .word TMR6_3_GCmp_Handler
    .word TMR6_3_Ovf_Udf_Handler
    .word TMR6_3_Dte_Handler
    .word TMR6_3_SCmp_Handler
    .word TMRA_3_Ovf_Udf_Handler
    .word TMRA_3_Cmp_Handler
    .word TMR6_4_GCmp_Handler
    .word TMR6_4_Ovf_Udf_Handler
    .word TMR6_4_Dte_Handler
    .word TMR6_4_SCmp_Handler
    .word TMRA_4_Ovf_Udf_Handler
    .word TMRA_4_Cmp_Handler
    .word HRPWM_1_GCmp_Handler
    .word HRPWM_1_Ovf_Udf_Handler
    .word HRPWM_1_SCmp_Handler
    .word HRPWM_1_GCap_Handler
    .word HRPWM_2_GCmp_Handler
    .word HRPWM_2_Ovf_Udf_Handler
    .word HRPWM_2_SCmp_Handler
    .word HRPWM_2_GCap_Handler
    .word HRPWM_Handler
    .word EMB_GR0_Handler
    .word EMB_GR1_Handler
    .word EMB_GR2_Handler
    .word EMB_GR3_Handler
    .word HRPWM_3_GCmp_Handler
    .word HRPWM_3_Ovf_Udf_Handler
    .word HRPWM_3_SCmp_Handler
    .word HRPWM_3_GCap_Handler
    .word HRPWM_4_GCmp_Handler
    .word HRPWM_4_Ovf_Udf_Handler
    .word HRPWM_4_SCmp_Handler
    .word HRPWM_4_GCap_Handler
    .word EMB_GR4_Handler
    .word EMB_GR5_Handler
    .word EMB_GR6_Handler
    .word EMB_GR7_Handler
    .word EMB_GR8_Handler
    .word HRPWM_5_GCmp_Handler
    .word HRPWM_5_Ovf_Udf_Handler
    .word HRPWM_5_SCmp_Handler
    .word HRPWM_5_GCap_Handler
    .word HRPWM_6_GCmp_Handler
    .word HRPWM_6_Ovf_Udf_Handler
    .word HRPWM_6_SCmp_Handler
    .word HRPWM_6_GCap_Handler
    .word MCANRAM_ECCError_Handler
    .word MCAN1_INT0_Handler
    .word MCAN1_INT1_Handler
    .word MCAN2_INT0_Handler
    .word MCAN2_INT1_Handler
    .word TMR4_GCmp_Handler
    .word TMR4_Ovf_Udf_Handler
    .word TMR4_Reload_Handler
    .word TMR4_SCmp_Handler
    .word CMP1_Handler
    .word CMP2_Handler
    .word CMP3_Handler
    .word I2C_Handler
    .word USART1_Handler
    .word USART1_TxComplete_Handler
    .word SPI_Handler
    .word TMRA_5_Ovf_Udf_Handler
    .word TMRA_5_Cmp_Handler
    .word EVENT_PORT1_Handler
    .word EVENT_PORT2_Handler
    .word EVENT_PORT3_Handler
    .word EVENT_PORT4_Handler
    .word USART2_Handler
    .word USART2_TxComplete_Handler
    .word USART3_Handler
    .word USART3_TxComplete_Handler
    .word USART4_Handler
    .word USART4_TxComplete_Handler
    .word 0
    .word USART1_WKUP_Handler
    .word PWC_LVD1_Handler
    .word PWC_LVD2_Handler
    .word FCM_Handler
    .word WDT_Handler
    .word CTC_Handler
    .word ADC1_Handler
    .word ADC2_Handler
    .word ADC3_Handler
__Vectors_End:
.equ __Vectors_Size, __Vectors_End - __Vectors
.size __Vectors, . - __Vectors

.section .text.Reset_Handler, "ax", %progbits
.align 2
.weak Reset_Handler
.global Reset_Handler
.type Reset_Handler, %function
.thumb_func
Reset_Handler:
    ldr r0, =0x40050810
    ldr r1, =0x1FC
    str r1, [r0]

    ldr r1, =__data_load__
    ldr r2, =__data_start__
    ldr r3, =__data_end__
1:
    cmp r2, r3
    bcs 2f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 1b
2:
    ldr r1, =__bss_start__
    ldr r2, =__bss_end__
    movs r0, #0
3:
    cmp r1, r2
    bcs 4f
    str r0, [r1], #4
    b 3b
4:
    bl SystemInit
    bl __libc_init_array
    bl main
5:
    b 5b
.size Reset_Handler, . - Reset_Handler

.section .text.Default_Handler, "ax", %progbits
.align 2
.type Default_Handler, %function
.thumb_func
Default_Handler:
    b .
.size Default_Handler, . - Default_Handler

.macro Set_Default_Handler handler
.weak \handler
.thumb_set \handler, Default_Handler
.endm

Set_Default_Handler NMI_Handler
Set_Default_Handler HardFault_Handler
Set_Default_Handler MemManage_Handler
Set_Default_Handler BusFault_Handler
Set_Default_Handler UsageFault_Handler
Set_Default_Handler SVC_Handler
Set_Default_Handler DebugMon_Handler
Set_Default_Handler PendSV_Handler
Set_Default_Handler SysTick_Handler
Set_Default_Handler IRQ000_Handler
Set_Default_Handler IRQ001_Handler
Set_Default_Handler IRQ002_Handler
Set_Default_Handler IRQ003_Handler
Set_Default_Handler IRQ004_Handler
Set_Default_Handler IRQ005_Handler
Set_Default_Handler IRQ006_Handler
Set_Default_Handler IRQ007_Handler
Set_Default_Handler IRQ008_Handler
Set_Default_Handler IRQ009_Handler
Set_Default_Handler IRQ010_Handler
Set_Default_Handler IRQ011_Handler
Set_Default_Handler IRQ012_Handler
Set_Default_Handler IRQ013_Handler
Set_Default_Handler IRQ014_Handler
Set_Default_Handler IRQ015_Handler
Set_Default_Handler EXTINT00_SWINT16_Handler
Set_Default_Handler EXTINT01_SWINT17_Handler
Set_Default_Handler EXTINT02_SWINT18_Handler
Set_Default_Handler EXTINT03_SWINT19_Handler
Set_Default_Handler EXTINT04_SWINT20_Handler
Set_Default_Handler EXTINT05_SWINT21_Handler
Set_Default_Handler EXTINT06_SWINT22_Handler
Set_Default_Handler EXTINT07_SWINT23_Handler
Set_Default_Handler EXTINT08_SWINT24_Handler
Set_Default_Handler EXTINT09_SWINT25_Handler
Set_Default_Handler EXTINT10_SWINT26_Handler
Set_Default_Handler EXTINT11_SWINT27_Handler
Set_Default_Handler EXTINT12_SWINT28_Handler
Set_Default_Handler EXTINT13_SWINT29_Handler
Set_Default_Handler EXTINT14_SWINT30_Handler
Set_Default_Handler EXTINT15_SWINT31_Handler
Set_Default_Handler DMA_Error_Handler
Set_Default_Handler DMA_TC0_BTC0_Handler
Set_Default_Handler DMA_TC1_BTC1_Handler
Set_Default_Handler DMA_TC2_BTC2_Handler
Set_Default_Handler DMA_TC3_BTC3_Handler
Set_Default_Handler DMA_TC4_BTC4_Handler
Set_Default_Handler DMA_TC5_BTC5_Handler
Set_Default_Handler DMA_TC6_BTC6_Handler
Set_Default_Handler DMA_TC7_BTC7_Handler
Set_Default_Handler EFM_PEError_ReadCol_Handler
Set_Default_Handler EFM_OpEnd_Handler
Set_Default_Handler FPU_Error_Handler
Set_Default_Handler TMR0_1_Handler
Set_Default_Handler TMR0_2_Handler
Set_Default_Handler RTC_Handler
Set_Default_Handler CLK_XtalStop_Handler
Set_Default_Handler PWC_WKTM_Handler
Set_Default_Handler SWDT_Handler
Set_Default_Handler TMR6_1_GCmp_Handler
Set_Default_Handler TMR6_1_Ovf_Udf_Handler
Set_Default_Handler TMR6_1_Dte_Handler
Set_Default_Handler TMR6_1_SCmp_Handler
Set_Default_Handler TMRA_1_Ovf_Udf_Handler
Set_Default_Handler TMRA_1_Cmp_Handler
Set_Default_Handler TMR6_2_GCmp_Handler
Set_Default_Handler TMR6_2_Ovf_Udf_Handler
Set_Default_Handler TMR6_2_Dte_Handler
Set_Default_Handler TMR6_2_SCmp_Handler
Set_Default_Handler TMRA_2_Ovf_Udf_Handler
Set_Default_Handler TMRA_2_Cmp_Handler
Set_Default_Handler TMR6_3_GCmp_Handler
Set_Default_Handler TMR6_3_Ovf_Udf_Handler
Set_Default_Handler TMR6_3_Dte_Handler
Set_Default_Handler TMR6_3_SCmp_Handler
Set_Default_Handler TMRA_3_Ovf_Udf_Handler
Set_Default_Handler TMRA_3_Cmp_Handler
Set_Default_Handler TMR6_4_GCmp_Handler
Set_Default_Handler TMR6_4_Ovf_Udf_Handler
Set_Default_Handler TMR6_4_Dte_Handler
Set_Default_Handler TMR6_4_SCmp_Handler
Set_Default_Handler TMRA_4_Ovf_Udf_Handler
Set_Default_Handler TMRA_4_Cmp_Handler
Set_Default_Handler HRPWM_1_GCmp_Handler
Set_Default_Handler HRPWM_1_Ovf_Udf_Handler
Set_Default_Handler HRPWM_1_SCmp_Handler
Set_Default_Handler HRPWM_1_GCap_Handler
Set_Default_Handler HRPWM_2_GCmp_Handler
Set_Default_Handler HRPWM_2_Ovf_Udf_Handler
Set_Default_Handler HRPWM_2_SCmp_Handler
Set_Default_Handler HRPWM_2_GCap_Handler
Set_Default_Handler HRPWM_Handler
Set_Default_Handler EMB_GR0_Handler
Set_Default_Handler EMB_GR1_Handler
Set_Default_Handler EMB_GR2_Handler
Set_Default_Handler EMB_GR3_Handler
Set_Default_Handler HRPWM_3_GCmp_Handler
Set_Default_Handler HRPWM_3_Ovf_Udf_Handler
Set_Default_Handler HRPWM_3_SCmp_Handler
Set_Default_Handler HRPWM_3_GCap_Handler
Set_Default_Handler HRPWM_4_GCmp_Handler
Set_Default_Handler HRPWM_4_Ovf_Udf_Handler
Set_Default_Handler HRPWM_4_SCmp_Handler
Set_Default_Handler HRPWM_4_GCap_Handler
Set_Default_Handler EMB_GR4_Handler
Set_Default_Handler EMB_GR5_Handler
Set_Default_Handler EMB_GR6_Handler
Set_Default_Handler EMB_GR7_Handler
Set_Default_Handler EMB_GR8_Handler
Set_Default_Handler HRPWM_5_GCmp_Handler
Set_Default_Handler HRPWM_5_Ovf_Udf_Handler
Set_Default_Handler HRPWM_5_SCmp_Handler
Set_Default_Handler HRPWM_5_GCap_Handler
Set_Default_Handler HRPWM_6_GCmp_Handler
Set_Default_Handler HRPWM_6_Ovf_Udf_Handler
Set_Default_Handler HRPWM_6_SCmp_Handler
Set_Default_Handler HRPWM_6_GCap_Handler
Set_Default_Handler MCANRAM_ECCError_Handler
Set_Default_Handler MCAN1_INT0_Handler
Set_Default_Handler MCAN1_INT1_Handler
Set_Default_Handler MCAN2_INT0_Handler
Set_Default_Handler MCAN2_INT1_Handler
Set_Default_Handler TMR4_GCmp_Handler
Set_Default_Handler TMR4_Ovf_Udf_Handler
Set_Default_Handler TMR4_Reload_Handler
Set_Default_Handler TMR4_SCmp_Handler
Set_Default_Handler CMP1_Handler
Set_Default_Handler CMP2_Handler
Set_Default_Handler CMP3_Handler
Set_Default_Handler I2C_Handler
Set_Default_Handler USART1_Handler
Set_Default_Handler USART1_TxComplete_Handler
Set_Default_Handler SPI_Handler
Set_Default_Handler TMRA_5_Ovf_Udf_Handler
Set_Default_Handler TMRA_5_Cmp_Handler
Set_Default_Handler EVENT_PORT1_Handler
Set_Default_Handler EVENT_PORT2_Handler
Set_Default_Handler EVENT_PORT3_Handler
Set_Default_Handler EVENT_PORT4_Handler
Set_Default_Handler USART2_Handler
Set_Default_Handler USART2_TxComplete_Handler
Set_Default_Handler USART3_Handler
Set_Default_Handler USART3_TxComplete_Handler
Set_Default_Handler USART4_Handler
Set_Default_Handler USART4_TxComplete_Handler
Set_Default_Handler USART1_WKUP_Handler
Set_Default_Handler PWC_LVD1_Handler
Set_Default_Handler PWC_LVD2_Handler
Set_Default_Handler FCM_Handler
Set_Default_Handler WDT_Handler
Set_Default_Handler CTC_Handler
Set_Default_Handler ADC1_Handler
Set_Default_Handler ADC2_Handler
Set_Default_Handler ADC3_Handler

.section .note.GNU-stack, "", %progbits
