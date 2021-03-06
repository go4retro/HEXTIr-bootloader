#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>
#include "config.h"
#include "fat.h"
#include "uart.h"


typedef struct
{
  uint32_t dev_id;
  uint16_t app_version;
  uint16_t crc;
} bootldrinfo_t;


sector_t updatecluster; //is set when update is available
bootldrinfo_t current_bootldrinfo;

void (*app_start)(void) = 0x0000;


/* Make sure the watchdog is disabled as soon as possible    */
/* Copy this code to your bootloader if you use one and your */
/* MCU doesn't disable the WDT after reset!                  */
void get_mcusr(void) \
      __attribute__((naked)) \
      __attribute__((section(".init3")));
void get_mcusr(void)
{
  MCUSR = 0;
  wdt_disable();
}


#ifdef CONFIG_FLASH_CRC_CHECK
static inline uint16_t crc_flash(void) {
  uint32_t adr;
  uint16_t flash_crc;

  for (adr=0,flash_crc = 0xFFFF; adr<FLASHEND - BOOTLDRSIZE + 1; adr++)
    flash_crc = _crc_ccitt_update(flash_crc, pgm_read_byte(adr));

  return flash_crc;
}
#endif

static inline uint16_t crc_file(void)
{
  uint16_t filesector;
  uint16_t index;
  uint16_t flash_crc = 0xFFFF;

  for (filesector = 0; filesector < (FLASHEND - BOOTLDRSIZE + 1) / 512; filesector++)
  {
#   ifdef USE_FLASH_LED
    FLASH_LED_PORT ^= 1<<FLASH_LED_PIN;
#   endif

    fat_readfilesector(filestart, filesector);

    for (index=0; index < 512; index++)
    {
      flash_crc = _crc_ccitt_update(flash_crc, fat_buf[index]);
	}
  }

  //LED on
# ifdef USE_FLASH_LED
#   if FLASH_LED_POLARITY
  FLASH_LED_PORT &= ~(1<<FLASH_LED_PIN);
#   else
  FLASH_LED_PORT |= 1<<FLASH_LED_PIN;
#   endif
# endif

  return flash_crc; // result is ZERO when CRC is okay
}


static inline void check_file(void)
{
  //Check filesize

  if (filesize != FLASHEND - BOOTLDRSIZE + 1)
    return;

  bootldrinfo_t *file_bootldrinfo;
  fat_readfilesector(filestart, (FLASHEND - BOOTLDRSIZE + 1) / 512 - 1);

  file_bootldrinfo =  (bootldrinfo_t*) (uint8_t*) (fat_buf + (FLASHEND - BOOTLDRSIZE - sizeof(bootldrinfo_t) + 1) % 512);

  //Check DEVID
  if (file_bootldrinfo->dev_id != CONFIG_BOOT_DEVID)
    return;

  //Check application version
  if (file_bootldrinfo->app_version <= current_bootldrinfo.app_version &&
      file_bootldrinfo->app_version   != 0 &&
      current_bootldrinfo.app_version != 0)
    return;

  // If development version in flash and in file,
        // check for different crc
  if (current_bootldrinfo.app_version == 0 &&
    file_bootldrinfo->app_version   == 0 &&
    current_bootldrinfo.crc == file_bootldrinfo->crc)
    return;

# ifdef CONFIG_FILE_CRC_CHECK
  // check CRC of file
  if(crc_file() != 0)
    return;
# endif

  current_bootldrinfo.app_version = file_bootldrinfo->app_version;
  updatecluster = filestart;
}


static inline void flash_update(void)
{
  uint16_t filesector, j;
  uint8_t i;
  uint16_t *lpword;
  uint32_t adr;

  uart_putc('F'); // flash the new FW

  for (filesector = 0; filesector < (FLASHEND - BOOTLDRSIZE + 1) / 512; filesector++)
  {
# ifdef USE_FLASH_LED
    FLASH_LED_PORT ^= 1<<FLASH_LED_PIN;
# endif

    lpword = (uint16_t*) fat_buf;
    fat_readfilesector(updatecluster, filesector);

    for (i=0; i<(512 / SPM_PAGESIZE); i++)
    {
	    uart_putc('.'); // flash a block
      adr = (filesector * 512UL) + i * SPM_PAGESIZE;
      boot_page_erase(adr);
      while (boot_rww_busy())
        boot_rww_enable();

      for (j=0; j<SPM_PAGESIZE; j+=2) {
        boot_page_fill(adr + j, *lpword++);
      }

      boot_page_write(adr);
      while (boot_rww_busy())
        boot_rww_enable();

    }
  }
  uart_putcrlf();

  //LED on
# ifdef USE_FLASH_LED
#   if FLASH_LED_POLARITY
    FLASH_LED_PORT &= ~(1<<FLASH_LED_PIN);
#   else
    FLASH_LED_PORT |= 1<<FLASH_LED_PIN;
#   endif
# endif
}

int main(void)
{
  uint8_t res;
  uint16_t i;

  uart_init();
  //LED On
# ifdef USE_FLASH_LED
  FLASH_LED_DDR |= 1<<FLASH_LED_PIN;
#   if !FLASH_LED_POLARITY
  FLASH_LED_PORT |= 1<<FLASH_LED_PIN;
#   endif
# endif

# ifdef USE_ALIVE_LED
  ALIVE_LED_DDR |= 1<<ALIVE_LED_PIN;
  ALIVE_LED_PORT |= 1<<ALIVE_LED_PIN;
# endif

# if FLASHEND > 0xffff
  for(i = 0;i < sizeof(bootldrinfo_t);i++) {
    ((uint8_t*)&current_bootldrinfo)[i] = PGM_READ_BYTE(FLASHEND - BOOTLDRSIZE - sizeof(bootldrinfo_t) + 1 + i);
  }
# else
  memcpy_P(&current_bootldrinfo, (uint8_t*) FLASHEND - BOOTLDRSIZE - sizeof(bootldrinfo_t) + 1, sizeof(bootldrinfo_t));
# endif

  uart_putc('I'); // init finished

  if (current_bootldrinfo.app_version == 0xFFFF) {
    current_bootldrinfo.app_version = 0;    //application not flashed yet
# ifdef CONFIG_FLASH_CRC_CHECK
  } else {
    if(crc_flash())
       current_bootldrinfo.app_version = 0; //bad app code, reflash
# endif
  }

  if (fat_init() == ERR_OK) {
    uart_putc('S'); // search for new FW
    i = 0;
    do {
# ifdef USE_FLASH_LED
      FLASH_LED_PORT ^= 1<<FLASH_LED_PIN;
# endif

      res = fat_readRootDirEntry(i++);

      if (res == ERR_ENDOFDIR)
        break;

      if(res == ERR_OK)
        check_file();
    } while (i);
# ifdef USE_FLASH_LED
#   if FLASH_LED_POLARITY
    FLASH_LED_PORT &= ~(1<<FLASH_LED_PIN);
#   else
    FLASH_LED_PORT |= 1<<FLASH_LED_PIN;
#   endif
# endif

    if (updatecluster)
      flash_update();
  }
# ifdef CONFIG_FLASH_CRC_CHECK
  if(crc_flash() == 0)  {
    //Led off
#   ifdef USE_FLASH_LED
    FLASH_LED_DDR = 0x00;
#   endif
    uart_putc('g'); // go APP
    app_start();
  }
//  while (1);
# endif

    uart_putc('G'); // go APP
    app_start();
}
