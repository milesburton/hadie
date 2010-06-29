/* Interface to the C328 UART camera */

#include "config.h"
#include <stdint.h>
#include <avr/io.h> 
#include "c328.h"

/* >10ms timeout at 300 hz */
#define CMD_TIMEOUT (4)

/* Wait longer for the camera to take the image and return DATA response */
#define PIC_TIMEOUT (200)

#define RXREADY (UCSR0A & (1 << RXC0))

/* Receive buffer */
#define RXBUF_LEN (64)
static uint8_t rxbuf[RXBUF_LEN];
static uint8_t rxbuf_len = 0;

/* Expected package size */
static uint8_t pkg_len = 64; /* Default is 64 according to datasheet */

/* Timeout counter */
volatile static uint8_t timeout_clk = 0;

void inline c3_tick()
{
	if(timeout_clk) timeout_clk--;
}

static void tx_byte(uint8_t b)
{
	/* Wait for empty transmit buffer */
	while(!(UCSR0A & (1 << UDRE0)));
	
	/* Put data into buffer, sends the data */
	UDR0 = b;
}

static uint8_t c3_rx(uint8_t timeout)
{
	rxbuf_len = 0;
	
	timeout_clk = timeout;
	while(timeout_clk)
	{
		if(!RXREADY) continue;
		rxbuf[rxbuf_len++] = UDR0;
		if(rxbuf_len == 6) break;
	}
	
	if(rxbuf_len != 6) return(0); /* Timeout or incomplete response */
	if(rxbuf[0] != 0xAA) return(0); /* All responses should begin 0xAA */
	
	/* Return the received command ID */
	return(rxbuf[1]);
}

static void c3_tx(uint8_t cmd, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4)
{
	tx_byte(0xAA);
	tx_byte(cmd);
	tx_byte(a1);
	tx_byte(a2);
	tx_byte(a3);
	tx_byte(a4);
}

static char c3_cmd(uint8_t cmd, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4)
{
	uint8_t r;
	
	c3_tx(cmd, a1, a2, a3, a4);
	r = c3_rx(CMD_TIMEOUT);
	
	/* Did we get an ACK for this command? */
	if(r != CMD_ACK || rxbuf[2] != cmd) return(-1);
	
	return(0);
}

void c3_init()
{
	/* Do UART initialisation, port 0 @ 57600 baud for 7.3728 MHz clock */
	UBRR0H = 0;
	UBRR0L = 7;
	
	/* Enable TX & RX */
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	
	/* 8-bit, no parity and 1 stop bit */
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

char c3_sync()
{
	char i;
	
	/* Send the SYNC command until the camera responds, up to 60 */
	for(i = 0; i < 60; i++)
	{
		/* Send the sync and wait for an ACK */
		if(c3_cmd(CMD_SYNC, 0, 0, 0, 0) != 0) continue;
		
		/* ACK should be followed by a SYNC */
		if(c3_rx(CMD_TIMEOUT) != CMD_SYNC) continue;
		
		/* ACK the SYNC and return success code */
		c3_tx(CMD_ACK, CMD_SYNC, 0, 0, 0);
		return(0);
	}
	
	/* If we got here, the camera failed to sync. Panic */
	return(-1);
}

char c3_setup(uint8_t ct, uint8_t rr, uint8_t jr)
{
	return(c3_cmd(CMD_INIT, 0, ct, rr, jr));
}

char c3_set_package_size(uint16_t s)
{
	char r;
	
	if(s > RXBUF_LEN) return(-1);
	
	r = c3_cmd(CMD_SET_PKG_SIZE, 0x08, s & 0xFF, s >> 8, 0);
	
	if(r == 0) pkg_len = s;
	
	return(r);
}

char c3_snapshot(uint8_t st, uint16_t skip_frame)
{
	return(c3_cmd(CMD_SNAPSHOT, st, skip_frame & 0xFF, skip_frame >> 8, 0));
}

char c3_get_picture(uint8_t pt, uint16_t *length)
{
	/* Send the command */
	if(c3_cmd(CMD_GET_PICTURE, pt, 0, 0, 0) != 0) return(-1);
	
	/* The camera should now send a DATA message */
	if(c3_rx(PIC_TIMEOUT) != CMD_DATA) return(-1);
	
	/* Get the file size from the DATA args */
	*length = rxbuf[3] + (rxbuf[4] << 8);
	
	return(0);
}

char c3_get_package(uint16_t id, uint8_t **dst, uint16_t *length)
{
	uint8_t checksum;
	uint16_t s;
	
	rxbuf_len = 0;
	checksum = 0;
	s = pkg_len;
	
	/* Get the package by sending an ACK */
	c3_tx(CMD_ACK, 0, 0, id & 0xFF, id >> 8);
	
	/* The camera should immediatly start returning data */
	timeout_clk = CMD_TIMEOUT;
	while(timeout_clk && rxbuf_len < s)
	{
		if(!RXREADY) continue;
		
		/* Read the byte and update checksum */
		checksum += rxbuf[rxbuf_len++] = UDR0;
		
		if(rxbuf_len == 4)
		{
			/* Get the actual length of the package */
			s = rxbuf[2] + (rxbuf[3] << 8) + 6;
			if(s > pkg_len) return(-1);
		}
	}
	
	/* Test for timeout or incomplete package */
	if(rxbuf_len < s) return(-1);
	
	/* Test checksum */
	checksum -= rxbuf[rxbuf_len - 2];
	if(checksum != rxbuf[rxbuf_len - 2]) return(-1);
	
	/* All done */
	*dst = rxbuf;
	*length = rxbuf_len;
	
	return(0);
}

char c3_finish_picture()
{
	c3_tx(CMD_ACK, 0, 0, 0xF0, 0xF0);
	return(0);
}

