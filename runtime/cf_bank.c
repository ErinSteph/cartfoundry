#include "cf_bank.h"
/* Always resident: restore the caller’s mapped bank before returning. */
void cf_bank_copy(uint8_t bank,const uint8_t *source,uint8_t *destination,uint16_t length) NONBANKED {
    uint8_t saved;
    CRITICAL {saved=CURRENT_BANK;SWITCH_ROM(bank);while(length--)*(volatile uint8_t *)destination++=*source++;SWITCH_ROM(saved);}
}

#include "cf_collision.h"
const CFResource *cf_collision_resource;
uint8_t cf_collision_at(const uint8_t *unused,uint16_t cell) NONBANKED {
 (void)unused;return (cf_resource_read(cf_collision_resource,cell/8)>>(cell&7))&1;
}
