/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
 */

#ifndef I2C_MASTER_H
#define I2C_MASTER_H

/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "semphr.h"   /* Semaphore related API prototypes. */

/* Add any project specific header files can be included here. */
#include <project.h>

/*****************************************************************************
* Data Type Definition
*****************************************************************************/


/*****************************************************************************
* Enumerated Data Definition
*****************************************************************************/
typedef enum I2Cm_transactionMethod
{
    I2CM_READ,
    I2CM_WRITE,
    I2CM_GTT
} I2Cm_transactionMethod_t;

typedef enum I2Cm_registerAddresType
{
    I2CM_NONE,
    I2CM_BIT8,
    I2CM_BIT16
} i2cm_registerAddressType_t;

typedef struct I2Cm_transaction
{
    I2Cm_transactionMethod_t   method;             // I2CM_READ or I2CM_WRITE
    uint8_t                    i2cAddress;         // The I2C Address of the slave
    i2cm_registerAddressType_t regType;            // I2CM_8BIT or I2CM_16BIT
    uint16_t                   slaveRegister;      // The register in the slave
    uint8_t                    *data;              // A pointer to the data to be written (or a place to save the data)
    uint32_t                   *bytesProcessed;    // A return value with the number of bytes written
    uint32_t                   length;             // How many bytes are in the request or max number in buffer
    cy_en_scb_i2c_status_t     *errorStatus;       // Saves the error code
    SemaphoreHandle_t          doneSemaphore;      // If you want a semaphore flagging that the transaction is done
} I2Cm_transaction_t;

/*****************************************************************************
* Data Structure Definition
*****************************************************************************/


/*****************************************************************************
* Global Variable Declaration
*****************************************************************************/


/*****************************************************************************
* Function Prototypes
*****************************************************************************/
/*
 * The I2C m1 is written to by more than one task so is controlled by this
 * 'gatekeeper' task.  This is the only task that is actually permitted to
 * access the I2C directly.  Other tasks wanting to send an I2C transaction send
 * the message to the gatekeeper task.
 */
extern bool I2Cm1RunTransaction(I2Cm_transaction_t *I2Cm1_transaction); // The public interace

uint32_t I2CmlWriteTransaction(uint8_t i2cAddress,uint8_t *data,uint32_t length);
uint32_t I2CmlReadTransaction(uint8_t i2cAddress,uint8_t *data,uint32_t length, uint32_t *actualLength);
uint32_t I2CmlReadGttTransaction(uint8_t i2cAddress, uint8_t *data,uint32_t length, uint32_t *actualLength);
/*****************************************************************************
* External Function Prototypes
*****************************************************************************/

#endif /* I2C_MASTER_H */
/* [] END OF FILE */
