/*******************************************************************************
* \file mtb_display_ek79007ad3.c
* \version 1.0.0
*
* \brief
* Provides implementation of the EK79007AD3 TFT DSI display driver library.
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
#include "mtb_display_ek79007ad3.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#define GPIO_LOW                                         (0U)
#define GPIO_HIGH                                        (1U)

#define ONE_MS_DELAY                                     (1U)
#define PIN_HIGH_DELAY_MS                                (5U)

/* MIPI DSI command packet size/length in bytes */
#define PACKET_LENGTH                                    (2U)

/* Display controller command commit mask */
#define DC_CMD_COMMIT_MASK                               (0x00000001U)

#define RESET_VAL                                        (0U)

#define MAX_BRIGHTNESS_PERCENT                           (100U)

/* Macro to convert brightness percentage into PWM counter value */
#define BRIGHTNESS_PERCENT_TO_PWM_COUNT(percentage, period) \
    ((uint8_t)(((percentage) * (period)) / 100))


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Display specific MIPI DSI params (can be configured using gfxss personality in ModusToolbox
 * device configurator tool)
 */
cy_stc_mipidsi_display_params_t mtb_display_ek79007ad3_mipidsi_display_params =
{
    .pixel_clock    = MTB_DISPLAY_EK79007AD3_PANEL_PIXEL_CLK,
    .hdisplay       = MTB_DISPLAY_EK79007AD3_DEFAULT_HOR_RES,
    .hsync_width    = MTB_DISPLAY_EK79007AD3_PANEL_HSYNC_WIDTH,
    .hfp            = MTB_DISPLAY_EK79007AD3_PANEL_HFP,
    .hbp            = MTB_DISPLAY_EK79007AD3_PANEL_HBP,
    .vdisplay       = MTB_DISPLAY_EK79007AD3_DEFAULT_VER_RES,
    .vsync_width    = MTB_DISPLAY_EK79007AD3_PANEL_VSYNC_WIDTH,
    .vfp            = MTB_DISPLAY_EK79007AD3_PANEL_VFP,
    .vbp            = MTB_DISPLAY_EK79007AD3_PANEL_VBP,
    .polarity_flags = RESET_VAL,
};

/* MIPI DSI block configuration */
cy_stc_mipidsi_config_t mtb_display_ek79007ad3_mipidsi_config =
{
    .virtual_ch     = RESET_VAL,
    .num_of_lanes   = MTB_DISPLAY_EK79007AD3_PANEL_NUM_LANES,
    .per_lane_mbps  = MTB_DISPLAY_EK79007AD3_PANEL_PER_LANE_MBPS,
    .dpi_fmt        = CY_MIPIDSI_FMT_RGB888,
    .dsi_mode       = DSI_VIDEO_MODE,
    .max_phy_clk    = MIPI_MAX_PHY_CLK_HZ,
    .mode_flags     = VID_MODE_TYPE_BURST | ENABLE_LOW_POWER_CMD | ENABLE_LOW_POWER,
    .display_params = &mtb_display_ek79007ad3_mipidsi_display_params,
};

