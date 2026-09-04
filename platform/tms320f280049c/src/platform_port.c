// SPDX-License-Identifier: MIT
/**
 * @file    platform_port.c
 * @brief   TMS320F280049C LaunchPad platform services.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure user LED LD4 and SCIA through C2000Ware DriverLib
 *          - Generate the SECTION 100 us tick with CPU Timer0
 *          - Provide a free-running ERAD Counter1 CPU-cycle time base for Perf
 *          - Buffer interrupt-driven RX and TX through the XDS110 virtual COM port
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - SCIA ISRs only move logical octets through bounded platform queues
 *          - Hardware access is isolated in this platform file
 *
 *          Design beliefs:
 *          - Entity: this platform owns the hardware tick, board pins, and SCI queues
 *          - Prior: received octets are untrusted until the FRAME parser validates a complete packet
 *          - Time: the tick is 100 us and every transmit wait has a bounded deadline
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "platform.h"
#include "section.h"

#define TMS320F280049C_SYSCLK_HZ DEVICE_SYSCLK_FREQ /* Official LaunchPad PLL configuration: 100 MHz. */
#define TMS320F280049C_LSPCLK_HZ DEVICE_LSPCLK_FREQ /* SYSCLK/4; 115200 request yields about 115741 baud. */
#define TMS320F280049C_TICK_HZ 10000u      /* SECTION scheduler tick frequency. */
#define TMS320F280049C_UART_BAUD 115200u    /* Requested XDS110 virtual-COM baud rate. */
#define TMS320F280049C_UART_WIRE_OCTET_MASK 0x00FFu /* Significant bits in one physical SCI character. */
#define TMS320F280049C_UART_RX_QUEUE_CAPACITY 128u /* Stored logical octets, including one sentinel slot. */
#define TMS320F280049C_UART_TX_QUEUE_CAPACITY 1024u /* Pending logical octets, including one sentinel slot. */
#define TMS320F280049C_SCIA_RX_FIFO_CAPACITY 16u  /* Maximum hardware FIFO words drained by one ISR. */
#define TMS320F280049C_SCIA_TX_FIFO_CAPACITY 16u  /* Maximum hardware FIFO words filled by one ISR. */
#define TMS320F280049C_UART_TX_WAIT_TICKS 10000u /* Maximum 1 s wait for room for one complete frame. */
#define TMS320F280049C_UART_TX_WAIT_ITERATIONS 1000000u /* Fallback bound when the system tick cannot advance. */

static volatile uint32_t g_section_tick_100us = 0u; /* SECTION time base incremented every 100 us. */
static volatile uint8_t g_uart_rx_queue[TMS320F280049C_UART_RX_QUEUE_CAPACITY]; /* ISR-to-task RX queue. */
static volatile uint16_t g_uart_rx_write_index = 0u; /* Next queue slot written only by the SCIA ISR. */
static volatile uint16_t g_uart_rx_read_index = 0u;  /* Next queue slot consumed only by foreground code. */
static volatile uint8_t g_uart_tx_queue[TMS320F280049C_UART_TX_QUEUE_CAPACITY]; /* Task-to-ISR TX queue. */
static volatile uint16_t g_uart_tx_write_index = 0u; /* Published end of complete frames from foreground. */
static volatile uint16_t g_uart_tx_read_index = 0u;  /* Next queue slot consumed only by the SCIA ISR. */
volatile uint32_t g_tms320f280049c_uart_rx_octet_count = 0u; /* Accepted physical RX octets. */
volatile uint32_t g_tms320f280049c_uart_rx_error_count = 0u; /* Receiver recovery events. */
volatile uint32_t g_tms320f280049c_uart_rx_drop_count = 0u; /* Logical octets discarded when the queue is full. */
volatile uint32_t g_tms320f280049c_uart_tx_octet_count = 0u; /* Completed physical TX octets. */
volatile uint32_t g_tms320f280049c_uart_tx_drop_frame_count = 0u; /* Complete frames rejected after bounded wait. */

