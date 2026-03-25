/*******************************************************************************
* \file mtb_disp_ws7p0dsi_drv.h
*
* \brief
* This file provides constants, parameter values, and API prototypes for
* Waveshare 7-inch Raspberry Pi DSI LCD (C) Display driver.
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

#ifndef MTB_DISP_WS7P0DSI_RPI
#define MTB_DISP_WS7P0DSI_RPI


#if defined(__cplusplus)
extern "C" {
#endif


/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_pdl.h"
#include "cy_graphics.h"


/*******************************************************************************
* Macros
*******************************************************************************/
/* Tessellation window = DISPLAY_W * DISPLAY_H */
/* Horizontal Display resolution of Waveshare 7-inch DSI LCD (C) Display */
#define DISP_WS7P0DSI_DEFAULT_RES_X         (1024U)
/* Vertical Display resolution of Waveshare 7-inch DSI LCD (C) Display */
#define DISP_WS7P0DSI_DEFAULT_RES_Y         (600U)
/* Waveshare 7-inch Raspberry Pi DSI LCD (C) Display driver error */
#define WS7IN_DSIC_DISP_DRIVER_ERR          (1U)


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Display specific config parameters, these are given as input to the graphics
 * personality in the MTB device configurator tool
 */
extern cy_stc_mipidsi_display_params_t mtb_disp_ws7p0dsi_dsi_display_params;
/* Display specific DSI config parameters. The values for two of the struct
 * members - "per_lane_mbps" and "max_phy_clk" are calculated by the MTB device
 * configurator tool based on the input given to the graphics personality.
 */
extern cy_stc_mipidsi_config_t mtb_disp_ws7p0dsi_dsi_config;


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
/* Initializes Waveshare 7-inch Raspberry Pi DSI Display using I2C interface. */
cy_rslt_t mtb_disp_ws7p0dsi_panel_init(CySCB_Type* i2c_base,
                                       cy_stc_scb_i2c_context_t* i2c_context);

/* De-initializes Waveshare 7-inch Raspberry Pi DSI Display using I2C interface. */
cy_rslt_t mtb_disp_ws7p0dsi_panel_deinit(CySCB_Type* i2c_base,
                                         cy_stc_scb_i2c_context_t* i2c_context);

/* Sets the brightness of Waveshare 7-inch Raspberry Pi DSI Display using
 * I2C interface. */
cy_rslt_t mtb_disp_ws7p0dsi_panel_brightness_ctrl(CySCB_Type* i2c_base,
                                                  cy_stc_scb_i2c_context_t* i2c_context,
                                                  uint8_t value);

#if defined(__cplusplus)
}
#endif /* __cplusplus */


#endif /* MTB_DISP_WS7P0DSI_RPI */

/* [] END OF FILE */