cy_stc_tcpwm_pwm_config_t mtb_display_ek79007ad3_backlight_pwm_config =
{
    .pwmMode                      = CY_TCPWM_PWM_MODE_PWM,
    .clockPrescaler               = CY_TCPWM_PWM_PRESCALER_DIVBY_1,
    .pwmAlignment                 = CY_TCPWM_PWM_CENTER_ASYMMETRIC_CC0_CC1_ALIGN,
    .deadTimeClocks               = RESET_VAL,
    .runMode                      = CY_TCPWM_PWM_CONTINUOUS,
    .period0                      = MTB_DISPLAY_BACKLIGHT_PWM_PERIOD0,
    .period1                      = DEFAULT_PWM_PERIOD,
    .enablePeriodSwap             = false,
    .compare0                     = MTB_DISPLAY_BACKLIGHT_PWM_COMPARE0,
    .compare1                     = RESET_VAL,
    .enableCompareSwap            = true,
    .interruptSources             = (CY_TCPWM_INT_ON_TC & RESET_VAL) | \
                                    (CY_TCPWM_INT_ON_CC0 & RESET_VAL) | \
                                    (CY_TCPWM_INT_ON_CC1 & RESET_VAL),
    .invertPWMOut                 = CY_TCPWM_PWM_INVERT_DISABLE,
    .invertPWMOutN                = CY_TCPWM_PWM_INVERT_DISABLE,
    .killMode                     = CY_TCPWM_PWM_STOP_ON_KILL,
    .swapInputMode                = MTB_DISPLAY_BACKLIGHT_PWM_INPUT_DISABLED & CY_TCPWM_INPUT_LEVEL,
    .swapInput                    = CY_TCPWM_INPUT_0,
    .reloadInputMode              = MTB_DISPLAY_BACKLIGHT_PWM_INPUT_DISABLED & CY_TCPWM_INPUT_LEVEL,
    .reloadInput                  = CY_TCPWM_INPUT_0,
    .startInputMode               = MTB_DISPLAY_BACKLIGHT_PWM_INPUT_DISABLED & CY_TCPWM_INPUT_LEVEL,
    .startInput                   = CY_TCPWM_INPUT_0,
    .killInputMode                = MTB_DISPLAY_BACKLIGHT_PWM_INPUT_DISABLED & CY_TCPWM_INPUT_LEVEL,
    .killInput                    = CY_TCPWM_INPUT_0,
    .countInputMode               = MTB_DISPLAY_BACKLIGHT_PWM_INPUT_DISABLED & CY_TCPWM_INPUT_LEVEL,
    .countInput                   = CY_TCPWM_INPUT_1,
    .swapOverflowUnderflow        = false,
    .immediateKill                = false,
    .tapsEnabled                  = MTB_DISPLAY_BACKLIGHT_ENABLED_TAPS,
    .compare2                     = MTB_DISPLAY_BACKLIGHT_PWM_COMPARE2,
    .compare3                     = RESET_VAL,
    .enableCompare1Swap           = true,
    .compare0MatchUp              = true,
    .compare0MatchDown            = false,
    .compare1MatchUp              = false,
    .compare1MatchDown            = true,
    .kill1InputMode               = MTB_DISPLAY_BACKLIGHT_PWM_INPUT_DISABLED & CY_TCPWM_INPUT_LEVEL,
    .kill1Input                   = CY_TCPWM_INPUT_0,
    .pwmOnDisable                 = CY_TCPWM_PWM_OUTPUT_HIGHZ,
    .trigger0Event                = CY_TCPWM_CNT_TRIGGER_ON_DISABLED,
    .trigger1Event                = CY_TCPWM_CNT_TRIGGER_ON_DISABLED,
    .reloadLineSelect             = false,
    .line_out_sel                 = CY_TCPWM_OUTPUT_PWM_SIGNAL,
    .linecompl_out_sel            = CY_TCPWM_OUTPUT_INVERTED_PWM_SIGNAL,
    .line_out_sel_buff            = CY_TCPWM_OUTPUT_PWM_SIGNAL,
    .linecompl_out_sel_buff       = CY_TCPWM_OUTPUT_INVERTED_PWM_SIGNAL,
    .deadTimeClocks_linecompl_out = RESET_VAL,
};

static const uint8_t lcm_init_setting[][PACKET_LENGTH] =
{
    /* CTRL register address & value to configure 2 data lanes */
    { MTB_DISPLAY_EK79007AD3_PANEL_CTRL_REG, MTB_DISPLAY_EK79007AD3_EN_2LANE_MASK },
};

static mtb_display_ek79007ad3_pin_config_t* disp_pin_config = NULL;
static mtb_display_ek79007ad3_backlight_config_t* backlight_config = NULL;

/* Variable to store period value of TCPWM block */
static uint32_t pwm_period = RESET_VAL;


