#include "project.h"
#include <stdio.h>
char buff[128];
uint8 inbuff[512];

// These commands come the GTT 2.0 and GTT2.5 Protocol Manuals
uint8 clearCMD[] = { 0x58 };
uint8 resetCMD[] = { 0x01};
uint8 comI2CCMD[] = { 0x05, 0x02};
uint8 comNONECMD[] = { 0x05, 0x00};

#define I2CADDR (0x28)
#define I2CTIMEOUT (0xFF)

typedef enum {
    MODE_IDLE,
    MODE_PACKET,
    MODE_STREAMING
} i2cmode_t;

i2cmode_t i2cpoling=MODE_IDLE;


#define processPacketError(code,msg)  if(code != I2C_I2C_MSTR_NO_ERROR) \
    { \
        errorMsg = msg; \
        goto errorexit; \
    } \

// writePacket 
// This function sends a start packet, number of bytes, and then the actual data
void writePacket(uint32 len, uint8 *data)
{
    char *errorMsg;
    uint32_t returnCode;
    returnCode = I2C_I2CMasterSendStart(I2CADDR,I2C_I2C_WRITE_XFER_MODE , I2CTIMEOUT);
    processPacketError(returnCode,"Send Start");
    
    returnCode = I2C_I2CMasterWriteByte(0xFE,I2CTIMEOUT); // 0xFE=254 = Packet Start code
    processPacketError(returnCode,"Write Byte");
    
    for(uint32 i=0;i<len;i++)
    {
        returnCode = I2C_I2CMasterWriteByte(data[i],I2CTIMEOUT);
        processPacketError(returnCode,"Write Byte");
    }
    
    returnCode = I2C_I2CMasterSendStop(I2CTIMEOUT);
    processPacketError(returnCode,"Send Stop");    
    return;
    
    errorexit:
    i2cpoling = MODE_IDLE;
    sprintf(buff,"Write Packet: %s Error=%X\r\n",errorMsg,(unsigned int)returnCode);
    UART_UartPutString(buff);
    return;
    
}

void readPacket()
{
    int length;
    int command;
    uint8_t data;
    uint32_t returncode;
    
    returncode = I2C_I2CMasterSendStart(I2CADDR,I2C_I2C_READ_XFER_MODE , I2CTIMEOUT);
    returncode |= I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&data,I2CTIMEOUT);
    returncode |= I2C_I2CMasterSendStop(I2CTIMEOUT);
    
    // Something bad happened on the I2C Bus ....
    if(returncode)
    {
        i2cpoling = MODE_IDLE; 
        sprintf(buff,"I2C Return Code %X\r\n",(unsigned int)returncode);
        UART_UartPutString(buff);
    }
 
    // The screen returns a 0 when there is nothing in the buffer.
    if(data == 0)
    {
        return;
    }

    // This is bad because there was something other than a packet start byte
    if(data != 252)
    {
        sprintf(buff,"bad data = %d\r\n",data);
        UART_UartPutString(buff);
        i2cpoling = MODE_IDLE; // put it into nothing mode...
        return;
    }
    
    // We know that we have a command
    returncode = I2C_I2CMasterSendStart(I2CADDR,I2C_I2C_READ_XFER_MODE , I2CTIMEOUT);
    returncode |= I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA,&data,I2CTIMEOUT); // command
    command = data;
    
    returncode |= I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA,&data,I2CTIMEOUT); // length
    length = data<<8;
    returncode |= I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&data,I2CTIMEOUT); // length
    length = length + data;
    returncode |= I2C_I2CMasterSendStop(I2CTIMEOUT);
    
    // If the packet has any data... then read it.
    if(length != 0)
    {
        returncode |= I2C_I2CMasterSendStart(I2CADDR,I2C_I2C_READ_XFER_MODE , I2CTIMEOUT);
    
        for(int i=0;i<length-1; i++)
        {
            I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA,&data,I2CTIMEOUT); // length
            inbuff[i] = data;
        }

        // Read the last byte
        I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&data,I2CTIMEOUT); // length
        inbuff[length-1] = data;
        returncode |= I2C_I2CMasterSendStop(I2CTIMEOUT);
        
        I2C_I2CMasterSendStop(I2CTIMEOUT);
    }
      
    sprintf(buff,"command = %d length = %d bytes= ",command,length);
    UART_UartPutString(buff);
    for(int i=0;i<length;i++)
    {
        sprintf(buff,"%d ",inbuff[i]);
        UART_UartPutString(buff);
    }
    UART_UartPutString("\r\n");
    
}


#define I2CPORTSEL CYREG_HSIOM_PORT_SEL3

void i2cReset()
{
    uint32_t val;

    sprintf(buff,"Reset I2C SCL\r\nMaking digitial i/o SCL Val=%x SCL=%d SDA=%d\r\n",(unsigned int)val,I2C_scl_Read(),I2C_sda_Read());
    UART_UartPutString(buff);

    // This makes the portselect mux be set to 0 aka software digitial control
    // We know that the lower 4 bits of this register are the P4[0]
    *((uint32 *)I2CPORTSEL) &= ~0x0F;

    // write 9 0's onto scl
    for(int i=0;i<9;i++)
    {
        I2C_scl_Write(0);
        CyDelay(1);
        I2C_scl_Write(1);
        CyDelay(1);
    }
    
    // make the pin back to i2C
    val = *((uint32 *)I2CPORTSEL);
    sprintf(buff,"Making scb i2c i/o Current Val=%x\r\n",(unsigned int)val);
    UART_UartPutString(buff);
    
    // Put it back to control by the SCB
    *((uint32 *)I2CPORTSEL) =  (*((uint32 *)I2CPORTSEL) & ~0x0F) | 0x0E;
    val = *((uint32 *)I2CPORTSEL);
    sprintf(buff,"Current Port Selection Val=%x\r\n",(unsigned int)val);
    UART_UartPutString(buff);
    sprintf(buff,"Current SCL=%d SDA=%d\r\n",I2C_scl_Read(),I2C_sda_Read());
    UART_UartPutString(buff);
}