/**
 * @brief Advance one RX queue index with explicit wraparound.
 * @param index Current queue index.
 * @return Next queue index in the bounded storage range.
 */
static uint16_t tms320f280049c_uart_rx_index_next(uint16_t index)
{
    index++;
    if (index >= TMS320F280049C_UART_RX_QUEUE_CAPACITY)
    {
        index = 0u;
    }
    return index;
}

/**
 * @brief Advance one TX queue index with explicit wraparound.
 * @param index Current queue index.
 * @return Next queue index in the bounded storage range.
 */
#pragma CODE_SECTION(tms320f280049c_uart_tx_index_next, ".TI.ramfunc")
static uint16_t tms320f280049c_uart_tx_index_next(uint16_t index)
{
    index++;
    if (index >= TMS320F280049C_UART_TX_QUEUE_CAPACITY)
    {
        index = 0u;
    }
    return index;
}

/**
 * @brief Read the currently available TX queue capacity.
 * @return Number of logical octets that can be committed without overwriting pending data.
 */
#pragma CODE_SECTION(tms320f280049c_uart_tx_free_get, ".TI.ramfunc")
static uint16_t tms320f280049c_uart_tx_free_get(void)
{
    uint16_t read_index = g_uart_tx_read_index;   /* Consumer position sampled from the TX ISR. */
    uint16_t write_index = g_uart_tx_write_index; /* Producer position owned by foreground code. */
    uint16_t free_count = 0u;                     /* Queue slots available without using the sentinel. */

    if (write_index >= read_index)
    {
        free_count = (uint16_t)((TMS320F280049C_UART_TX_QUEUE_CAPACITY - 1u) -
                                (write_index - read_index));
    }
    else
    {
        free_count = (uint16_t)(read_index - write_index - 1u);
    }
    return free_count;
}

/**
 * @brief Add one validated logical octet from the SCIA ISR.
 * @param data Low-eight-bit wire value read from the hardware FIFO.
 * @return 1 when queued, otherwise 0 when bounded storage is full.
 */
#pragma CODE_SECTION(tms320f280049c_uart_rx_push, ".TI.ramfunc")
static uint8_t tms320f280049c_uart_rx_push(uint8_t data)
{
    uint16_t write_index = g_uart_rx_write_index; /* Queue position exclusively owned by the ISR producer. */
    uint16_t next_index = tms320f280049c_uart_rx_index_next(write_index); /* Candidate committed position. */

    if (next_index == g_uart_rx_read_index)
    {
        g_tms320f280049c_uart_rx_drop_count++;
        return 0u;
    }

    g_uart_rx_queue[write_index] = (uint8_t)((uint16_t)data & TMS320F280049C_UART_WIRE_OCTET_MASK);
    g_uart_rx_write_index = next_index;
    return 1u;
}

