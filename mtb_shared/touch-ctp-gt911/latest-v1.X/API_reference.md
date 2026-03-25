# GT911 touch panel driver library API reference guide

## General description

This library provides basic set of APIs for supporting touch panel of Waveshare 7-inch DSI LCD (C) display GT911 touch panel driver.

## Code snippets

### Snippet 1: Touch driver initialization and reading touch data

The following snippet initializes the touch panel driver and reads touch data from it. This API expects an initialized I2C instance.

```cpp
#include "cybsp.h"
#include "mtb_ctp_gt911.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#define I2C_MASTER_IRQ_PRIORITY             (2UL)


/*******************************************************************************
* Global variables
*******************************************************************************/
/* I2C master context. */
cy_stc_scb_i2c_context_t i2c_controller_context;

/* I2C IRQ configuration. */
cy_stc_sysint_t i2c_irq_cfg =
{
    .intrSrc = CYBSP_I2C_CONTROLLER_IRQ,
    .intrPriority = I2C_MASTER_IRQ_PRIORITY,
};

/*******************************************************************************
* Code
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    cy_en_scb_i2c_status_t i2c_result = CY_SCB_I2C_SUCCESS;
    cy_en_sysint_status_t sys_status = CY_SYSINT_SUCCESS;
    
    int x = 0;
    int y = 0;

    /* Initializes the device and board peripherals. */
    cybsp_init();

    /* Enables global interrupts. */
    __enable_irq();

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

    /* Initializes GT911 touch driver. */
    result = mtb_gt911_init(CYBSP_I2C_CONTROLLER_HW, &i2c_controller_context);

    if (CY_RSLT_SUCCESS == result)
    {
        /* Read touch coordinates. */
        result = mtb_gt911_get_single_touch(CYBSP_I2C_CONTROLLER_HW,
                                                &i2c_controller_context, &x, &y);

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
