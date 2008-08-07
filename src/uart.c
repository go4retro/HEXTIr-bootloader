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
#include "avrcompat.h"
#include "uart.h"

void uart_putc(char c) {
  loop_until_bit_is_set(UCSRA,UDRE);
  UDR = c;
}

void uart_puthex(uint8_t num) {
  uint8_t tmp;
  tmp = (num & 0xf0) >> 4;
  if (tmp < 10)
    uart_putc('0'+tmp);
  else
    uart_putc('a'+tmp-10);

  tmp = num & 0x0f;
  if (tmp < 10)
    uart_putc('0'+tmp);
  else
    uart_putc('a'+tmp-10);
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

static int ioputc(char c, FILE *stream) {
  if (c == '\n') uart_putc('\r');
  uart_putc(c);
  return 0;
}

uint8_t uart_getc(void) {
  loop_until_bit_is_set(UCSRA,RXC);
  return UDR;
}

void uart_flush(void) {
}

void uart_puts_P(prog_char *text) {
  uint8_t ch;

  while ((ch = pgm_read_byte(text++))) {
    uart_putc(ch);
  }
}

void uart_putcrlf(void) {
  uart_putc(13);
  uart_putc(10);
}

static FILE mystdout = FDEV_SETUP_STREAM(ioputc, NULL, _FDEV_SETUP_WRITE);

void init_serial(void) {
  /* Seriellen Port konfigurieren */

  UBRRH = (int)((double)F_CPU/(16.0*CONFIG_UART_BAUDRATE)-1) >> 8;
  UBRRL = (int)((double)F_CPU/(16.0*CONFIG_UART_BAUDRATE)-1) & 0xff;

  UCSRB = _BV(RXEN) | _BV(TXEN);
  // I really don't like random #ifdefs in the code =(
#if defined __AVR_ATmega644__ || defined __AVR_ATmega644P__ || defined __AVR_ATmega128__ ||  defined __AVR_ATmega1281__
  UCSRC = _BV(UCSZ1) | _BV(UCSZ0);
#elif defined __AVR_ATmega32__
  UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
#else
#  error Unknown chip!
#endif

  stdout = &mystdout;
}
