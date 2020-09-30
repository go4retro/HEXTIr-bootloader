/* FAT bootloader

   Initial code from mikrocontroller.net, additional code and Makefile system
   from sd2iec.

*/

#ifndef CONFIG_H
#define CONFIG_H

#include "autoconf.h"

#ifdef CONFIG_UART_DEBUG
  #define UART0_ENABLE
  #define UART0_BAUDRATE CONFIG_UART_BAUDRATE
  #define UART_DOUBLE_SPEED
#endif

#if CONFIG_HARDWARE_VARIANT==1
/* Hardware configuration: HEX-TI-r */
/* SD Card supply voltage - choose the one appropiate to your board */
/* #  define SD_SUPPLY_VOLTAGE (1L<<15)  / * 2.7V - 2.8V */
/* #  define SD_SUPPLY_VOLTAGE (1L<<16)  / * 2.8V - 2.9V */
/* #  define SD_SUPPLY_VOLTAGE (1L<<17)  / * 2.9V - 3.0V */
/* #  define SD_SUPPLY_VOLTAGE (1L<<18)  / * 3.0V - 3.1V */
/* #  define SD_SUPPLY_VOLTAGE (1L<<19)  / * 3.1V - 3.2V */
/* #  define SD_SUPPLY_VOLTAGE (1L<<20)  / * 3.2V - 3.3V */
#  define SD_SUPPLY_VOLTAGE (1L<<21)  /* 3.3V - 3.4V */
/* #  define SD_SUPPLY_VOLTAGE (1L<<22)  / * 3.4V - 3.5V */
/* #  define SD_SUPPLY_VOLTAGE (1L<<23)  / * 3.5V - 3.6V */

/* CARD_DETECT must return non-zero when card is inserted */
/* This must be a pin capable of generating interrupts.   */
#  define SDCARD_DETECT         (!(PINB & _BV(PIN1)))
#  define SDCARD_DETECT_SETUP() do { DDRB &= ~_BV(PIN1); PORTB |= _BV(PIN1); } while(0)
#  define SD_CHANGE_SETUP()     do { } while(0)

#  define USE_FLASH_LED
#  define FLASH_LED_PORT PORTC
#  define FLASH_LED_DDR DDRC
#  define FLASH_LED_PIN PC5
#  define FLASH_LED_POLARITY 1

#  define USE_ALIVE_LED
#  define ALIVE_LED_PORT PORTD
#  define ALIVE_LED_DDR DDRD
#  define ALIVE_LED_PIN PIN3

//#  define USE_FAT12
#define USE_FAT32

#  define INIT_RETRIES 10

#else
#  error "CONFIG_HARDWARE_VARIANT is unset or set to an unknown value."
#endif



/* ---------------- End of user-configurable options ---------------- */

#endif
