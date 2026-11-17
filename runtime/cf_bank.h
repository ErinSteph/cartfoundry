#ifndef CF_BANK_H
#define CF_BANK_H
#include <gb/gb.h>
void cf_bank_copy(uint8_t bank,const uint8_t *source,uint8_t *destination,uint16_t length) NONBANKED;
#endif
