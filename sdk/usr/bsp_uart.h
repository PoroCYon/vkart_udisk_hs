#ifndef _BSP_UART_H_
#define _BSP_UART_H_

#include "ch32v30x.h"

/* Global define */

#define size(a)   (sizeof(a) / sizeof(*(a)))
#define MIN(X,Y)  ((X) < (Y) ? (X) : (Y))


typedef enum { IDEL = 0, BUSY = !IDEL} Uart_TX_DMA_State;

#define UART_RX_DMA_SIZE    2048  /* Must be a power of 2 ( a%2^n = a&(2^n - ) )*/
#define ETH_RECEIVE_SIZE    800
#define UART_TX_BUF_NUM     5



struct uart_data
{
	uint8_t (*TX_buffer)[ETH_RECEIVE_SIZE];   /* receive from eth */
	uint8_t *RX_buffer;                       /* receive from uart */
	uint16_t *TX_data_length;
	uint16_t RX_data_length;
	uint16_t last_RX_DMA_length;
	uint32_t rx_read;
	uint32_t rx_write;
	uint8_t  tx_read;
	uint8_t  tx_write;
	Uart_TX_DMA_State  uart_tx_dma_state;    /* 0 -> idle, 1 -> busy */
};

extern struct uart_data  uart_data_t[8];


extern void BSP_Uart_Init(void);


#endif /* end of bsp_uart.h */