/*******************************************************************************
* Function name: mtb_display_ek79007ad3_init
********************************************************************************
*
* Performs EK79007AD3 TFT driver initialization using MIPI DSI interface.
*
* \param mipi_dsi_base
* Pointer to the MIPI DSI register base address.
*
* \param disp_ek79007ad3_pin_config
* Pointer to the EK79007AD3 display pin configuration structure.
*
* \return cy_en_mipidsi_status_t
* Initialization status.
*
* \funcusage
* \snippet snippet/main.c snippet_mtb_display_ek79007ad3_init
*
*******************************************************************************/
cy_en_mipidsi_status_t mtb_display_ek79007ad3_init(GFXSS_MIPIDSI_Type* mipi_dsi_base,
                                                   mtb_display_ek79007ad3_pin_config_t* disp_ek79007ad3_pin_config)
{
    uint32_t i;
    cy_en_mipidsi_status_t status = CY_MIPIDSI_BAD_PARAM;

    CY_ASSERT(NULL != mipi_dsi_base);
    CY_ASSERT(NULL != disp_ek79007ad3_pin_config);

    disp_pin_config = disp_ek79007ad3_pin_config;

    /* Display pin initialization sequence */
    /* Initialize display RESET GPIO pin with initial value as HIGH */
    Cy_GPIO_Pin_FastInit(disp_pin_config->reset_port, disp_pin_config->reset_pin,
                         CY_GPIO_DM_STRONG_IN_OFF, GPIO_HIGH, HSIOM_SEL_GPIO);

    /* Perform reset */
    /* Pull the display RESET GPIO pin to LOW */
    Cy_GPIO_Write(disp_pin_config->reset_port, disp_pin_config->reset_pin, GPIO_LOW);
    Cy_SysLib_Delay(ONE_MS_DELAY);

    /* Pull the display RESET GPIO pin to HIGH */
    Cy_GPIO_Write(disp_pin_config->reset_port, disp_pin_config->reset_pin, GPIO_HIGH);
    Cy_SysLib_Delay(PIN_HIGH_DELAY_MS);

    /* Set the LCM init settings */
    for (i = 0; i < CY_ARRAY_SIZE(lcm_init_setting); i++)
    {
        status = Cy_MIPIDSI_WritePacket(mipi_dsi_base, lcm_init_setting[i], PACKET_LENGTH);
        if (CY_MIPIDSI_SUCCESS != status)
        {
            break;
        }
    }

    if (CY_MIPIDSI_SUCCESS == status)
    {
        Cy_SysLib_Delay(ONE_MS_DELAY);

        status = Cy_MIPIDSI_ExitSleep(mipi_dsi_base);

        if (CY_MIPIDSI_SUCCESS == status)
        {
            Cy_SysLib_Delay(ONE_MS_DELAY);
            status = Cy_MIPIDSI_DisplayON(mipi_dsi_base);

            if (CY_MIPIDSI_SUCCESS == status)
            {
                Cy_SysLib_Delay(ONE_MS_DELAY);

                viv_set_commit(DC_CMD_COMMIT_MASK);
            }
        }
    }

    return status;
}


/*******************************************************************************
* Function name: mtb_display_ek79007ad3_backlight_init
********************************************************************************
*
* Configures display backlight GPIO pin and enables PWM output on it.
*
* \param disp_ek79007ad3_backlight_config
* Pointer to the display backlight configuration structure.
*
* \return cy_en_tcpwm_status_t
* Display backlight PWM initialization status.
*
* \funcusage
* \snippet snippet/main.c snippet_mtb_display_ek79007ad3_backlight_init
*
*******************************************************************************/
cy_en_tcpwm_status_t mtb_display_ek79007ad3_backlight_init(
    mtb_display_ek79007ad3_backlight_config_t* disp_ek79007ad3_backlight_config)
{
    cy_en_tcpwm_status_t status = CY_TCPWM_BAD_PARAM;

    CY_ASSERT(NULL != disp_ek79007ad3_backlight_config);

    backlight_config = disp_ek79007ad3_backlight_config;

    /* Display backlight GPIO pin initialization in PWM mode */
    Cy_GPIO_Pin_FastInit(backlight_config->bl_port, backlight_config->bl_pin,
                         CY_GPIO_DM_STRONG_IN_OFF, GPIO_LOW, HSIOM_SEL_ACT_2);

    /* Initialize the TCPWM block for backlight pin */
    status = Cy_TCPWM_PWM_Init(backlight_config->pwm_hw, backlight_config->pwm_num,
                               backlight_config->pwm_config);

    if (CY_TCPWM_SUCCESS == status)
    {
        /* Enable the TCPWM block for backlight pin */
        Cy_TCPWM_PWM_Enable(backlight_config->pwm_hw, backlight_config->pwm_num);

        /* Fetch the initial values of period register */
        pwm_period =
            Cy_TCPWM_PWM_GetPeriod0(backlight_config->pwm_hw, backlight_config->pwm_num);

        /* Trigger a software start on the selected TCPWM */
        Cy_TCPWM_TriggerStart_Single(backlight_config->pwm_hw, backlight_config->pwm_num);
    }

    return status;
}


