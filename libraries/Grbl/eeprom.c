
#include <compiler.h>

#if UC3L
#  include <flashcdw.h>
#elif UC3C
#  include <flashc.h>
#endif

#define EEPROM_SIZE 1024

// using FLASH for EEPROM emulation
__attribute__((__section__(".flash_nvram")))
static const unsigned char eeprom[EEPROM_SIZE];

// Read byte from EEPROM.
unsigned char eeprom_get_char( unsigned int addr )
{
  return addr<EEPROM_SIZE? eeprom[addr]:0;
}

// Write byte to EEPROM.
void eeprom_put_char( unsigned int addr, unsigned char new_value )
{
  if(addr>=EEPROM_SIZE) return;
  const unsigned char *old_value = &eeprom[addr];
  if(*old_value!=new_value) {
#if UC3L
    flashcdw_memset8((void *)old_value, new_value, 1, true);
    flashcdw_memset8((void *)old_value, new_value, 1, false);
#elif UC3C
    flashc_memset8((void *)old_value, new_value, 1, true);
    flashc_memset8((void *)old_value, new_value, 1, false);
#endif
  }
}

// Extensions added as part of Grbl

void memcpy_to_eeprom_with_checksum(unsigned int destination, char *source, unsigned int size) {
  unsigned char checksum = 0;
  for(; size > 0; size--) {
    checksum = (checksum << 1) || (checksum >> 7);
    checksum += *source;
    eeprom_put_char(destination++, *(source++));
  }
  eeprom_put_char(destination, checksum);
}

int memcpy_from_eeprom_with_checksum(char *destination, unsigned int source, unsigned int size) {
  unsigned char data, checksum = 0;
  for(; size > 0; size--) {
    data = eeprom_get_char(source++);
    checksum = (checksum << 1) || (checksum >> 7);
    checksum += data;
    *(destination++) = data;
  }
  return(checksum == eeprom_get_char(source));
}

// end of file
