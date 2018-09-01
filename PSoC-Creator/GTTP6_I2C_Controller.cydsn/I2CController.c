/*  
*   This is some bad-ass software written for a bad-ass redneck car.
*   It doesnt even need to have the motor on to burn rubber.
*
*   This car was built for some tire smoking, beer swilling, asskick shit.
*
*   Back the fuck up!
*
*/

/*****************************************************************************
* Header File Includes
*****************************************************************************/
#include "I2CController.h"
#include <project.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
//#include "SystemConfiguration.h"
#include <stdbool.h>

/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */


/*****************************************************************************
* MACRO Definition
*****************************************************************************/
/* The length of the queue used to send messages to the LCD gatekeeper task. */
#define I2Cm1_QUEUE_SIZE             5

/* Define the timeout in ticks for the transaction to wait until the queue is available */
#define I2Cm1_QUEUE_TICKS_TO_WAIT    10

/*******************************************
 * I2C Low Level Driver Macros
 ********************************************/
/* Timeout */
#define I2C_TIMEOUT              (50UL)

/* Combine master error statuses in single mask  */
#define I2C_MASTER_ERROR_MASK    (CY_SCB_I2C_MASTER_DATA_NAK | CY_SCB_I2C_MASTER_ADDR_NAK |    \
                                  CY_SCB_I2C_MASTER_ARB_LOST | CY_SCB_I2C_MASTER_ABORT_START | \
                                  CY_SCB_I2C_MASTER_BUS_ERR)

/*****************************************************************************
* Function Prototypes
*****************************************************************************/
/*The I2C m1 is written to by more than one task so is controlled by this
 * 'gatekeeper' task.  This is the only task that is actually permitted to
 * access the I2C directly.  Other tasks wanting to send an I2C transaction send
 * the message to the gatekeeper task. */
bool I2Cm1RunTransaction(I2Cm_transaction_t *I2Cm1_transaction); // The public interace

/*****************************************************************************
* Variable Declaration
*****************************************************************************/
static QueueHandle_t xI2Cm1Queue = NULL;    /* This is the main queue for holding I2C transactions for the UI
                                             * It is private and should only be used by the I2C Master
                                             * you can add tasks with the public interface
                                             */

#define i2cHandleError(var,loc) if(CY_SCB_I2C_SUCCESS != *var->errorStatus) goto loc;


void writeBasic(I2Cm_transaction_t *trans)
{
    printf("Write Basic Len=%d ",trans->length);
    for(uint32_t i=0;i<trans->length;i++)
    {
        printf("%02X ",trans->data[i]);
    }
    printf("\r\n");
    /* Begining of an I2C transaction */
    *trans->bytesProcessed = 0;
    /* First write the register address */
    /* Generate start and send address to start transfer */
    /* Sends data to slave using low level PDL library functions. */
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStart(I2Cm1_HW, trans->i2cAddress, CY_SCB_I2C_WRITE_XFER, I2C_TIMEOUT, &I2Cm1_context);
    i2cHandleError(trans,writeBasicError);
   
    // loop 
    for(uint32_t i=0;i < trans->length ; i++)
    {
        *trans->errorStatus = Cy_SCB_I2C_MasterWriteByte(I2Cm1_HW, trans->data[i], I2C_TIMEOUT, &I2Cm1_context);
        *trans->bytesProcessed += 1;
        i2cHandleError(trans,writeBasicError);
    }
 
    /* Send Stop condition on the bus */
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStop(I2Cm1_HW, I2C_TIMEOUT, &I2Cm1_context);
 
writeBasicError:
    return;
}

