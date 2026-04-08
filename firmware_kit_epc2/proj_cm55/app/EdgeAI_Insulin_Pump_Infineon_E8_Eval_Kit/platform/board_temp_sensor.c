#include "board_temp_sensor.h"

#include "cy_i3c.h"
#include "cy_i3c_ccc.h"
#include "cycfg_peripherals.h"

#define BOARD_TEMP_REG      (0x00u)
#define BOARD_TEMP_ADDR_MIN (0x48u)
#define BOARD_TEMP_ADDR_MAX (0x4Bu)
#define BOARD_TEMP_ADDR_DYN (0x08u)

static cy_stc_i3c_context_t s_i3c_ctx;
static bool s_inited = false;
static bool s_ready = false;
static uint8_t s_addr = BOARD_TEMP_ADDR_MIN;

static bool board_temp_decode_c10(uint8_t msb, uint8_t lsb, int16_t *temp_c10_out)
{
    int16_t raw12;
    int32_t temp_c10;

    if (temp_c10_out == NULL)
    {
        return false;
    }

    raw12 = (int16_t)((((uint16_t)msb << 8) | lsb) >> 4);
    if ((raw12 & 0x0800) != 0)
    {
        raw12 |= (int16_t)0xF000;
    }

    temp_c10 = ((int32_t)raw12 * 5) / 8;
    if ((temp_c10 < -400) || (temp_c10 > 1250))
    {
        return false;
    }

    *temp_c10_out = (int16_t)temp_c10;
    return true;
}

static bool board_temp_read_raw(uint8_t addr, uint8_t *msb_out, uint8_t *lsb_out)
{
    cy_en_i3c_status_t st;
    uint8_t msb = 0u;
    uint8_t lsb = 0u;

    if ((msb_out == NULL) || (lsb_out == NULL))
    {
        return false;
    }

    st = Cy_I3C_ControllerWriteByte(CYBSP_I3C_CONTROLLER_HW, addr, (int8_t)BOARD_TEMP_REG, &s_i3c_ctx);
    if (st != CY_I3C_SUCCESS)
    {
        return false;
    }

    st = Cy_I3C_ControllerReadByte(CYBSP_I3C_CONTROLLER_HW, addr, &msb, &s_i3c_ctx);
    if (st != CY_I3C_SUCCESS)
    {
        return false;
    }

    st = Cy_I3C_ControllerReadByte(CYBSP_I3C_CONTROLLER_HW, addr, &lsb, &s_i3c_ctx);
    if (st != CY_I3C_SUCCESS)
    {
        return false;
    }

    *msb_out = msb;
    *lsb_out = lsb;
    return true;
}

static bool board_temp_probe_legacy_i2c(int16_t *temp_c10_out)
{
    cy_stc_i2c_device_t dev;
    uint8_t addr;
    uint8_t msb;
    uint8_t lsb;
    int16_t temp_c10;

    for (addr = BOARD_TEMP_ADDR_MIN; addr <= BOARD_TEMP_ADDR_MAX; ++addr)
    {
        dev.staticAddress = addr;
        dev.lvr = 0u; /* Fm+ capable legacy I2C device */
        (void)Cy_I3C_ControllerAttachI2CDevice(CYBSP_I3C_CONTROLLER_HW, &dev, &s_i3c_ctx);

        if (board_temp_read_raw(addr, &msb, &lsb) && board_temp_decode_c10(msb, lsb, &temp_c10))
        {
            s_addr = addr;
            s_ready = true;
            if (temp_c10_out != NULL)
            {
                *temp_c10_out = temp_c10;
            }
            return true;
        }
    }
    return false;
}

static bool board_temp_probe_dynamic_i3c(int16_t *temp_c10_out)
{
    cy_stc_i3c_ccc_cmd_t ccc_cmd;
    cy_stc_i3c_ccc_payload_t payload;
    cy_stc_i3c_ccc_setda_t setda;
    cy_en_i3c_status_t st;
    uint8_t msb;
    uint8_t lsb;
    int16_t temp_c10;

    /* Clear stale dynamic addresses first (best effort). */
    ccc_cmd.cmd = CY_I3C_CCC_RSTDAA(true);
    ccc_cmd.dbp = false;
    ccc_cmd.db = 0u;
    ccc_cmd.data = NULL;
    ccc_cmd.address = CY_I3C_BROADCAST_ADDR;
    (void)Cy_I3C_SendCCCCmd(CYBSP_I3C_CONTROLLER_HW, &ccc_cmd, &s_i3c_ctx);

    /* Assign dynamic address to static 0x48 target. */
    setda.address = BOARD_TEMP_ADDR_DYN;
    payload.len = 1u;
    payload.data = &setda;
    ccc_cmd.cmd = CY_I3C_CCC_SETDASA;
    ccc_cmd.dbp = false;
    ccc_cmd.db = 0u;
    ccc_cmd.data = &payload;
    ccc_cmd.address = BOARD_TEMP_ADDR_MIN;

    st = Cy_I3C_SetDASA(CYBSP_I3C_CONTROLLER_HW, &ccc_cmd, &s_i3c_ctx);
    if (st != CY_I3C_SUCCESS)
    {
        return false;
    }

    if (!board_temp_read_raw(BOARD_TEMP_ADDR_DYN, &msb, &lsb) ||
        !board_temp_decode_c10(msb, lsb, &temp_c10))
    {
        return false;
    }

    s_addr = BOARD_TEMP_ADDR_DYN;
    s_ready = true;
    if (temp_c10_out != NULL)
    {
        *temp_c10_out = temp_c10;
    }
    return true;
}

void board_temp_sensor_init(void)
{
    cy_en_i3c_status_t st;
    int16_t temp_c10;

    if (s_inited)
    {
        return;
    }
    s_inited = true;

    st = Cy_I3C_Init(CYBSP_I3C_CONTROLLER_HW, &CYBSP_I3C_CONTROLLER_config, &s_i3c_ctx);
    if (st != CY_I3C_SUCCESS)
    {
        return;
    }
    Cy_I3C_Enable(CYBSP_I3C_CONTROLLER_HW, &s_i3c_ctx);
    if (!board_temp_probe_dynamic_i3c(&temp_c10))
    {
        (void)board_temp_probe_legacy_i2c(&temp_c10);
    }
}

bool board_temp_sensor_read_c10(int16_t *temp_c10_out)
{
    uint8_t msb;
    uint8_t lsb;
    int16_t temp_c10;

    if (temp_c10_out == NULL)
    {
        return false;
    }

    if (!s_inited)
    {
        board_temp_sensor_init();
    }
    if (!s_ready)
    {
        return false;
    }

    if (!board_temp_read_raw(s_addr, &msb, &lsb))
    {
        /* Attempt one recovery scan/rebind on bus or address changes. */
        s_ready = false;
        if (!board_temp_probe_dynamic_i3c(&temp_c10) &&
            !board_temp_probe_legacy_i2c(&temp_c10))
        {
            return false;
        }
        *temp_c10_out = temp_c10;
        return true;
    }
    if (!board_temp_decode_c10(msb, lsb, &temp_c10))
    {
        /* Data decode failed; try one re-probe and fresh read. */
        s_ready = false;
        if (!board_temp_probe_dynamic_i3c(&temp_c10) &&
            !board_temp_probe_legacy_i2c(&temp_c10))
        {
            return false;
        }
        *temp_c10_out = temp_c10;
        return true;
    }

    *temp_c10_out = temp_c10;
    return true;
}
