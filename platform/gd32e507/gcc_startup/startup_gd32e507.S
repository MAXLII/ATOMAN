/* SPDX-License-Identifier: MIT */
/* GNU startup and GD32E50X_CL vector table for the GD32E507 platform. */

.syntax unified
.cpu cortex-m33
.fpu fpv5-sp-d16
.thumb

.global Reset_Handler
.global Default_Handler
.global g_pfn_vectors

.extern SystemInit
.extern __libc_init_array
.extern main
.extern _estack
.extern _sidata
.extern _sdata
.extern _edata
.extern _sbss
.extern _ebss
.extern _sifunc_ram
.extern _sfunc_ram
.extern _efunc_ram

.section .text.Reset_Handler,"ax",%progbits
.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
1:
    cmp r1, r2
    bcc 2f
    b 3f
2:
    ldr r3, [r0], #4
    str r3, [r1], #4
    b 1b
3:
    ldr r0, =_sifunc_ram
    ldr r1, =_sfunc_ram
    ldr r2, =_efunc_ram
8:
    cmp r1, r2
    bcc 9f
    b 10f
9:
    ldr r3, [r0], #4
    str r3, [r1], #4
    b 8b
10:
    ldr r1, =_sbss
    ldr r2, =_ebss
    movs r3, #0
4:
    cmp r1, r2
    bcc 5f
    b 6f
5:
    str r3, [r1], #4
    b 4b
6:
    bl SystemInit
    bl __libc_init_array
    bl main
7:
    b 7b
.size Reset_Handler, .-Reset_Handler

.section .text.Default_Handler,"ax",%progbits
.type Default_Handler, %function
Default_Handler:
    b Default_Handler
.size Default_Handler, .-Default_Handler

.section .isr_vector,"a",%progbits
.align 9
.type g_pfn_vectors, %object
g_pfn_vectors:
    .word _estack
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
    .word WWDGT_IRQHandler
    .word LVD_IRQHandler
    .word TAMPER_IRQHandler
    .word RTC_IRQHandler
    .word FMC_IRQHandler
    .word RCU_CTC_IRQHandler
    .word EXTI0_IRQHandler
    .word EXTI1_IRQHandler
    .word EXTI2_IRQHandler
    .word EXTI3_IRQHandler
    .word EXTI4_IRQHandler
    .word DMA0_Channel0_IRQHandler
    .word DMA0_Channel1_IRQHandler
    .word DMA0_Channel2_IRQHandler
    .word DMA0_Channel3_IRQHandler
    .word DMA0_Channel4_IRQHandler
    .word DMA0_Channel5_IRQHandler
    .word DMA0_Channel6_IRQHandler
    .word ADC0_1_IRQHandler
    .word CAN0_TX_IRQHandler
    .word CAN0_RX0_IRQHandler
    .word CAN0_RX1_IRQHandler
    .word CAN0_EWMC_IRQHandler
    .word EXTI5_9_IRQHandler
    .word TIMER0_BRK_TIMER8_IRQHandler
    .word TIMER0_UP_TIMER9_IRQHandler
    .word TIMER0_TRG_CMT_TIMER10_IRQHandler
    .word TIMER0_Channel_IRQHandler
    .word TIMER1_IRQHandler
    .word TIMER2_IRQHandler
    .word TIMER3_IRQHandler
    .word I2C0_EV_IRQHandler
    .word I2C0_ER_IRQHandler
    .word I2C1_EV_IRQHandler
    .word I2C1_ER_IRQHandler
    .word SPI0_IRQHandler
    .word SPI1_I2S1ADD_IRQHandler
    .word USART0_IRQHandler
    .word USART1_IRQHandler
    .word USART2_IRQHandler
    .word EXTI10_15_IRQHandler
    .word RTC_Alarm_IRQHandler
    .word USBHS_WKUP_IRQHandler
    .word TIMER7_BRK_TIMER11_IRQHandler
    .word TIMER7_UP_TIMER12_IRQHandler
    .word TIMER7_TRG_CMT_TIMER13_IRQHandler
    .word TIMER7_Channel_IRQHandler
    .word 0
    .word EXMC_IRQHandler
    .word 0
    .word TIMER4_IRQHandler
    .word SPI2_I2S2ADD_IRQHandler
    .word UART3_IRQHandler
    .word UART4_IRQHandler
    .word TIMER5_DAC_IRQHandler
    .word TIMER6_IRQHandler
    .word DMA1_Channel0_IRQHandler
    .word DMA1_Channel1_IRQHandler
    .word DMA1_Channel2_IRQHandler
    .word DMA1_Channel3_IRQHandler
    .word DMA1_Channel4_IRQHandler
    .word ENET_IRQHandler
    .word ENET_WKUP_IRQHandler
    .word CAN1_TX_IRQHandler
    .word CAN1_RX0_IRQHandler
    .word CAN1_RX1_IRQHandler
    .word CAN1_EWMC_IRQHandler
    .word USBHS_IRQHandler
    .word 0
    .word SHRTIMER_IRQ2_IRQHandler
    .word SHRTIMER_IRQ3_IRQHandler
    .word SHRTIMER_IRQ4_IRQHandler
    .word SHRTIMER_IRQ5_IRQHandler
    .word SHRTIMER_IRQ6_IRQHandler
    .word USBHS_EP1_OUT_IRQHandler
    .word USBHS_EP1_IN_IRQHandler
    .word SHRTIMER_IRQ0_IRQHandler
    .word SHRTIMER_IRQ1_IRQHandler
    .word CAN2_TX_IRQHandler
    .word CAN2_RX0_IRQHandler
    .word CAN2_RX1_IRQHandler
    .word CAN2_EWMC_IRQHandler
    .word I2C2_EV_IRQHandler
    .word I2C2_ER_IRQHandler
    .word USART5_IRQHandler
    .word I2C2_WKUP_IRQHandler
    .word USART5_WKUP_IRQHandler
    .word TMU_IRQHandler