void writeRegister(I2Cm_transaction_t *trans)
{
    /* Begining of an I2C transaction */
    *trans->bytesProcessed = 0;
    /* First write the register address */
    /* Generate start and send address to start transfer */
    /* Sends data to slave using low level PDL library functions. */
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStart(I2Cm1_HW, trans->i2cAddress, CY_SCB_I2C_WRITE_XFER, I2C_TIMEOUT, &I2Cm1_context);
    /* If transfer is successful, route to next state */
    i2cHandleError(trans,writeRegisterError);
    
    // if register is 16 then send the high byte
    if (trans->regType == I2CM_BIT16)
    {
        /* Write byte and receive ACK/NACK response */
        *trans->errorStatus = Cy_SCB_I2C_MasterWriteByte(I2Cm1_HW, ((trans->slaveRegister >> 8) & 0xFF), I2C_TIMEOUT, &I2Cm1_context);
        i2cHandleError(trans,writeRegisterError);
    }

     // send byte i2cAddress byte2
    *trans->errorStatus = Cy_SCB_I2C_MasterWriteByte(I2Cm1_HW, ((trans->slaveRegister) & 0xFF), I2C_TIMEOUT, &I2Cm1_context);
    i2cHandleError(trans,writeRegisterError);

    // loop 
    for(uint32_t i=0;i < trans->length ; i++)
    {
        *trans->errorStatus = Cy_SCB_I2C_MasterWriteByte(I2Cm1_HW, trans->data[i], I2C_TIMEOUT, &I2Cm1_context);
        *trans->bytesProcessed += 1;
        i2cHandleError(trans,writeRegisterError);
    }
 
    /* Send Stop condition on the bus */
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStop(I2Cm1_HW, I2C_TIMEOUT, &I2Cm1_context);
 
writeRegisterError:
    return;
}

void readBasic(I2Cm_transaction_t *trans)
{
    /* Begining of an I2C transaction */
    *trans->bytesProcessed = 0;
    
    /* First write the register address */
    /* Generate start and send address to start transfer */
    /* Sends data to slave using low level PDL library functions. */
    
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStart(I2Cm1_HW, trans->i2cAddress, CY_SCB_I2C_READ_XFER, I2C_TIMEOUT, &I2Cm1_context);
    i2cHandleError(trans,readBasicError);

    // loop - 1 & ack
    for(uint32_t i=0;i < trans->length - 1 ; i++)
    {
        *trans->errorStatus = Cy_SCB_I2C_MasterReadByte(I2Cm1_HW, CY_SCB_I2C_ACK, &trans->data[i], I2C_TIMEOUT, &I2Cm1_context);
        *trans->bytesProcessed += 1;
        i2cHandleError(trans,readBasicError);
    }
    // read last byte & nak
    *trans->errorStatus = Cy_SCB_I2C_MasterReadByte(I2Cm1_HW, CY_SCB_I2C_NAK, &trans->data[trans->length-1], I2C_TIMEOUT, &I2Cm1_context);
    i2cHandleError(trans,readBasicError);
    *trans->bytesProcessed += 1;
    
 
    /* Send Stop condition on the bus */
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStop(I2Cm1_HW, I2C_TIMEOUT, &I2Cm1_context);
 
readBasicError:
    return;
}