int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    UART_Start();
    UART_UartPutString("Started\r\n");
    I2C_Start();
    
    char c;
    uint32 returncode;
    uint8 data;
        
    for(;;)
    {
        cnt:
        c = UART_UartGetChar();
        switch(c)
        {
            case 0:
            break;
            
            case 'c':
                UART_UartPutString("Sent Clear String\r\n");
                writePacket(sizeof(clearCMD),clearCMD);
            break;
            case 'R':
                UART_UartPutString("Sent Reset String\r\n");
                writePacket(sizeof(resetCMD),resetCMD);
            break;
            case 'I':
                UART_UartPutString("I2C Communcation Channel\r\n");
                writePacket(sizeof(comI2CCMD),comI2CCMD);
            break;
            case 'N':
                UART_UartPutString("NONE Communcation Channel\r\n");
                writePacket(sizeof(comNONECMD),comNONECMD);
            break;

            // If you are IDLE you can read 1 byte with 'r' or read a while packet with 'p'
            case 'r':  // Read byte
                if(i2cpoling != MODE_IDLE)
                    break;
                returncode = I2C_I2CMasterSendStart(I2CADDR,I2C_I2C_READ_XFER_MODE , I2CTIMEOUT);
                returncode |= I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&data,I2CTIMEOUT);
                returncode |= I2C_I2CMasterSendStop(I2CTIMEOUT);
                sprintf(buff,"Returncode = %X Data=%d\r\n",(unsigned int)returncode,data);
                UART_UartPutString(buff);
            break;
            case 'p': // read packet
                if(i2cpoling == MODE_IDLE)
                    readPacket();
                break;
                    
                
            // I2C Modes
            case '0':
                    Pin_1_Write(0);
                    UART_UartPutString("Packet Poling Off\r\n");
                    i2cpoling = MODE_IDLE;
                break;
               
            case '1':
                    Pin_1_Write(0);
                    UART_UartPutString("Packet Poling On\r\n");
                    i2cpoling = MODE_PACKET;
            break;
            case '2':
                    Pin_1_Write(0);
                    UART_UartPutString("Read continuous\r\n");
                    i2cpoling = MODE_STREAMING;
                    break;

      
          // Debug I2C
            case 's':  // What is the status of the SCB 
                returncode = I2C_I2CMasterStatus();
                sprintf(buff,"master status = %X\r\n",(unsigned int)returncode);
                UART_UartPutString(buff);
                break;

            case 'z': // Send the reset sequence
                I2C_Stop();
                i2cReset();
                I2C_Start();
            break;
    
            case 'x': // What are the digital reads of the bus
                sprintf(buff,"SDA=%d SCL=%d\r\n",I2C_sda_Read(),I2C_scl_Read());
                UART_UartPutString(buff);
            break;
            
            case '?':
                UART_UartPutString("N\tDefault Comm None\r\n");
                UART_UartPutString("I\tDefault Comm I2C\r\n");
                UART_UartPutString("R\tReset\r\n");
                UART_UartPutString("c\tSend Clear Screen\r\n");
                
                UART_UartPutString("r\tRead one byte if I2C polling is off\r\n");
                UART_UartPutString("p\tRead Packet if I2C polling is off\r\n");
                
                
                UART_UartPutString("0\tTurn I2C polling off \r\n");
                UART_UartPutString("1\tTurn on I2C packet polling\r\n");
                UART_UartPutString("2\tRead i2c bytes \r\n");
                
                UART_UartPutString("s\tPrint SCB Status\r\n");
                UART_UartPutString("z\tSend I2C Reset Sequence\r\n");
                UART_UartPutString("x\tPrint I2C SCL and SDA value\r\n");
            break;
        }
        
        if(i2cpoling == MODE_PACKET)
            readPacket();
        if(i2cpoling == MODE_STREAMING)
        {
            returncode = I2C_I2CMasterSendStart(I2CADDR,I2C_I2C_READ_XFER_MODE , I2CTIMEOUT);
            if(returncode)
            {
                sprintf(buff,"send start error %lX status %lX\r\n",returncode,I2C_I2CMasterStatus());
                UART_UartPutString(buff);
                Pin_1_Write(1);
                i2cpoling = MODE_IDLE;
                goto cnt;
            }
            
            returncode = I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA,&data,I2CTIMEOUT);
            if(returncode)
            {
                sprintf(buff,"read byte error %lX status %lX sda=%d scl =%d\r\n",returncode,I2C_I2CMasterStatus(),I2C_sda_Read(),I2C_scl_Read());
                UART_UartPutString(buff);
                Pin_1_Write(1);
                i2cpoling = MODE_IDLE;
                goto cnt;
            }
            
            returncode = I2C_I2CMasterSendStop(I2CTIMEOUT);
            if(returncode)
            {
                sprintf(buff,"send stop error %lX status %lX\r\n",returncode,I2C_I2CMasterStatus());
                UART_UartPutString(buff);
                Pin_1_Write(1);
                i2cpoling = MODE_IDLE;
                goto cnt;
            }
            sprintf(buff,"Returncode = %X Data=%d\r\n",(unsigned int)returncode,data);
            UART_UartPutString(buff);
        }
        
    }
}
