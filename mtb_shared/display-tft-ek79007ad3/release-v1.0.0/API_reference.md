# EK79007AD3 TFT DSI display driver library API reference guide

## General description

A basic set of APIs to interact with the TFT display. This library provides functions for supporting a 10.1 inch video mode TFT LCD driven by an EK79007AD3 controller. 

For more information about the TFT display, see [WF101JTYAHMNB0](https://www.winstar.com.tw/products/tft-lcd/ips-tft/ips-touch.html) display. <br>


## Code snippets

### Snippet 1: Display driver and backlight initialization as part of graphics application 

The following snippet initializes the TFT display controller as well as the backlight and then renders graphics artifacts using the display controller.

```
/* Displays specific MIPI DSI configurations. */
GFXSS_config.mipi_dsi_cfg = &mtb_display_ek79007ad3_mipidsi_config;

/* Sets frame buffer address to the GFXSS configuration structure. */
GFXSS_config.dc_cfg->gfx_layer_config->buffer_address    = frame_buffer;
GFXSS_config.dc_cfg->gfx_layer_config->uv_buffer_address = frame_buffer;

/* Initializes the graphics system according to the configuration. */
gfx_status = Cy_GFXSS_Init(GFXSS, &GFXSS_config, &gfx_context);

if (CY_GFX_SUCCESS == gfx_status)
{
   /* Initializes EK79007AD3 10.1 inch display driver. */
   if (CY_MIPIDSI_SUCCESS !=
      mtb_display_ek79007ad3_init(GFXSS_GFXSS_MIPIDSI, &ek79007ad3_pin_cfg))
   {
      /* Handles possible errors. */
      CY_ASSERT(0);
   }

   /* Initializes 10.1 inch display backlight. */
   if (CY_TCPWM_SUCCESS != 
       mtb_display_ek79007ad3_backlight_init(&ek79007ad3_backlight_cfg))
   {
      /* Handles possible errors. */
      CY_ASSERT(0);
   }

   /* Renders a graphics frame on the display as per application use case. 
    * An example of sending a graphics frame to the display is shown below,
    * assuming img_ptr holds the image frame.
    */

   Cy_GFXSS_Set_FrameBuffer((GFXSS_Type*) GFXSS, (uint32_t*) img_ptr,
                            &gfx_context);
}
```

### Snippet 2: Changing display brightness

The following snippet shows how to use the brightness control API.

```
/* Calls this API from anywhere in the application code to set 
 * display brightness once backlight initialization is complete. 
 */
#define BRIGHTNESS_PERCENT     (70U)
mtb_display_ek79007ad3_set_brightness(BRIGHTNESS_PERCENT);
```

## Macros

```
MTB_DISPLAY_EK79007AD3_DEFAULT_HOR_RES             Displays horizontal resolution in pixels
MTB_DISPLAY_EK79007AD3_DEFAULT_VER_RES             Displays vertical resolution in pixels

MTB_DISPLAY_EK79007AD3_PANEL_CTRL_REG              EK79007AD3 panel control register
MTB_DISPLAY_EK79007AD3_EN_2LANE_BIT_POS            Enables/disables 2 lane MIPI interface register bit position
MTB_DISPLAY_EK79007AD3_EN_2LANE_MASK               Enables/disables 2 lane MIPI interface register bit mask 
MIPI_MAX_PHY_CLK_HZ                                Device max DPHY clock in Hz 
MTB_DISPLAY_EK79007AD3_PANEL_NUM_LANES             EK79007AD3 panel number of lanes
MTB_DISPLAY_EK79007AD3_PANEL_PIXEL_CLK             EK79007AD3 panel pixel clock in kHz
MTB_DISPLAY_EK79007AD3_PANEL_HSYNC_WIDTH           EK79007AD3 panel horizontal sync width
MTB_DISPLAY_EK79007AD3_PANEL_HFP                   EK79007AD3 panel horizontal front porch
MTB_DISPLAY_EK79007AD3_PANEL_HBP                   EK79007AD3 panel horizontal back porch
MTB_DISPLAY_EK79007AD3_PANEL_VSYNC_WIDTH           EK79007AD3 panel vertical sync width
MTB_DISPLAY_EK79007AD3_PANEL_VFP                   EK79007AD3 panel vertical front porch
MTB_DISPLAY_EK79007AD3_PANEL_VBP                   EK79007AD3 panel vertical back porch
MTB_DISPLAY_EK79007AD3_PANEL_PER_LANE_MBPS         EK79007AD3 panel per lane data width in Mbps

DEFAULT_PWM_PERIOD                                 PWM default period value

MTB_DISPLAY_BACKLIGHT_PWM_INPUT_DISABLED           PWM swap input mode bit
MTB_DISPLAY_BACKLIGHT_PWM_PERIOD0                  PWM period 0 value
MTB_DISPLAY_BACKLIGHT_PWM_COMPARE0                 PWM compare 0 value
MTB_DISPLAY_BACKLIGHT_PWM_COMPARE2                 PWM compare 2 value
MTB_DISPLAY_BACKLIGHT_ENABLED_TAPS                 PWM taps enabled value
```

## Data structures
```
struct mtb_display_ek79007ad3_pin_config_t         Structure holding display pin configuration 
struct mtb_display_ek79007ad3_backlight_config_t   Structure holding display backlight pin and PWM configuration 
```

## Global variables
```
extern cy_stc_mipidsi_display_params_t mtb_display_ek79007ad3_mipidsi_display_params  Structure holding display-specific MIPI DSI parameters 
extern cy_stc_mipidsi_config_t mtb_display_ek79007ad3_mipidsi_config                  Structure holding display-specific MIPI DSI configuration
extern cy_stc_tcpwm_pwm_config_t mtb_display_ek79007ad3_backlight_pwm_config          Structure holding display backlight configuration
```

## Functions

cy_en_mipidsi_status_t `mtb_display_ek79007ad3_init(GFXSS_MIPIDSI_Type* mipi_dsi_base, mtb_display_ek79007ad3_pin_config_t* disp_ek79007ad3_pin_config)`
- Performs EK79007AD3 TFT driver initialization using MIPI DSI interface

cy_en_tcpwm_status_t `mtb_display_ek79007ad3_backlight_init(mtb_display_ek79007ad3_backlight_config_t* disp_ek79007ad3_backlight_config)`
- Configures display backlight GPIO pin and enables PWM output on it

void `mtb_display_ek79007ad3_set_brightness(uint8_t brightness_percent)`
- Sets the brightness of the display panel to the desired percent.

cy_en_mipidsi_status_t `mtb_display_ek79007ad3_deinit(GFXSS_MIPIDSI_Type* mipi_dsi_base)`
- De-initializes the EK79007AD3 TFT driver and PWM-based backlight control.

## Macro documentation

Macros      |   Values               |           
|:------------      |  :------------         |  
MTB_DISPLAY_EK79007AD3_DEFAULT_HOR_RES            |  1024                | 
MTB_DISPLAY_EK79007AD3_DEFAULT_VER_RES            |  600                 | 
MTB_DISPLAY_EK79007AD3_PANEL_CTRL_REG            |  0xB2                  | 
MTB_DISPLAY_EK79007AD3_EN_2LANE_BIT_POS            |  4                 | 
MTB_DISPLAY_EK79007AD3_EN_2LANE_MASK            | 16                | 
MIPI_MAX_PHY_CLK_HZ            | 2500000000                | 
MTB_DISPLAY_EK79007AD3_PANEL_NUM_LANES            | 2                | 
MTB_DISPLAY_EK79007AD3_PANEL_PIXEL_CLK            | 41677                | 
MTB_DISPLAY_EK79007AD3_PANEL_HSYNC_WIDTH            | 70                | 
MTB_DISPLAY_EK79007AD3_PANEL_HFP            | 160                | 
MTB_DISPLAY_EK79007AD3_PANEL_HBP            | 160                | 
MTB_DISPLAY_EK79007AD3_PANEL_VSYNC_WIDTH            | 20                | 
MTB_DISPLAY_EK79007AD3_PANEL_VFP            | 12                | 
MTB_DISPLAY_EK79007AD3_PANEL_VBP            | 23                | 
MTB_DISPLAY_EK79007AD3_PANEL_PER_LANE_MBPS            | 500                | 
DEFAULT_PWM_PERIOD            | 32768                | 
MTB_DISPLAY_BACKLIGHT_PWM_INPUT_DISABLED            | 0x7                | 
MTB_DISPLAY_BACKLIGHT_PWM_PERIOD0            | 200                | 
MTB_DISPLAY_BACKLIGHT_PWM_COMPARE0            | 10                | 
MTB_DISPLAY_BACKLIGHT_PWM_COMPARE2            | 10                | 
MTB_DISPLAY_BACKLIGHT_ENABLED_TAPS            | 45                | 

<br>

## Data structure documentation

- mtb_display_ek79007ad3_pin_config_t

   Data type                 |           Member variable      |    Description
   :-------                  |  :------------         |  :------------
   GPIO_PRT_Type*            | reset_port             |  Pointer to the display reset pin's port register base address
   uint32_t                  | reset_pin              |  Display reset pin number

   <br>

- mtb_display_ek79007ad3_backlight_config_t

   Data type                 |           Member variable      |    Description
   :-------                  |  :------------         |  :------------
   GPIO_PRT_Type*            | bl_port             |  Pointer to the display backlight pin's port register base address
   uint32_t                  | bl_pin              |  Display the backlight pin number
   TCPWM_Type*               | pwm_hw              |  Pointer to the TCPWM instance
   uint32_t                  | pwm_num              |  Counter instance number in the selected TCPWM
   cy_stc_tcpwm_pwm_config_t*  | pwm_config              |  Pointer to the PWM configuration structure. See cy_stc_tcpwm_pwm_config_t

   <br>

## Global variable documentation

Data type         |    Variable                |           Description     
:-------         |  :------------           |  :------------
cy_stc_mipidsi_display_params_t          |    mtb_display_ek79007ad3_mipidsi_display_params        | Structure holding display-specific MIPI DSI parameters 
cy_stc_mipidsi_config_t          |    mtb_display_ek79007ad3_mipidsi_config         | Structure holding display-specific MIPI DSI configuration
cy_stc_tcpwm_pwm_config_t |    mtb_display_ek79007ad3_backlight_pwm_config         | Structure holding display backlight configuration

<br>

## Function documentation

#### mtb_display_ek79007ad3_deinit

- cy_en_mipidsi_status_t `mtb_display_ek79007ad3_deinit(GFXSS_MIPIDSI_Type* mipi_dsi_base)`

   **Summary:** De-initializes the EK79007AD3 TFT driver and PWM-based backlight control.

   **Parameter**
   Parameters            |  Description       
   :-------              |  :------------
   [in] mipi_dsi_base    |  Pointer to the MIPI DSI register base address

  <br>

   **Return**
   - cy_en_mipidsi_status_t: De-initialization status

#### mtb_display_ek79007ad3_init

- cy_en_mipidsi_status_t `mtb_display_ek79007ad3_init(GFXSS_MIPIDSI_Type* mipi_dsi_base, mtb_display_ek79007ad3_pin_config_t* disp_ek79007ad3_pin_config)`

   **Summary:** Performs EK79007AD3 TFT driver initialization using MIPI DSI interface.

   **Parameter**
   Parameters            |  Description       
   :-------              |  :------------
   [in] mipi_dsi_base    |  Pointer to the MIPI DSI register base address
   [in] disp_ek79007ad3_pin_config    |  Pointer to the EK79007AD3 display pin configuration structure

   <br>

   **Return**
   - cy_en_mipidsi_status_t: Initialization status

#### mtb_display_ek79007ad3_backlight_init

- cy_en_tcpwm_status_t `mtb_display_ek79007ad3_backlight_init(mtb_display_ek79007ad3_backlight_config_t* disp_ek79007ad3_backlight_config)`

   **Summary:** Configures display backlight GPIO pin and enables PWM output on it.

   **Parameter**
   Parameters            |  Description       
   :-------              |  :------------
   [in] disp_ek79007ad3_backlight_config    |  Pointer to the display backlight configuration structure
   
   <br>

   **Return**
   - cy_en_tcpwm_status_t: Display backlight PWM initialization status

#### mtb_display_ek79007ad3_set_brightness

- void `mtb_display_ek79007ad3_set_brightness(uint8_t brightness_percent)`
   **Summary:** Sets the brightness of the display panel to the desired percent.

   **Parameter**
   Parameters            |  Description       
   :-------              |  :------------
   [in] brightness_percent    |  Brightness value in percentage

   <br>

---
© 2025, Cypress Semiconductor Corporation (an Infineon company)