.size g_pfn_vectors, .-g_pfn_vectors

.macro weak_default handler
    .weak \handler
    .thumb_set \handler, Default_Handler
.endm

weak_default NMI_Handler
weak_default HardFault_Handler
weak_default MemManage_Handler
weak_default BusFault_Handler
weak_default UsageFault_Handler
weak_default SVC_Handler
weak_default DebugMon_Handler
weak_default PendSV_Handler
weak_default SysTick_Handler
weak_default WWDGT_IRQHandler
weak_default LVD_IRQHandler
weak_default TAMPER_IRQHandler
weak_default RTC_IRQHandler
weak_default FMC_IRQHandler
weak_default RCU_CTC_IRQHandler
weak_default EXTI0_IRQHandler
weak_default EXTI1_IRQHandler
weak_default EXTI2_IRQHandler
weak_default EXTI3_IRQHandler
weak_default EXTI4_IRQHandler
weak_default DMA0_Channel0_IRQHandler
weak_default DMA0_Channel1_IRQHandler
weak_default DMA0_Channel2_IRQHandler
weak_default DMA0_Channel3_IRQHandler
weak_default DMA0_Channel4_IRQHandler
weak_default DMA0_Channel5_IRQHandler
weak_default DMA0_Channel6_IRQHandler
weak_default ADC0_1_IRQHandler
weak_default CAN0_TX_IRQHandler
weak_default CAN0_RX0_IRQHandler
weak_default CAN0_RX1_IRQHandler
weak_default CAN0_EWMC_IRQHandler
weak_default EXTI5_9_IRQHandler
weak_default TIMER0_BRK_TIMER8_IRQHandler
weak_default TIMER0_UP_TIMER9_IRQHandler
weak_default TIMER0_TRG_CMT_TIMER10_IRQHandler
weak_default TIMER0_Channel_IRQHandler
weak_default TIMER1_IRQHandler
weak_default TIMER2_IRQHandler
weak_default TIMER3_IRQHandler
weak_default I2C0_EV_IRQHandler
weak_default I2C0_ER_IRQHandler
weak_default I2C1_EV_IRQHandler
weak_default I2C1_ER_IRQHandler
weak_default SPI0_IRQHandler
weak_default SPI1_I2S1ADD_IRQHandler
weak_default USART0_IRQHandler
weak_default USART1_IRQHandler
weak_default USART2_IRQHandler
weak_default EXTI10_15_IRQHandler
weak_default RTC_Alarm_IRQHandler
weak_default USBHS_WKUP_IRQHandler
weak_default TIMER7_BRK_TIMER11_IRQHandler
weak_default TIMER7_UP_TIMER12_IRQHandler
weak_default TIMER7_TRG_CMT_TIMER13_IRQHandler
weak_default TIMER7_Channel_IRQHandler
weak_default EXMC_IRQHandler
weak_default TIMER4_IRQHandler
weak_default SPI2_I2S2ADD_IRQHandler
weak_default UART3_IRQHandler
weak_default UART4_IRQHandler
weak_default TIMER5_DAC_IRQHandler
weak_default TIMER6_IRQHandler
weak_default DMA1_Channel0_IRQHandler
weak_default DMA1_Channel1_IRQHandler
weak_default DMA1_Channel2_IRQHandler
weak_default DMA1_Channel3_IRQHandler
weak_default DMA1_Channel4_IRQHandler
weak_default ENET_IRQHandler
weak_default ENET_WKUP_IRQHandler
weak_default CAN1_TX_IRQHandler
weak_default CAN1_RX0_IRQHandler
weak_default CAN1_RX1_IRQHandler
weak_default CAN1_EWMC_IRQHandler
weak_default USBHS_IRQHandler
weak_default SHRTIMER_IRQ2_IRQHandler
weak_default SHRTIMER_IRQ3_IRQHandler
weak_default SHRTIMER_IRQ4_IRQHandler
weak_default SHRTIMER_IRQ5_IRQHandler
weak_default SHRTIMER_IRQ6_IRQHandler
weak_default USBHS_EP1_OUT_IRQHandler
weak_default USBHS_EP1_IN_IRQHandler
weak_default SHRTIMER_IRQ0_IRQHandler
weak_default SHRTIMER_IRQ1_IRQHandler
weak_default CAN2_TX_IRQHandler
weak_default CAN2_RX0_IRQHandler
weak_default CAN2_RX1_IRQHandler
weak_default CAN2_EWMC_IRQHandler
weak_default I2C2_EV_IRQHandler
weak_default I2C2_ER_IRQHandler
weak_default USART5_IRQHandler
weak_default I2C2_WKUP_IRQHandler
weak_default USART5_WKUP_IRQHandler
weak_default TMU_IRQHandler

.section .note.GNU-stack,"",%progbits
