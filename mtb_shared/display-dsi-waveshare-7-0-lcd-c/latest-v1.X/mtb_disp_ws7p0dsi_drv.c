/*******************************************************************************
* \file mtb_disp_ws7p0dsi_drv.c
* \version 1.0
*
* \brief
* This source file provides driver implementation for Waveshare 7-inch
* Raspberry Pi DSI LCD (C) Display.
*
********************************************************************************
* \copyright
* Copyright 2025 Cypress Semiconductor Corporation (an Infineon company)
*
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "mtb_disp_ws7p0dsi_drv.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#define BRIGHTNESS_LEVEL_ADDR               (0xABU)

/* The brightness levels are inverted in this panel, value "0x00" means full
 * brightness, value "0xFF" means minimum brightness.
 */
#define BRIGHTNESS_LEVEL_FULL               (0x00U)

#define PANEL_ENABLE_CTRL_ADDR              (0xADU)
#define PANEL_ENABLE_CMD                    (0x01U)
#define PANEL_DISABLE_CMD                   (0x00U)
#define PANEL_STOP_CMD_TIMEOUT_MS           (2000U)
#define PANEL_CMD_SEND_TIMEOUT_MS           (1000000UL)

/* I2C slave address to communicate with */
#define WS7IN_DSIC_DISP_DRIVER_SYS_CTRL_ADDR                (0x45U)
#define WS7IN_DSIC_CMD_PACKET_SIZE                          (2UL)
#define WS7IN_DSIC_DEINIT_CMD_DATA_SIZE                     (2UL)
#define WS7IN_DSIC_DISP_DRIVER_SYS_START_WAIT_MS            (500U)
#define WS7IN_DSIC_DISP_DRIVER_SYS_PRG_WAIT_MS              (500U)
#define WS7IN_DSIC_DISP_INTER_CMD_DELAY                     (100U)
#define WS7IN_DSIC_DISP_DEINIT_CMD_DELAY                    (100U)
#define BRIGHTNESS_CMD_PKT_POS                              (0U)
#define BRIGHTNESS_VAL_POS                                  (1U)
#define WS7IN_DSIC_DISP_BRIGHTNESS_MIN_VAL                  (0U)
#define WS7IN_DSIC_DISP_BRIGHTNESS_MAX_VAL                  (255U)

/* Display specific config parameters MIPI DSI */
#define WS7IN_DSIC_DISP_PARAM_PIXEL_CLK_KHZ                 (41743U)
#define WS7IN_DSIC_DISP_PARAM_HSYNC_WIDTH                   (100U)
#define WS7IN_DSIC_DISP_PARAM_HFP                           (125U)
#define WS7IN_DSIC_DISP_PARAM_HBP                           (20U)
#define WS7IN_DSIC_DISP_PARAM_VSYNC_WIDTH                   (5U)
#define WS7IN_DSIC_DISP_PARAM_VFP                           (20U)
#define WS7IN_DSIC_DISP_PARAM_VBP                           (20U)
#define WS7IN_DSIC_DISP_PARAM_POLARITY_FLAGS                (0U)

#define WS7IN_DSIC_DISP_DSI_CONFIG_VIRTUAL_CH               (0U)
#define WS7IN_DSIC_DISP_DSI_CONFIG_NUM_OF_LANES             (2U)
#define WS7IN_DSIC_DISP_DSI_CONFIG_PER_LANE_MBPS            (500U)
#define WS7IN_DSIC_DISP_DSI_CONFIG_MAX_PHY_CLK              (2500000000U)


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Display specific MIPI DSI params (can be configured using gfxss personality
 * in ModusToolbox device configurator tool)
 */
cy_stc_mipidsi_display_params_t mtb_disp_ws7p0dsi_dsi_display_params =
{
    .pixel_clock    = WS7IN_DSIC_DISP_PARAM_PIXEL_CLK_KHZ,
    .hdisplay       = DISP_WS7P0DSI_DEFAULT_RES_X,
    .hsync_width    = WS7IN_DSIC_DISP_PARAM_HSYNC_WIDTH,
    .hfp            = WS7IN_DSIC_DISP_PARAM_HFP,
    .hbp            = WS7IN_DSIC_DISP_PARAM_HBP,
    .vdisplay       = DISP_WS7P0DSI_DEFAULT_RES_Y,
    .vsync_width    = WS7IN_DSIC_DISP_PARAM_VSYNC_WIDTH,
    .vfp            = WS7IN_DSIC_DISP_PARAM_VFP,
    .vbp            = WS7IN_DSIC_DISP_PARAM_VBP,
    .polarity_flags = WS7IN_DSIC_DISP_PARAM_POLARITY_FLAGS,
};

