# Waveshare 7-inch Raspberry Pi DSI C display driver library API reference guide

## General description

This library provides functions for supporting Waveshare 7-inch DSI LCD (C) display panel driver.

## Code snippets

### Snippet 1: Display driver initialization

The following snippet initializes the display panel driver. This API expects an initialized I2C instance.

```cpp
#include "cybsp.h"

#include "mtb_disp_ws7p0dsi_drv.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#define I2C_CONTROLLER_IRQ_PRIORITY             (2UL)

#define BRIGHTNESS_TEST_VALUE                   (245U)


/*******************************************************************************
* Global variables
*******************************************************************************/
/* I2C master context. */
cy_stc_scb_i2c_context_t i2c_controller_context;

/* I2C IRQ configuration. */
cy_stc_sysint_t i2c_irq_cfg =
{
    .intrSrc      = CYBSP_I2C_CONTROLLER_IRQ,
    .intrPriority = I2C_CONTROLLER_IRQ_PRIORITY,
};

/* GFX instance, which is to be used for the display is expected to be initialized
* using the Device Configurator tool of ModusToolbox&trade; software. In this example code,
* a dummy instance named GFXSS will be used.
*/
cy_stc_gfx_context_t gfx_context;


/*******************************************************************************
* Function name: i2c_interrupt_handler
****************************************************************************//**
*
* Invokes the Cy_SCB_I2C_Interrupt() PDL driver function.
*
*******************************************************************************/
void i2c_interrupt_handler(void)
{
    Cy_SCB_I2C_Interrupt(CYBSP_I2C_CONTROLLER_HW, &i2c_controller_context);
}


/*******************************************************************************
* Code
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    cy_en_scb_i2c_status_t i2c_result = CY_SCB_I2C_SUCCESS;
    cy_en_sysint_status_t sys_status = CY_SYSINT_SUCCESS;
    cy_en_gfx_status_t gfx_status = CY_GFX_SUCCESS;

    /* Initializes the device and board peripherals. */
    result = cybsp_init();
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }
    /* Enables global interrupts. */
    __enable_irq();

    /* GFXSS init. */
    /* MIPI-DSI display-specific configurations. */
    GFXSS_config.mipi_dsi_cfg = &mtb_disp_ws7p0dsi_dsi_config;
    /* Initializes the graphics system. */
    gfx_status = Cy_GFXSS_Init(GFXSS, &GFXSS_config, &gfx_context);

    if (CY_GFX_SUCCESS == gfx_status)
    {
        Cy_GFXSS_Enable_GPU_Interrupt(base);

        /* Initializes the I2C in master mode. */
        i2c_result = Cy_SCB_I2C_Init(CYBSP_I2C_CONTROLLER_HW,
                                     &CYBSP_I2C_CONTROLLER_config, &i2c_controller_context);

        if (CY_SCB_I2C_SUCCESS != i2c_result)
        {
            CY_ASSERT(0);
        }

        /* Initializes the I2C interrupt. */
        sys_status = Cy_SysInt_Init(&i2c_irq_cfg,
                                    &i2c_interrupt_handler);

        if (CY_SYSINT_SUCCESS != sys_status)
        {
            CY_ASSERT(0);
        }

        NVIC_EnableIRQ(i2c_irq_cfg.intrSrc);

        /* Enables the I2C. */
        Cy_SCB_I2C_Enable(CYBSP_I2C_CONTROLLER_HW);

        /* Initializes the display. */
        result = mtb_disp_ws7p0dsi_panel_init(CYBSP_I2C_CONTROLLER_HW,
                                              &i2c_controller_context);

        if (CY_RSLT_SUCCESS != result)
        {
            CY_ASSERT(0);
        }

        /* Displays graphics frame as per application use case, an example shown in
         * below comment, assume img_ptr holds the image frame.
         */
        /*
         *   Cy_GFXSS_Set_FrameBuffer((GFXSS_Type*) GFXSS, (uint32_t*) img_ptr,
         *                            &gfx_context);
         */
        /* Brightness control of the display panel. */
        result = mtb_disp_ws7p0dsi_panel_brightness_ctrl(CYBSP_I2C_CONTROLLER_HW,
                                                         &i2c_controller_context,
                                                         BRIGHTNESS_TEST_VALUE);

        if (CY_RSLT_SUCCESS != result)
        {
            CY_ASSERT(0);
        }
        /* De-initializes the display. */
        result = mtb_disp_ws7p0dsi_panel_deinit(CYBSP_I2C_CONTROLLER_HW,
                                                &i2c_controller_context);

        if (CY_RSLT_SUCCESS != result)
        {
            CY_ASSERT(0);
        }
    }

    for (;;)
    {
    }
}
```

---
© 2025, Cypress Semiconductor Corporation (an Infineon company)
