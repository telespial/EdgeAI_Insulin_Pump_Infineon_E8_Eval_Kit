/*******************************************************************************
* \file mtb_ctp_gt911.c
* \version 1.0
*
* \brief
* Provides implementation of the GT911 touch panel driver library.
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
#include <stdbool.h>
#include "mtb_ctp_gt911.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#define GT911_I2C_SLAVE_ADDR            (0x14U)
#define GT911_PRODUCT_ID_REG            (0x8140U)
#define GT911_PRODUCT_ID                (0x00313139U)
#define GT911_PRODUCT_ID_LEN            (4U)
#define GT911_REG_ADDR_LEN              (2U)
#define GT911_REG_CFG_ADDR              (0x8047U)
#define GT911_REG_CFG_DATA_LEN          (186U)
#define GT911_REG_CFG_DATA              (0x8047U)
#define GT911_REG_MAX_X                 (0x8048U)
#define GT911_REG_MAX_Y                 (0x804AU)
#define GT911_TOUCH_STATUS_ADDR         (0x814EU)
#define GT911_TOUCH_STATUS_DATA_LEN     (1U)
#define GT911_VALID_TOUCH_MASK          (0x80U)
#define GT911_TOUCH_1_COORD_ADDR        (0x8150U)
#define GT911_TOUCH_X_LOW_POS           (0U)
#define GT911_TOUCH_X_HIGH_POS          (1U)
#define GT911_TOUCH_Y_LOW_POS           (2U)
#define GT911_TOUCH_Y_HIGH_POS          (3U)
#define GT911_TOUCH_DATA_LEN            (4U)
#define BYTE_SHIFT_AMT                  (8U)
#define GT911_RESET_VAL                 (0U)

#define GT911_START_WAIT_MS             (500U)
#define GT911_CMD_SEND_TIMEOUT_MS       (1000000UL)

#define GT911_MEM_ADDR_BUF_SIZE_BYTES   (2U)
#define GT911_CHECKSUM_REG_OFFSET       (2U)
#define GT911_CFG_VER_LOW_LIMIT         (65U)
#define GT911_CFG_VER_HIGH_LIMIT        (89U)

#define GT911_BYTE_SHIFT_AMT            (8U)
#define GT911_MEM_SIZE_1_BYTE           (1U)
#define GT911_MEM_SIZE_2_BYTE           (2U)
#define GT911_MEM_ADDR_LOW_BYTE_POS     (0U)
#define GT911_MEM_ADDR_HIGH_BYTE_POS    (1U)
#define GT911_DATA_SIZE_1_BYTE          (1U)


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* This is the vendor's firmware configuration for GT911 touch panel controller */
static uint8_t gt911x_fw[] =
{
    0x58U, 0x00U, 0x04U, 0x58U, 0x02U, 0x01U, 0x3DU, 0x00U, 0x22U, 0x08U, 0x1EU,
    0x08U, 0x6EU, 0x50U, 0x03U, 0x03U, 0x00U, 0x00U, 0x00U, 0x00U, 0x11U, 0x11U,
    0x00U, 0x18U, 0x1AU, 0x1EU, 0x14U, 0x8CU, 0x2EU, 0x0EU, 0xB1U, 0xB3U, 0xB8U,
    0x08U, 0x00U, 0x00U, 0x00U, 0xD9U, 0x02U, 0x11U, 0x00U, 0x01U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x96U, 0xC8U, 0x9EU, 0xF5U,
    0xF4U, 0x07U, 0x00U, 0x00U, 0x04U, 0x81U, 0x9AU, 0x00U, 0x7FU, 0xA3U, 0x00U,
    0x7EU, 0xADU, 0x00U, 0x7DU, 0xB7U, 0x00U, 0x7DU, 0xC2U, 0x00U, 0x7DU, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x02U, 0x04U, 0x06U, 0x08U, 0x0AU, 0x0CU, 0x0EU, 0x10U, 0x12U,
    0x14U, 0x16U, 0x18U, 0x1AU, 0x1CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2AU,
    0x29U, 0x28U, 0x26U, 0x24U, 0x22U, 0x21U, 0x20U, 0x1FU, 0x1EU, 0x1DU, 0x1CU,
    0x18U, 0x16U, 0x14U, 0x13U, 0x12U, 0x10U, 0x0FU, 0x0CU, 0x0AU, 0x08U, 0x06U,
    0x04U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x31U, 0x01U
};