static __interrupt void tms320f280049c_cpu_timer0_isr(void)
{
    g_section_tick_100us++;
    section_interrupt();
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

#pragma CODE_SECTION(tms320f280049c_scia_rx_isr, ".TI.ramfunc")
static __interrupt void tms320f280049c_scia_rx_isr(void)
{
    uint16_t processed_count = 0u; /* Hardware FIFO entries handled during this bounded ISR invocation. */
    uint16_t receive_status = SCI_getRxStatus(SCIA_BASE); /* Receiver status captured before FIFO access. */

    if ((receive_status & SCI_RXSTATUS_ERROR) != 0u)
    {
        g_tms320f280049c_uart_rx_error_count++;
        SCI_performSoftwareReset(SCIA_BASE);
        SCI_resetRxFIFO(SCIA_BASE);
        SCI_clearOverflowStatus(SCIA_BASE);
        SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXFF);
        Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
        return;
    }

    if (SCI_getOverflowStatus(SCIA_BASE) == true)
    {
        g_tms320f280049c_uart_rx_error_count++;
    }

    while ((SCI_getRxFIFOStatus(SCIA_BASE) != SCI_FIFO_RX0) &&
           (processed_count < TMS320F280049C_SCIA_RX_FIFO_CAPACITY))
    {
        uint8_t data = (uint8_t)(SCI_readCharNonBlocking(SCIA_BASE) &
                                 TMS320F280049C_UART_WIRE_OCTET_MASK); /* One physical octet. */

        if (tms320f280049c_uart_rx_push(data) != 0u)
        {
            g_tms320f280049c_uart_rx_octet_count++;
        }
        processed_count++;
    }

    SCI_clearOverflowStatus(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

#pragma CODE_SECTION(tms320f280049c_scia_tx_isr, ".TI.ramfunc")
static __interrupt void tms320f280049c_scia_tx_isr(void)
{
    uint16_t read_index = g_uart_tx_read_index; /* Queue position exclusively owned by this ISR consumer. */
    uint16_t processed_count = 0u; /* Hardware FIFO entries filled during this bounded ISR invocation. */

    while ((SCI_getTxFIFOStatus(SCIA_BASE) != SCI_FIFO_TX16) &&
           (read_index != g_uart_tx_write_index) &&
           (processed_count < TMS320F280049C_SCIA_TX_FIFO_CAPACITY))
    {
        SCI_writeCharNonBlocking(SCIA_BASE,
                                 (uint16_t)g_uart_tx_queue[read_index] &
                                     TMS320F280049C_UART_WIRE_OCTET_MASK);
        read_index = tms320f280049c_uart_tx_index_next(read_index);
        g_tms320f280049c_uart_tx_octet_count++;
        processed_count++;
    }

    g_uart_tx_read_index = read_index;
    if (read_index == g_uart_tx_write_index)
    {
        SCI_disableInterrupt(SCIA_BASE, SCI_INT_TXFF);
    }
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

static void tms320f280049c_led_init(void)
{
    GPIO_setPadConfig(DEVICE_GPIO_PIN_LED1, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);
    GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1u);
}

static void tms320f280049c_uart_init(void)
{
    Interrupt_register(INT_SCIA_RX, &tms320f280049c_scia_rx_isr);
    Interrupt_register(INT_SCIA_TX, &tms320f280049c_scia_tx_isr);

    GPIO_setAnalogMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_ANALOG_DISABLED);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    SCI_performSoftwareReset(SCIA_BASE);
    SCI_setConfig(SCIA_BASE,
                  TMS320F280049C_LSPCLK_HZ,
                  TMS320F280049C_UART_BAUD,
                  SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE | SCI_CONFIG_PAR_NONE);
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_enableFIFO(SCIA_BASE);
    SCI_setFIFOInterruptLevel(SCIA_BASE, SCI_FIFO_TX0, SCI_FIFO_RX1);
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_RXFF);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
    Interrupt_enable(INT_SCIA_RX);
    Interrupt_enable(INT_SCIA_TX);
}

static void tms320f280049c_tick_init(void)
{
    const uint32_t timer_period = (TMS320F280049C_SYSCLK_HZ / TMS320F280049C_TICK_HZ) - 1u;

    Interrupt_register(INT_TIMER0, &tms320f280049c_cpu_timer0_isr);
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_setPeriod(CPUTIMER0_BASE, timer_period);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0u);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_setEmulationMode(CPUTIMER0_BASE,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_enableInterrupt(CPUTIMER0_BASE);
    Interrupt_enable(INT_TIMER0);
    CPUTimer_startTimer(CPUTIMER0_BASE);
}

/**
 * @brief Configure ERAD Counter1 as the free-running Perf CPU-cycle counter.
 * @details ERAD ownership is global. Selecting application ownership reserves
 *          Counter1 for firmware Perf measurements instead of debugger ERAD use.
 */
