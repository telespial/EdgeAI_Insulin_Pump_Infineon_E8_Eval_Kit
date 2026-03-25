/*******************************************************************************
* \file mtb_ctp_gt911.h
*
* \brief
* Provides constants, parameter values, and API prototypes for the GT911 touch
* panel driver library.
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

#ifndef MTB_GT911_H
#define MTB_GT911_H


#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_utils.h"
#include "cy_result.h"
#include "cybsp.h"


/*******************************************************************************
* Macros
*******************************************************************************/
/* Error code base definition. */
#define CY_RSLT_GT911_ERR_BASE          (0x8140U)

/* No touch detected by GT911 */
#define CY_RSLT_GT911_NO_TOUCH                            \
               CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_GT911_ERR_BASE, 1)

/* Error status, for cases like GT911 device ID mismatch or incorrect
 * firmware configuration */
#define CY_RSLT_GT911_DEV_ERR                            \
               CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_GT911_ERR_BASE, 2)

/* Enable logging for GT911 driver library */
#ifdef MTB_GT911_LOG_ENABLE
#define GT911_LOG(...)                      printf(__VA_ARGS__)
#else
#define GT911_LOG(...)
#endif


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
/* Function to initialize the touch panel driver GT911. */
cy_rslt_t mtb_gt911_init(CySCB_Type* i2c_base,
                         cy_stc_scb_i2c_context_t* i2c_context);

/* Reads single touch data from the GT911 touch panel driver over
 * I2C interface. */
cy_rslt_t mtb_gt911_get_single_touch(CySCB_Type* i2c_base,
                                     cy_stc_scb_i2c_context_t* i2c_context, int* touch_x,
                                     int* touch_y);


#if defined(__cplusplus)
}
#endif /* __cplusplus */


#endif /* MTB_GT911_H_ */


/* [] END OF FILE */