void readRegister(I2Cm_transaction_t *trans)
{
    /* Begining of an I2C transaction */
    *trans->bytesProcessed = 0;
    /* First write the register address */
    /* Generate start and send address to start transfer */
    /* Sends data to slave using low level PDL library functions. */
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStart(I2Cm1_HW, trans->i2cAddress, CY_SCB_I2C_WRITE_XFER, I2C_TIMEOUT, &I2Cm1_context);
    i2cHandleError(trans,readRegisterError);
    
    // if register is 16 then send the high byte
    if (trans->regType == I2CM_BIT16)
    {
        /* Write byte and receive ACK/NACK response */
        *trans->errorStatus = Cy_SCB_I2C_MasterWriteByte(I2Cm1_HW, ((trans->slaveRegister >> 8) & 0xFF), I2C_TIMEOUT, &I2Cm1_context);
        i2cHandleError(trans,readRegisterError);
    }

     // send byte i2cAddress byte2
    *trans->errorStatus = Cy_SCB_I2C_MasterWriteByte(I2Cm1_HW, ((trans->slaveRegister) & 0xFF), I2C_TIMEOUT, &I2Cm1_context);
    i2cHandleError(trans,readRegisterError);

    // send restart with read
    *trans->errorStatus = Cy_SCB_I2C_MasterSendReStart(I2Cm1_HW, trans->i2cAddress, CY_SCB_I2C_READ_XFER, I2C_TIMEOUT, &I2Cm1_context);
    i2cHandleError(trans,readRegisterError);

    // loop - 1 & ack
    for(uint32_t i=0;i < trans->length - 1 ; i++)
    {
        *trans->errorStatus = Cy_SCB_I2C_MasterReadByte(I2Cm1_HW, CY_SCB_I2C_ACK, &trans->data[i], I2C_TIMEOUT, &I2Cm1_context);
        *trans->bytesProcessed += 1;
        i2cHandleError(trans,readRegisterError);
    }
    // read last byte & nak
    *trans->errorStatus = Cy_SCB_I2C_MasterReadByte(I2Cm1_HW, CY_SCB_I2C_NAK, &trans->data[trans->length-1], I2C_TIMEOUT, &I2Cm1_context);
    i2cHandleError(trans,readRegisterError);
    *trans->bytesProcessed += 1;
    
 
    /* Send Stop condition on the bus */
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStop(I2Cm1_HW, I2C_TIMEOUT, &I2Cm1_context);
 
readRegisterError:
    return;
}



void readPacketGtt(I2Cm_transaction_t *trans)
{
    uint8_t data;
    uint32_t dataLength;
    uint32_t cnt=0;
    
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStart (I2Cm1_HW, trans->i2cAddress,CY_SCB_I2C_READ_XFER , I2C_TIMEOUT,&I2Cm1_context);
    i2cHandleError(trans,readGttError);
    *trans->errorStatus = Cy_SCB_I2C_MasterReadByte(I2Cm1_HW,CY_SCB_I2C_NAK,&data,I2C_TIMEOUT,&I2Cm1_context);
    i2cHandleError(trans,readGttError);
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStop(I2Cm1_HW,I2C_TIMEOUT,&I2Cm1_context);
    i2cHandleError(trans,readGttError);
    
     // The screen returns a 0 when there is nothing in the buffer.
    if(data == 0)
    {
        *trans->bytesProcessed = 0;
        return;
    }

    // This is bad because there was something other than a packet start byte
    if(data != 252)
    {
        *trans->bytesProcessed = 0;
        return;
    }
    
    // We know that we have a command
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStart (I2Cm1_HW, trans->i2cAddress,CY_SCB_I2C_READ_XFER , I2C_TIMEOUT,&I2Cm1_context);
    i2cHandleError(trans,readGttError);
    *trans->errorStatus |= Cy_SCB_I2C_MasterReadByte(I2Cm1_HW,CY_SCB_I2C_ACK,&data,I2C_TIMEOUT,&I2Cm1_context); // command
    i2cHandleError(trans,readGttError);
    trans->data[cnt++] = data;
    
    // Read the Length
    *trans->errorStatus = Cy_SCB_I2C_MasterReadByte(I2Cm1_HW,CY_SCB_I2C_ACK,&data,I2C_TIMEOUT,&I2Cm1_context); // length
    i2cHandleError(trans,readGttError);    
    dataLength = data<<8;
    trans->data[cnt++] = data;
    
    *trans->errorStatus = Cy_SCB_I2C_MasterReadByte(I2Cm1_HW,CY_SCB_I2C_NAK,&data,I2C_TIMEOUT,&I2Cm1_context); // length
    i2cHandleError(trans,readGttError);
    dataLength += data;
    trans->data[cnt++] = data;
    *trans->errorStatus = Cy_SCB_I2C_MasterSendStop(I2Cm1_HW,I2C_TIMEOUT,&I2Cm1_context);
    i2cHandleError(trans,readGttError);
  
    // EHK: If it isnt going to fit in the buffer this is a problem
    CY_ASSERT(dataLength+3 < trans->length);
    
    // If the packet has any data... then read it.
    if(dataLength != 0)
    {
        *trans->errorStatus = Cy_SCB_I2C_MasterSendStart (I2Cm1_HW, trans->i2cAddress,CY_SCB_I2C_READ_XFER , I2C_TIMEOUT,&I2Cm1_context);
        i2cHandleError(trans,readGttError);
        for(uint32_t i=0;i < dataLength-1; i++)
        {
            *trans->errorStatus = Cy_SCB_I2C_MasterReadByte(I2Cm1_HW,CY_SCB_I2C_ACK,&data,I2C_TIMEOUT,&I2Cm1_context); // length
            i2cHandleError(trans,readGttError);
            trans->data[cnt++] = data;
        }

        // Read the last byte
        *trans->errorStatus = Cy_SCB_I2C_MasterReadByte(I2Cm1_HW,CY_SCB_I2C_NAK,&data,I2C_TIMEOUT,&I2Cm1_context); // length
        i2cHandleError(trans,readGttError);
        trans->data[cnt++] = data;
        *trans->errorStatus = Cy_SCB_I2C_MasterSendStop(I2Cm1_HW,I2C_TIMEOUT,&I2Cm1_context); 
        i2cHandleError(trans,readGttError);
    }
    
    *trans->bytesProcessed = cnt;
    
    printf("Read packet len=%d\r\n",cnt);
readGttError:
    
    return ;
}