/*******************************************************************************
* Function Name: gt911_calc_checksum
********************************************************************************
* This function calculates the checksum of the GT911 firmware configuration data.
*
* \param *buf
* Pointer to data buffer
*
* \param len
* Length of data buffer
*
* \return uint8_t
* Calculated 8-bit checksum value
*
*******************************************************************************/
static uint8_t gt911_calc_checksum(uint8_t* buf, uint8_t len)
{
    uint8_t ccsum = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        ccsum += buf[i];
    }

    ccsum = (~ccsum) + 1U;
    return ccsum;
}


/*******************************************************************************
* Function Name: mtb_gt911_i2c_mem_read
********************************************************************************
* This function reads GT911's memory using I2C.
*
* \param base
* Pointer to I2C base.
*
* \param i2c_context
* Pointer to I2C context
*
* \param mem_addr
* Address of the I2C memory to be read
*
* \param mem_addr_size
* Size of the I2C memory to be read
*
* \param data
* Pointer to the data which is to be transmitted
*
* \param size
* Size of the data which is to be transmitted
*
* \param timeout_ms
* Timeout value in ms for I2C operations
*
* \return cy_en_scb_i2c_status_t
* Result of I2C mem read operations
*
*******************************************************************************/
static cy_en_scb_i2c_status_t mtb_gt911_i2c_mem_read(CySCB_Type* base,
                                                     cy_stc_scb_i2c_context_t* i2c_context,
                                                     uint16_t mem_addr, uint16_t mem_addr_size,
                                                     uint8_t* data, uint16_t size,
                                                     uint32_t timeout_ms)
{
    cy_en_scb_i2c_status_t i2c_result = CY_SCB_I2C_SUCCESS;
    cy_en_scb_i2c_command_t ack = CY_SCB_I2C_ACK;

    uint16_t temp_size = 0;
    uint8_t* temp_data = NULL;
    uint8_t mem_addr_buf[GT911_MEM_ADDR_BUF_SIZE_BYTES];

    if (mem_addr_size == GT911_MEM_SIZE_1_BYTE)
    {
        mem_addr_buf[GT911_MEM_ADDR_LOW_BYTE_POS] = (uint8_t)mem_addr;
    }
    else if (mem_addr_size == GT911_MEM_SIZE_2_BYTE)
    {
        mem_addr_buf[GT911_MEM_ADDR_LOW_BYTE_POS] = (uint8_t)(mem_addr >> GT911_BYTE_SHIFT_AMT);
        mem_addr_buf[GT911_MEM_ADDR_HIGH_BYTE_POS] = (uint8_t)mem_addr;
    }
    else
    {
        return CY_SCB_I2C_BAD_PARAM;
    }

    /* Start I2C communication with GT911 device */
    /* Send I2C memory address to read from. */
    i2c_result = Cy_SCB_I2C_MasterSendStart(base, GT911_I2C_SLAVE_ADDR,
                                            CY_SCB_I2C_WRITE_XFER, timeout_ms,
                                            i2c_context);

    if (i2c_result == CY_SCB_I2C_SUCCESS)
    {
        temp_data = mem_addr_buf;
        temp_size = mem_addr_size;
        while (temp_size)
        {
            i2c_result = Cy_SCB_I2C_MasterWriteByte(base, *temp_data,
                                                    timeout_ms, i2c_context);
            if (i2c_result != CY_SCB_I2C_SUCCESS)
            {
                break;
            }
            --temp_size;
            ++temp_data;
        }
    }
    else
    {
        return i2c_result;
    }

    /* Stop the I2C transmission to GT911 device */
    Cy_SCB_I2C_MasterSendStop(base, timeout_ms, i2c_context);

    /* Read the GT911 device's data into the memory buffer */
    temp_data = data;
    temp_size = size;

    i2c_result = Cy_SCB_I2C_MasterSendStart(base, GT911_I2C_SLAVE_ADDR,
                                            CY_SCB_I2C_READ_XFER, timeout_ms, i2c_context);
    if (i2c_result == CY_SCB_I2C_SUCCESS)
    {
        while (temp_size)
        {
            if (temp_size == GT911_DATA_SIZE_1_BYTE)
            {
                ack = CY_SCB_I2C_NAK;
            }
            i2c_result = Cy_SCB_I2C_MasterReadByte(base, ack, temp_data,
                                                   timeout_ms, i2c_context);
            if (i2c_result != CY_SCB_I2C_SUCCESS)
            {
                break;
            }
            --temp_size;
            ++temp_data;
        }
    }
    else
    {
        return i2c_result;
    }

    /* Stop the I2C transmission to GT911 device */
    Cy_SCB_I2C_MasterSendStop(base, timeout_ms, i2c_context);

    return CY_SCB_I2C_SUCCESS;
}