/* Display specific DSI config parameters */
cy_stc_mipidsi_config_t mtb_disp_ws7p0dsi_dsi_config =
{
    .virtual_ch     = WS7IN_DSIC_DISP_DSI_CONFIG_VIRTUAL_CH,
    .num_of_lanes   = WS7IN_DSIC_DISP_DSI_CONFIG_NUM_OF_LANES,
    /* Per lane speed is calculated by the MTB device configurator tool based
     * on the input given to the graphics personality.
     */
    .per_lane_mbps  = WS7IN_DSIC_DISP_DSI_CONFIG_PER_LANE_MBPS,
    .dpi_fmt        = CY_MIPIDSI_FMT_RGB888,
    .dsi_mode       = DSI_VIDEO_MODE,
    /* Max DSI PHY clock speed is calculated by the MTB device configurator tool
     * based on the input given to the graphics personality.
     */
    .max_phy_clk    = WS7IN_DSIC_DISP_DSI_CONFIG_MAX_PHY_CLK,
    .mode_flags     = VID_MODE_TYPE_BURST | ENABLE_LOW_POWER_CMD | ENABLE_LOW_POWER,
    .display_params = &mtb_disp_ws7p0dsi_dsi_display_params,
};

/* Init commands for the display driver system, the display driver system
 * expects a stop condition after every command set(one row of command array)
 */
static uint8_t ws_7_inch_dsi_panel_init_cmd[][WS7IN_DSIC_CMD_PACKET_SIZE] =
{
    { PANEL_ENABLE_CTRL_ADDR, PANEL_DISABLE_CMD                       },
    { 0xC0,                   0x01                                    },
    { 0xC2,                   0x01                                    },
    { 0xAC,                   0x01                                    },
    { PANEL_ENABLE_CTRL_ADDR, PANEL_ENABLE_CMD                        },
    { BRIGHTNESS_LEVEL_ADDR,  BRIGHTNESS_LEVEL_FULL                   },
    { 0xAA,                   0x01                                    },
};

/* De-Init commands for the display driver system, the display driver system
 * expects a stop condition after every command set (one row of command array)
 */
static uint8_t ws_7_inch_dsi_panel_deinit_cmd[WS7IN_DSIC_DEINIT_CMD_DATA_SIZE] =
    { PANEL_ENABLE_CTRL_ADDR, PANEL_DISABLE_CMD };

/* Brightness control commands the display driver system expects a stop condition
 * after every command set (one row of command array)
 */
static uint8_t ws_7_inch_dsi_panel_brightness_cmd[][WS7IN_DSIC_CMD_PACKET_SIZE] =
{
    { BRIGHTNESS_LEVEL_ADDR, BRIGHTNESS_LEVEL_FULL                  },
    { 0xAA,                  0x01                                   },
};