/*******************************************************************************
* Function Name: TaskHandler_I2Cm1
****************************************************************************//**
*
* \brief This is a FreeRTOS task handler for the I2Cm1 task
*
* \param void * param
* pointer to a parameter structure (not used)
*
* \return
* none
*
*******************************************************************************/
portTASK_FUNCTION(TaskHandler_I2Cm1, param)
{
    (void) param;
 
    vTaskDelay(200);
    /* Initialize and Start the I2Cm1 Block */
    I2Cm1_Start();
    printf("Started I2C Master\r\n");
    
    I2Cm_transaction_t I2Cm1_transaction;  /* Declare the transaction structure */

    /* Create the queue used by all tasks.  Messages for communicating via the
       I2Cm1 peripheral are received via this queue. */
    xI2Cm1Queue = xQueueCreate(I2Cm1_QUEUE_SIZE, sizeof(I2Cm_transaction_t));
    #if ( configUSE_TRACE_FACILITY == 1 )
    vTraceSetQueueName(xI2Cm1Queue, "Queue: I2C Transaction");   
    #endif
    
    
    while (1)
    {
        /* Wait until a transaction structure gets put in the queue */
        if (xQueueReceive(xI2Cm1Queue, &I2Cm1_transaction, portMAX_DELAY) == pdTRUE)
        {
            switch(I2Cm1_transaction.method)
            {
                case I2CM_READ:
                    if(I2Cm1_transaction.regType == I2CM_NONE)
                        readBasic(&I2Cm1_transaction);
                    else
                        readRegister(&I2Cm1_transaction);
                break;
                        
                case I2CM_WRITE:
                    if(I2Cm1_transaction.regType == I2CM_NONE)
                        writeBasic(&I2Cm1_transaction);
                    else
                        writeRegister(&I2Cm1_transaction);
  
                break;
                case I2CM_GTT:
                    readPacketGtt(&I2Cm1_transaction);
                break;
            }
            
            /* If transaction asked for a semphore when done, give it to release the I2C */
            if (I2Cm1_transaction.doneSemaphore)
                xSemaphoreGive(I2Cm1_transaction.doneSemaphore);
        }
    }
}