/*******************************************************************************
* Function Name: mtb_gt911_i2c_mem_write
********************************************************************************
* This function writes to GT911's memory using I2C.
*
* \param base
* Pointer to I2C base
*
* \param i2c_context
* Pointer to I2C context
*
* \param mem_addr
* Address of the I2C memory to be written
*
* \param mem_addr_size
* Size of the I2C memory to be written
*
* \param data
* Pointer to the data which is to be transmitted
*
* \param size
* Size of the data which is to be transmitted
*
* \param timeout_ms
* Timeout value in ms for I2C operations
*
* \return cy_en_scb_i2c_status_t
* Result of I2C mem write operations
*
*******************************************************************************/
static cy_en_scb_i2c_status_t mtb_gt911_i2c_mem_write(CySCB_Type* base,
                                                      cy_stc_scb_i2c_context_t* i2c_context,
                                                      uint16_t mem_addr, uint16_t mem_addr_size,
                                                      uint8_t* data,
                                                      uint16_t size, uint32_t timeout_ms)
{
    cy_en_scb_i2c_status_t i2c_result = CY_SCB_I2C_SUCCESS;
    uint16_t temp_size = 0;
    uint8_t* temp_data = NULL;

    uint8_t mem_addr_buf[GT911_MEM_ADDR_BUF_SIZE_BYTES];
    if (mem_addr_size == GT911_MEM_SIZE_1_BYTE)
    {
        mem_addr_buf[GT911_MEM_ADDR_LOW_BYTE_POS] = (uint8_t)mem_addr;
    }
    else if (mem_addr_size == GT911_MEM_SIZE_2_BYTE)
    {
        mem_addr_buf[GT911_MEM_ADDR_LOW_BYTE_POS] = (uint8_t)(mem_addr >> GT911_BYTE_SHIFT_AMT);
        mem_addr_buf[GT911_MEM_ADDR_HIGH_BYTE_POS] = (uint8_t)mem_addr;
    }
    else
    {
        return CY_SCB_I2C_BAD_PARAM;
    }

    /* Start I2C communication with GT911 device */
    /* Send I2C memory address to write to. */
    i2c_result = Cy_SCB_I2C_MasterSendStart(base, GT911_I2C_SLAVE_ADDR, CY_SCB_I2C_WRITE_XFER,
                                            timeout_ms, i2c_context);

    if (i2c_result == CY_SCB_I2C_SUCCESS)
    {
        temp_data = mem_addr_buf;
        temp_size = mem_addr_size;
        while (temp_size)
        {
            i2c_result = Cy_SCB_I2C_MasterWriteByte(base, *temp_data,
                                                    timeout_ms, i2c_context);
            if (i2c_result != CY_SCB_I2C_SUCCESS)
            {
                break;
            }
            --temp_size;
            ++temp_data;
        }
    }
    else
    {
        return i2c_result;
    }

    /* No stop should be sent here */

    /* Send the memory buffer's data to GT911 device */
    temp_data = data;
    temp_size = size;
    while (temp_size)
    {
        i2c_result = Cy_SCB_I2C_MasterWriteByte(base, *temp_data,
                                                timeout_ms, i2c_context);
        if (i2c_result != CY_SCB_I2C_SUCCESS)
        {
            break;
        }
        --temp_size;
        ++temp_data;
    }

    /* Stop the I2C transmission to GT911 device */
    Cy_SCB_I2C_MasterSendStop(base, timeout_ms, i2c_context);

    return CY_SCB_I2C_SUCCESS;
}