#pragma CODE_SECTION(tms320f280049c_perf_counter_init, ".TI.ramfunc")
static void tms320f280049c_perf_counter_init(void)
{
    ERAD_Counter_Config counter_config = {
        .event = ERAD_EVENT_NO_EVENT,
        .event_mode = ERAD_COUNTER_MODE_ACTIVE,
        .reference = 0u,
        .rst_on_match = false,
        .enable_int = false,
        .enable_stop = false,
    };

    /* ERAD ownership is global; Counter1 is intentionally firmware-owned. */
    ERAD_setOwnership(ERAD_OWNER_APPLICATION);
    ERAD_disableModules((uint16_t)ERAD_INST_COUNTER1);
    ERAD_resetCounter((uint16_t)ERAD_INST_COUNTER1);
    ERAD_configCounterInCountingMode(ERAD_COUNTER1_BASE, counter_config);
    ERAD_setCurrentCount(ERAD_COUNTER1_BASE, 0u);
    ERAD_enableModules((uint16_t)ERAD_INST_COUNTER1);
}

void tms320f280049c_device_init(void)
{
    Device_init();
    Device_initGPIO();
}

void tms320f280049c_platform_init(void)
{
    tms320f280049c_led_init();
    tms320f280049c_uart_init();
    tms320f280049c_tick_init();
    tms320f280049c_perf_counter_init();
}

uint32_t tms320f280049c_section_tick_get(void)
{
    return g_section_tick_100us;
}

volatile uint32_t *tms320f280049c_section_tick_address_get(void)
{
    return &g_section_tick_100us;
}

void tms320f280049c_led_toggle(void)
{
    GPIO_togglePin(DEVICE_GPIO_PIN_LED1);
}

uint8_t tms320f280049c_uart_rx_get_byte(uint8_t *p_data)
{
    uint16_t read_index = 0u; /* Queue position exclusively owned by the foreground consumer. */

    if (p_data == NULL)
    {
        return 0u;
    }

    read_index = g_uart_rx_read_index;
    if (read_index == g_uart_rx_write_index)
    {
        return 0u;
    }

    *p_data = (uint8_t)((uint16_t)g_uart_rx_queue[read_index] & TMS320F280049C_UART_WIRE_OCTET_MASK);
    g_uart_rx_read_index = tms320f280049c_uart_rx_index_next(read_index);
    return 1u;
}

#pragma CODE_SECTION(tms320f280049c_uart_tx_write, ".TI.ramfunc")
void tms320f280049c_uart_tx_write(const uint8_t *p_data, uint16_t length)
{
    uint16_t index = 0u; /* Logical wire-octet index copied into platform-owned storage. */
    uint16_t write_index = 0u; /* Private producer cursor published only after the complete frame is copied. */
    uint32_t start_tick = g_section_tick_100us; /* Tick used to bound waiting for queue capacity. */
    uint32_t wait_iterations = 0u; /* Fallback wait bound for disabled or stalled interrupts. */

    if ((p_data == NULL) || (length == 0u))
    {
        return;
    }

    if (length >= TMS320F280049C_UART_TX_QUEUE_CAPACITY)
    {
        g_tms320f280049c_uart_tx_drop_frame_count++;
        return;
    }

    while (tms320f280049c_uart_tx_free_get() < length)
    {
        SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXFF);
        wait_iterations++;
        if (((uint32_t)(g_section_tick_100us - start_tick) >= TMS320F280049C_UART_TX_WAIT_TICKS) ||
            (wait_iterations >= TMS320F280049C_UART_TX_WAIT_ITERATIONS))
        {
            g_tms320f280049c_uart_tx_drop_frame_count++;
            return;
        }
    }

    write_index = g_uart_tx_write_index;
    for (index = 0u; index < length; index++)
    {
        g_uart_tx_queue[write_index] =
            (uint8_t)((uint16_t)p_data[index] & TMS320F280049C_UART_WIRE_OCTET_MASK);
        write_index = tms320f280049c_uart_tx_index_next(write_index);
    }
    g_uart_tx_write_index = write_index;
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXFF);
}
