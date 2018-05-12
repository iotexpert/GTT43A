#include "project.h"
#include <stdio.h>
char buff[128];

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    UART_Start();
    UART_UartPutString("Started\r\n");
    I2C_1_Start();
    uint32 val;
    
    for(;;)
    {
        char c = UART_UartGetChar();
        switch(c)
        {
            case 0:
            break;
            
            case '1':
                sprintf(buff,"Writing 1\r\n");
                I2C_1_scl_Write(1);
                UART_UartPutString(buff);
                break;
            case '0':
                sprintf(buff,"Writing 0\r\n");
                UART_UartPutString(buff);
                I2C_1_scl_Write(0);
                break;
                
            case 'd':
                val = *((uint32 *)CYREG_HSIOM_PORT_SEL4);
                sprintf(buff,"Making digitial i/o Val=%lx\r\n",val);
                UART_UartPutString(buff);
                *((uint32 *)CYREG_HSIOM_PORT_SEL4) &= ~0x0F;
            break;
                
            case 'i':
                val = *((uint32 *)CYREG_HSIOM_PORT_SEL4);
                sprintf(buff,"Making scb i2c i/o Val=%lx\r\n",val);
                UART_UartPutString(buff);
                *((uint32 *)CYREG_HSIOM_PORT_SEL4) =  (*((uint32 *)CYREG_HSIOM_PORT_SEL4) & 0x0F) | 0x0E;
            break;
                
            case 's':
                val = *((uint32 *)CYREG_HSIOM_PORT_SEL4); 
                sprintf(buff,"Status val=%lx drive=%lx\r\n",val,*((uint32 *)CYREG_GPIO_PRT4_PC));
                UART_UartPutString(buff);
            break;
            case 'a':
                UART_UartPutString("Alive\r\n");
                break;
            case 't': // tri state
                I2C_1_scl_SetDriveMode(I2C_1_scl_DM_ALG_HIZ);
            break;
        }        
    }
}