/*******************************************************************************
* Function Name: mtb_gt911_init
********************************************************************************
* This function initializes the touch panel driver GT911.
*
* \param *i2c_base
* Pointer to I2C base
*
* \param *i2c_context
* Pointer to I2C context
*
* \return cy_rslt_t
* Driver initialization operation status
*
*******************************************************************************/
cy_rslt_t mtb_gt911_init(CySCB_Type* i2c_base,
                         cy_stc_scb_i2c_context_t* i2c_context)
{
    cy_en_scb_i2c_status_t i2c_result;
    /* Timeout 1 second (one unit is us) */
    int32_t timeout_ms = GT911_CMD_SEND_TIMEOUT_MS;

    uint32_t prod_id = 0;
    uint8_t read_firmware[GT911_REG_CFG_DATA_LEN] = { 0 };

    /* Wait for the display control system on-board the display to start-up */
    Cy_SysLib_Delay(GT911_START_WAIT_MS);

    i2c_result = mtb_gt911_i2c_mem_read(i2c_base, i2c_context,
                                        GT911_PRODUCT_ID_REG,
                                        GT911_REG_ADDR_LEN, (uint8_t*)(&prod_id),
                                        GT911_PRODUCT_ID_LEN, timeout_ms);

    if (i2c_result != CY_SCB_I2C_SUCCESS)
    {
        GT911_LOG("Fail to read product ID of GT911\n");
        return i2c_result;
    }

    /* Validate the product ID of GT911 */
    if (GT911_PRODUCT_ID != prod_id)
    {
        GT911_LOG("Invalid product ID of GT911, it should be %X, but read ID is"
                  "%lX\n",
                  GT911_PRODUCT_ID, prod_id);
        return CY_RSLT_GT911_DEV_ERR;
    }

    /* Read firmware configuration of GT911 */
    i2c_result = mtb_gt911_i2c_mem_read(i2c_base, i2c_context,
                                        GT911_REG_CFG_ADDR, GT911_REG_ADDR_LEN, read_firmware,
                                        GT911_REG_CFG_DATA_LEN, timeout_ms);

    if (i2c_result != CY_SCB_I2C_SUCCESS)
    {
        GT911_LOG("Fail to read firmware configuration of GT911\n");
        return i2c_result;
    }

    /* verify the firmware configuration of GT911 */
    GT911_LOG("read firmware configuration version = %d, read checksum = %d, calc checksum = %d\n",
              read_firmware[GT911_REG_CFG_DATA - GT911_REG_CFG_ADDR],
              read_firmware[GT911_REG_CFG_DATA_LEN - GT911_CHECKSUM_REG_OFFSET],
              gt911_calc_checksum(read_firmware,
                                  GT911_REG_CFG_DATA_LEN - GT911_CHECKSUM_REG_OFFSET));

    if ((read_firmware[GT911_REG_CFG_DATA - GT911_REG_CFG_ADDR] == 0) ||
        (gt911_calc_checksum(read_firmware, GT911_REG_CFG_DATA_LEN - GT911_CHECKSUM_REG_OFFSET) !=
         read_firmware[GT911_REG_CFG_DATA_LEN - GT911_CHECKSUM_REG_OFFSET]))
    {
        GT911_LOG("Invalid firmware configuration of GT911\n");
        return CY_RSLT_GT911_DEV_ERR;
    }

    /* Prepare for firmware(FW) configuration update of GT911, this FW will include the touch
     * resolution settings for 1024 * 600 pixel touch panel. */

    /* Set the firmware configuration version for update */
    if ((read_firmware[GT911_REG_CFG_DATA - GT911_REG_CFG_ADDR] >= GT911_CFG_VER_LOW_LIMIT) &&
        (read_firmware[GT911_REG_CFG_DATA - GT911_REG_CFG_ADDR] <= GT911_CFG_VER_HIGH_LIMIT))
    {
        gt911x_fw[GT911_REG_CFG_DATA - GT911_REG_CFG_ADDR] =
            read_firmware[GT911_REG_CFG_DATA - GT911_REG_CFG_ADDR] + 1;
    }
    else
    {
        gt911x_fw[GT911_REG_CFG_DATA - GT911_REG_CFG_ADDR] = 0;
    }

    GT911_LOG("gt911x_fw[firmware configuration version]= %d\n",
              gt911x_fw[GT911_REG_CFG_DATA - GT911_REG_CFG_ADDR]);

    /* Set the checksum of the update package */
    gt911x_fw[GT911_REG_CFG_DATA_LEN - GT911_CHECKSUM_REG_OFFSET] =
        gt911_calc_checksum(gt911x_fw, GT911_REG_CFG_DATA_LEN - GT911_CHECKSUM_REG_OFFSET);

    GT911_LOG("gt911x_fw[calculated firmware configuration checksum] = %d\n",
              gt911x_fw[GT911_REG_CFG_DATA_LEN - GT911_CHECKSUM_REG_OFFSET]);

    /* Send the firmware configuration update package */
    i2c_result = mtb_gt911_i2c_mem_write(i2c_base, i2c_context,
                                         GT911_REG_CFG_ADDR, GT911_REG_ADDR_LEN, gt911x_fw,
                                         GT911_REG_CFG_DATA_LEN, timeout_ms);

    if (CY_SCB_I2C_SUCCESS != i2c_result)
    {
        GT911_LOG("firmware configuration update of GT911 failed\n");
        return i2c_result;
    }

    return i2c_result;
}