/* The I2Cm1RunTransaction function is the public interace to the system
 * This function is thread safe (it can be called from any thread)
 * It will queue up an I2C tranaction and then wait (if there is a semaphore handle)
 */
bool I2Cm1RunTransaction(I2Cm_transaction_t *I2Cm1_transaction)
{
    if (xQueueSend(xI2Cm1Queue, I2Cm1_transaction, (TickType_t) I2Cm1_QUEUE_TICKS_TO_WAIT) != pdTRUE)
    {
        return(false);
    }
    /* Take the semaphore to release the I2C block for the next use */
    if (I2Cm1_transaction->doneSemaphore)
    {
        xSemaphoreTake(I2Cm1_transaction->doneSemaphore, portMAX_DELAY);
    }
    return(true);
}

/* I2C Helper functions */
uint32_t I2CmlWriteTransaction(uint8_t i2cAddress,uint8_t *data,uint32_t length)
{   
    I2Cm_transaction_t i2cTransaction;
    uint32_t bytesWritten;
    
    cy_en_scb_i2c_status_t errorStatus;
    
    memset(&i2cTransaction, 0, sizeof(i2cTransaction));
    i2cTransaction.i2cAddress        = i2cAddress;
    i2cTransaction.bytesProcessed = &bytesWritten;
    i2cTransaction.doneSemaphore  = xSemaphoreCreateBinary();
    i2cTransaction.errorStatus = &errorStatus;
    CY_ASSERT(i2cTransaction.doneSemaphore != 0);
    
    i2cTransaction.regType        = I2CM_NONE;
    i2cTransaction.method         = I2CM_WRITE;
    i2cTransaction.data            = data;
    i2cTransaction.length        = length;
    I2Cm1RunTransaction(&i2cTransaction);
    vSemaphoreDelete( i2cTransaction.doneSemaphore);
    return errorStatus;
}

uint32_t I2CmlReadTransaction(uint8_t i2cAddress,uint8_t *data, uint32_t length, uint32_t *actualLength)
{
    I2Cm_transaction_t I2CTransaction;
    cy_en_scb_i2c_status_t errorStatus;
    
    I2CTransaction.i2cAddress        = i2cAddress;
    I2CTransaction.bytesProcessed = actualLength;
    I2CTransaction.doneSemaphore  = xSemaphoreCreateBinary();
    CY_ASSERT(I2CTransaction.doneSemaphore != 0);
    I2CTransaction.errorStatus = &errorStatus;
    
    I2CTransaction.data          = data;
    I2CTransaction.regType        = I2CM_NONE;
    I2CTransaction.method         = I2CM_READ;
    I2CTransaction.length        = length;
    I2Cm1RunTransaction(&I2CTransaction);
    vSemaphoreDelete( I2CTransaction.doneSemaphore);
    return errorStatus;
}

// length = maximum buffer length
// data is a pointer to where to store the uint8s that are read
// actualLength is the actual number of bytes read... this had better be length
uint32_t I2CmlReadGttTransaction(uint8_t i2cAddress, uint8_t *data,uint32_t length, uint32_t *actualLength)
{
    
    I2Cm_transaction_t I2CTransaction;
    
    cy_en_scb_i2c_status_t errorStatus;
    
    I2CTransaction.errorStatus = &errorStatus;
    
    I2CTransaction.i2cAddress        = i2cAddress;
    I2CTransaction.bytesProcessed = actualLength;
    I2CTransaction.doneSemaphore  = xSemaphoreCreateBinary();
    CY_ASSERT(I2CTransaction.doneSemaphore != 0);
    
    I2CTransaction.data          = data;
    I2CTransaction.regType        = I2CM_NONE;
    I2CTransaction.method         = I2CM_GTT;
    I2CTransaction.length        = length;
    I2Cm1RunTransaction(&I2CTransaction);
    vSemaphoreDelete( I2CTransaction.doneSemaphore);
 
    return errorStatus;
}