/*******************************************************************************
* Function name: mtb_disp_ws7p0dsi_panel_init
********************************************************************************
* Initializes Waveshare 7-inch Raspberry Pi DSI LCD (C) Display
* using I2C interface.
*
* \param i2c_base
* Pointer to I2C base.
*
* \param i2c_context
* Pointer to I2C context.
*
* \return cy_rslt_t
* Read operation status.
*
*******************************************************************************/
cy_rslt_t mtb_disp_ws7p0dsi_panel_init(CySCB_Type* i2c_base,
                                       cy_stc_scb_i2c_context_t* i2c_context)
{
    cy_en_scb_i2c_status_t i2c_result;
    cy_stc_scb_i2c_master_xfer_config_t i2c_xfer_config;

    uint32_t timeout;

    uint32_t wspanel70c_cmd_size = CY_ARRAY_SIZE(ws_7_inch_dsi_panel_init_cmd);

    /* Wait for the display control system on-board the display to start-up */
    Cy_SysLib_Delay(WS7IN_DSIC_DISP_DRIVER_SYS_START_WAIT_MS);

    i2c_xfer_config.bufferSize = WS7IN_DSIC_CMD_PACKET_SIZE;
    i2c_xfer_config.slaveAddress = WS7IN_DSIC_DISP_DRIVER_SYS_CTRL_ADDR;

    for (int i = 0; i < wspanel70c_cmd_size; i++)
    {
        i2c_xfer_config.buffer = ws_7_inch_dsi_panel_init_cmd[i];
        i2c_xfer_config.xferPending = true;

        timeout =  PANEL_CMD_SEND_TIMEOUT_MS;

        /* Send display's command data */
        i2c_result = Cy_SCB_I2C_MasterWrite(i2c_base, &i2c_xfer_config, i2c_context);

        if (CY_SCB_I2C_SUCCESS == i2c_result)
        {
            do
            {
                i2c_result = (cy_en_scb_i2c_status_t)Cy_SCB_I2C_MasterGetStatus(i2c_base,
                                                                                i2c_context);
                Cy_SysLib_DelayUs(CY_SCB_WAIT_1_UNIT);
                timeout--;
            } while((i2c_result & CY_SCB_I2C_MASTER_BUSY) && (timeout));

            if (!timeout)
            {
                return WS7IN_DSIC_DISP_DRIVER_ERR;
            }
        }

        /* Check if all command's data sent correctly. */
        if (WS7IN_DSIC_CMD_PACKET_SIZE !=
            Cy_SCB_I2C_MasterGetTransferCount(i2c_base, i2c_context))
        {
            return WS7IN_DSIC_DISP_DRIVER_ERR;
        }

        /* The display driver system expects a stop condition after every
         * command set (one row of command array).
         */
        i2c_result = Cy_SCB_I2C_MasterSendStop(i2c_base,
                                               PANEL_STOP_CMD_TIMEOUT_MS, i2c_context);

        if (CY_SCB_I2C_SUCCESS != i2c_result)
        {
            return i2c_result;
        }

        Cy_SysLib_Delay(WS7IN_DSIC_DISP_INTER_CMD_DELAY);
    }

    /* Wait for the display control system on-board the display to be
     * programmed
     */
    Cy_SysLib_Delay(WS7IN_DSIC_DISP_DRIVER_SYS_PRG_WAIT_MS);

    return CY_RSLT_SUCCESS;
}


/*******************************************************************************
* Function name: mtb_disp_ws7p0dsi_panel_deinit
********************************************************************************
* De-initializes Waveshare 7-inch Raspberry Pi DSI LCD (C) Display
* using I2C interface.
*
* \param i2c_base
* Pointer to I2C base.
*
* \param i2c_context
* Pointer to I2C context.
*
* \return cy_rslt_t
* Read operation status.
*
*******************************************************************************/
cy_rslt_t mtb_disp_ws7p0dsi_panel_deinit(CySCB_Type* i2c_base,
                                         cy_stc_scb_i2c_context_t* i2c_context)
{
    cy_en_scb_i2c_status_t i2c_result;
    cy_stc_scb_i2c_master_xfer_config_t i2c_xfer_config;

    /* Timeout 1 second (one unit is us) */
    uint32_t timeout = PANEL_CMD_SEND_TIMEOUT_MS;

    /* Config I2C transfer for display de-init cmds */
    i2c_xfer_config.bufferSize = WS7IN_DSIC_CMD_PACKET_SIZE;
    i2c_xfer_config.slaveAddress = WS7IN_DSIC_DISP_DRIVER_SYS_CTRL_ADDR;
    i2c_xfer_config.buffer = ws_7_inch_dsi_panel_deinit_cmd;
    i2c_xfer_config.xferPending = true;

    /* Start display's command data */
    i2c_result = Cy_SCB_I2C_MasterWrite(i2c_base, &i2c_xfer_config, i2c_context);

    if (CY_SCB_I2C_SUCCESS == i2c_result)
    {
        do
        {
            /* Wait for master to send all data */
            i2c_result = (cy_en_scb_i2c_status_t)Cy_SCB_I2C_MasterGetStatus(i2c_base, i2c_context);
            Cy_SysLib_DelayUs(CY_SCB_WAIT_1_UNIT);
            timeout--;
        } while ((0UL != (i2c_result & CY_SCB_I2C_MASTER_BUSY)) && (timeout > 0));

        if (timeout <= 0)
        {
            return WS7IN_DSIC_DISP_DRIVER_ERR;
        }
    }

    /* Check if all command's data sent correctly. */
    if (WS7IN_DSIC_CMD_PACKET_SIZE !=
        Cy_SCB_I2C_MasterGetTransferCount(i2c_base, i2c_context))
    {
        return WS7IN_DSIC_DISP_DRIVER_ERR;
    }

    /* The display driver system expects a stop condition after every
     * command set(one row of command array).
     */
    i2c_result = Cy_SCB_I2C_MasterSendStop(i2c_base,
                                           PANEL_STOP_CMD_TIMEOUT_MS, i2c_context);

    if (CY_SCB_I2C_SUCCESS != i2c_result)
    {
        return i2c_result;
    }

    Cy_SysLib_Delay(WS7IN_DSIC_DISP_DEINIT_CMD_DELAY);

    return CY_RSLT_SUCCESS;
}


