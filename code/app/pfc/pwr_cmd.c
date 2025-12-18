#include "pwr_cmd.h"
#include "gpio.h"
#include "stdint.h"
#include "section.h"

uint8_t single_board_en = 0;
REG_SHELL_VAR(SGBD_EN, single_board_en, SHELL_UINT8, 1, 0, NULL, SHELL_STA_NULL)

uint8_t pwr_cmd_allow_dsg(void)
{
    if (single_board_en == 1)
    {
        return 1;
    }
    return (gpio_get_from_llc() == 0) ? 1 : 0;
}