/*******************************************************************************
* Function name: mtb_display_ek79007ad3_set_brightness
********************************************************************************
*
* Sets the brightness of the display panel to the desired percentage.
*
* \param brightness_percent
* Brightness value in percentage (0 to 100).
*
* \return void
*
*******************************************************************************/
void mtb_display_ek79007ad3_set_brightness(uint8_t brightness_percent)
{
    uint32_t compare0_value = 0;
    uint32_t compare1_value = 0;
    uint32_t counter_val    = 0;

    CY_ASSERT(MAX_BRIGHTNESS_PERCENT >= brightness_percent);

    counter_val = BRIGHTNESS_PERCENT_TO_PWM_COUNT(brightness_percent, pwm_period);
    compare0_value = compare1_value = (counter_val > pwm_period) ?
                                      pwm_period : counter_val;

    /* Set new values for CC0/1 compare buffers */
    Cy_TCPWM_PWM_SetCompare0BufVal(backlight_config->pwm_hw, backlight_config->pwm_num,
                                   compare0_value);
    Cy_TCPWM_PWM_SetCompare1BufVal(backlight_config->pwm_hw, backlight_config->pwm_num,
                                   compare1_value);

    /* Trigger compare swap with its buffer values */
    Cy_TCPWM_TriggerCaptureOrSwap_Single(backlight_config->pwm_hw, backlight_config->pwm_num);
}


/*******************************************************************************
* Function name: mtb_display_ek79007ad3_deinit
********************************************************************************
*
* Performs de-initialization of the EK79007AD3 TFT driver using MIPI DSI interface.
* It also de-initializes PWM output on display backlight pin.
*
* \param mipi_dsi_base
* Pointer to the MIPI DSI register base address.
*
* \return cy_en_mipidsi_status_t
* De-initialization status.
*
*******************************************************************************/
cy_en_mipidsi_status_t mtb_display_ek79007ad3_deinit(GFXSS_MIPIDSI_Type* mipi_dsi_base)
{
    cy_en_mipidsi_status_t status = CY_MIPIDSI_BAD_PARAM;

    CY_ASSERT(NULL != mipi_dsi_base);
    CY_ASSERT(NULL != disp_pin_config);
    CY_ASSERT(NULL != backlight_config);

    status = Cy_MIPIDSI_EnterSleep(mipi_dsi_base);
    if (CY_MIPIDSI_SUCCESS == status)
    {
        /* Set display RESET GPIO pin to LOW */
        Cy_GPIO_Write(disp_pin_config->reset_port, disp_pin_config->reset_pin, GPIO_LOW);

        /* Stop and de-initialize PWM on display backlight pin */
        Cy_TCPWM_TriggerStopOrKill_Single(backlight_config->pwm_hw, backlight_config->pwm_num);
        Cy_TCPWM_PWM_Disable(backlight_config->pwm_hw, backlight_config->pwm_num);
        Cy_TCPWM_PWM_DeInit(backlight_config->pwm_hw, backlight_config->pwm_num,
                            backlight_config->pwm_config);
    }

    return status;
}


/* [] END OF FILE */
