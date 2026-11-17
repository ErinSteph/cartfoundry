#include "cf_bank.h"
/* Always resident: restore bank 1 before returning to the main runtime. */
void cf_bank_copy(uint8_t bank,const uint8_t *source,uint8_t *destination,uint16_t length) NONBANKED {
    uint8_t saved;
    CRITICAL {saved=CURRENT_BANK;SWITCH_ROM(bank);while(length--)*(volatile uint8_t *)destination++=*source++;SWITCH_ROM(saved);}
}