/*******************************************************************************
* Function name: mtb_disp_ws7p0dsi_panel_brightness_ctrl
********************************************************************************
* Controls the brightness of Waveshare 7-inch Raspberry Pi DSI LCD (C) Display
* using I2C interface.
*
* \param *i2c_base
* Pointer to I2C base.
*
* \param *i2c_context
* Pointer to I2C.
*
* \param value
* Required brightness value for the display panel.
*
* \return cy_rslt_t
* Read operation status.
*
*******************************************************************************/
cy_rslt_t mtb_disp_ws7p0dsi_panel_brightness_ctrl(CySCB_Type* i2c_base,
                                                  cy_stc_scb_i2c_context_t* i2c_context,
                                                  uint8_t value)
{
    cy_en_scb_i2c_status_t i2c_result;
    cy_stc_scb_i2c_master_xfer_config_t i2c_xfer_config;

    uint32_t timeout;

    /* Calculate size of brightness cmd data. */
    uint32_t wspanel70c_cmd_size = CY_ARRAY_SIZE(ws_7_inch_dsi_panel_brightness_cmd);

    /* Brightness values are inverted in this panel. */
    value = WS7IN_DSIC_DISP_BRIGHTNESS_MAX_VAL - value;

    /* Brightness value updated in the command packet. */
    ws_7_inch_dsi_panel_brightness_cmd[BRIGHTNESS_CMD_PKT_POS][BRIGHTNESS_VAL_POS] =
        value;

    i2c_xfer_config.bufferSize = WS7IN_DSIC_CMD_PACKET_SIZE;
    i2c_xfer_config.slaveAddress = WS7IN_DSIC_DISP_DRIVER_SYS_CTRL_ADDR;

    for (int i = 0; i < wspanel70c_cmd_size; i++)
    {
        i2c_xfer_config.buffer = ws_7_inch_dsi_panel_brightness_cmd[i];
        i2c_xfer_config.xferPending = true;

        timeout =  PANEL_CMD_SEND_TIMEOUT_MS;

        /* Send display's command data */
        i2c_result = Cy_SCB_I2C_MasterWrite(i2c_base, &i2c_xfer_config, i2c_context);

        if (CY_SCB_I2C_SUCCESS == i2c_result)
        {
            do
            {
                i2c_result = (cy_en_scb_i2c_status_t)Cy_SCB_I2C_MasterGetStatus(i2c_base,
                                                                                i2c_context);
                Cy_SysLib_DelayUs(CY_SCB_WAIT_1_UNIT);
                timeout--;
            } while((0UL != (i2c_result & CY_SCB_I2C_MASTER_BUSY)) && (timeout > 0));

            if (timeout <= 0)
            {
                return WS7IN_DSIC_DISP_DRIVER_ERR;
            }
        }

        /* Check if all command's data sent correctly. */
        if (WS7IN_DSIC_CMD_PACKET_SIZE !=
            Cy_SCB_I2C_MasterGetTransferCount(i2c_base, i2c_context))
        {
            return WS7IN_DSIC_DISP_DRIVER_ERR;
        }

        /* The display driver system expects a stop condition after every
         * command set(one row of command array).
         */
        i2c_result = Cy_SCB_I2C_MasterSendStop(i2c_base,
                                               PANEL_STOP_CMD_TIMEOUT_MS,
                                               i2c_context);

        if (CY_SCB_I2C_SUCCESS != i2c_result)
        {
            return i2c_result;
        }
    }

    return CY_RSLT_SUCCESS;
}


/* [] END OF FILE */