/*******************************************************************************
* Function Name: mtb_gt911_get_single_touch
********************************************************************************
* Reads single touch data from the GT911 touch panel driver using I2C interface.
*
* \param *i2c_base
* Pointer to I2C base
*
* \param *i2c_context
* Pointer to I2C
*
* \param *touch_x
*  Pointer to the variable of X touch co-ordinate
*
* \param *touch_y
* Pointer to the variable of Y touch co-ordinate
*
* \return cy_rslt_t
* Read operation status
*
*******************************************************************************/
cy_rslt_t mtb_gt911_get_single_touch(CySCB_Type* i2c_base,
                                     cy_stc_scb_i2c_context_t* i2c_context, int* touch_x,
                                     int* touch_y)
{
    uint8_t touch_status = 0;
    uint8_t gt_touch_data[GT911_TOUCH_DATA_LEN];

    cy_en_scb_i2c_status_t i2c_result = CY_SCB_I2C_SUCCESS;

    /* Read touch status register of GT911 */
    i2c_result = mtb_gt911_i2c_mem_read(i2c_base, i2c_context,
                                        GT911_TOUCH_STATUS_ADDR, GT911_REG_ADDR_LEN,
                                        &touch_status,
                                        GT911_TOUCH_STATUS_DATA_LEN, GT911_CMD_SEND_TIMEOUT_MS);

    if (i2c_result != CY_SCB_I2C_SUCCESS)
    {
        GT911_LOG("Read touch i2c_result from GT911 failed\n");
        return i2c_result;
    }

    /* Check whether valid touch data available from GT911*/
    if (touch_status & GT911_VALID_TOUCH_MASK)
    {
        /* Read First Touch (point-1) coordinate data */
        i2c_result = mtb_gt911_i2c_mem_read(i2c_base, i2c_context,
                                            GT911_TOUCH_1_COORD_ADDR,
                                            GT911_REG_ADDR_LEN, gt_touch_data,
                                            GT911_TOUCH_DATA_LEN, GT911_CMD_SEND_TIMEOUT_MS);

        if (CY_SCB_I2C_SUCCESS != i2c_result)
        {
            return i2c_result;
        }
        else
        {
            *touch_x = ((uint16_t)gt_touch_data[GT911_TOUCH_X_LOW_POS]) +
                       (((uint16_t)gt_touch_data[GT911_TOUCH_X_HIGH_POS])
                        << BYTE_SHIFT_AMT);

            *touch_y = ((uint16_t)gt_touch_data[GT911_TOUCH_Y_LOW_POS]) +
                       (((uint16_t)gt_touch_data[GT911_TOUCH_Y_HIGH_POS])
                        << BYTE_SHIFT_AMT);
        }

        /* set GT911 touch status register to zero */
        touch_status = 0;
        i2c_result = mtb_gt911_i2c_mem_write(i2c_base, i2c_context,
                                             GT911_TOUCH_STATUS_ADDR, GT911_REG_ADDR_LEN,
                                             &touch_status,
                                             GT911_TOUCH_STATUS_DATA_LEN,
                                             GT911_CMD_SEND_TIMEOUT_MS);

        if (CY_SCB_I2C_SUCCESS != i2c_result)
        {
            GT911_LOG("setting touch status register to 0 after touch read fail in "
                      "GT911\n");
            return i2c_result;
        }
    }
    else
    {
        i2c_result = (cy_en_scb_i2c_status_t)CY_RSLT_GT911_NO_TOUCH;
    }

    return i2c_result;
}


/* [] END OF FILE */
