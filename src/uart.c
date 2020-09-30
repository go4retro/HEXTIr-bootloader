/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2007,2008  Ingo Korb <ingo@akana.de>

   Inspiration and low-level SD/MMC access based on code from MMC2IEC
     by Lars Pontoppidan et al., see sdcard.c|h and config.h.

   FAT filesystem access based on code from ChaN and Jim Brain, see ff.c|h.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License only.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   uart.c: UART access routines

*/

#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "config.h"

#include "uart.h"

void uart_putc(uint8_t data) {
  loop_until_bit_is_set(UCSRAA, UDREA);
  UDRA = data;
}

void uart_puthex(uint8_t num) {
  uint8_t tmp;
  tmp = (num & 0xf0) >> 4;
  if (tmp < 10)
    uart_putc('0' + tmp);
  else
    uart_putc('a' + tmp - 10);

  tmp = num & 0x0f;
  if (tmp < 10)
    uart_putc('0' + tmp);
  else
    uart_putc('a'+ tmp - 10);
}

void uart_trace(uint8_t* ptr, uint16_t start, uint16_t len) {
  uint16_t i;
  uint8_t j;
  uint8_t ch;

  ptr+=start;
  for(i=0;i<len;i+=16) {

    uart_puthex(start>>8);
    uart_puthex(start&0xff);
    uart_putc('|');
    uart_putc(' ');
    for(j=0;j<16;j++) {
      if(i+j<len) {
        ch=*(ptr + j);
        uart_puthex(ch);
      } else {
        uart_putc(' ');
        uart_putc(' ');
      }
      uart_putc(' ');
    }
    uart_putc('|');
    for(j=0;j<16;j++) {
      if(i+j<len) {
        ch=*(ptr++);
        if(ch<32 || ch>0x7e)
          ch='.';
        uart_putc(ch);
      } else {
        uart_putc(' ');
      }
    }
    uart_putc('|');
    uart_putcrlf();
    start+=16;
  }
}

#ifdef UART_USE_PRINTF
static int ioputc(char c, FILE *stream) {
  if (c == '\n') uart_putc('\r');
  uart_putc(c);
  return 0;
}
#endif

uint8_t uart_getc(void) {
  loop_until_bit_is_set(UCSRAA,RXCA);
  return UDRA;
}

void uart_flush(void) {
}

void uart_puts_P(const char *text) {
  uint8_t ch;

  while ((ch = pgm_read_byte(text++))) {
    uart_putc(ch);
  }
}

void uart_putcrlf(void) {
  uart_putc(13);
  uart_putc(10);
}

#ifdef UART_USE_PRINTF
static FILE mystdout = FDEV_SETUP_STREAM(ioputc, NULL, _FDEV_SETUP_WRITE);
#endif

void uart_init(void) {
  /* Seriellen Port konfigurieren */
#if defined UART0_ENABLE
  UART0_MODE_SETUP();

  UBRRAH = CALC_BPS(UART0_BAUDRATE) >> 8;
  UBRRAL = CALC_BPS(UART0_BAUDRATE) & 0xff;

  #ifdef UART_DOUBLE_SPEED
  /* double the speed of the serial port. */
  UCSRAA = (1<<U2X0);
  #endif


  /* Enable UART receiver and transmitter */
  UCSRAB = (0
  #if defined UART0_RX_BUFFER_SHIFT && UART0_RX_BUFFER_SHIFT > 0
            | _BV(RXCIEA)
  #endif
            | _BV(RXENA)
            | _BV(TXENA)
           );

  #ifdef UART_USE_PRINTF
  stdout = &mystdout;
  #endif
  #endif
}
