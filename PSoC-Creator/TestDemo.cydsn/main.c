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
#include "project.h"

#include <gtt_protocol.h>
#include <gtt_device.h>
#include <stdio.h>

int i2cstate=1;

// Buffer for incoming data
uint8_t rx_buffer[512];

// Buffer for outgoing data
uint8_t tx_buffer[512];

char buff[128];

#define I2CADDR 0x28
#define I2CTIMEOUT 0x100

int generic_write(gtt_device *device, uint8_t *data, size_t length)
{
    (void)device;
    uint32 returncode;
    
   
    sprintf(buff,"length = %d ",length);
    UART_UartPutString(buff);

            
    returncode = I2C_I2CMasterSendStart(I2CADDR,I2C_I2C_WRITE_XFER_MODE , I2CTIMEOUT);
    if(returncode != I2C_I2C_MSTR_NO_ERROR)
    {
        sprintf(buff,"error = %X\r\n",(unsigned int)returncode);
        UART_UartPutString(buff);
    }
    
    for(size_t i=0;i<length;i++)
    {
        I2C_I2CMasterWriteByte(data[i],I2CTIMEOUT);
        sprintf(buff,"%d ",data[i]);
        UART_UartPutString(buff);
    }
    
    I2C_I2CMasterSendStop(I2CTIMEOUT);
    UART_UartPutString("\r\n");
    return length;
        
}

int generic_read(gtt_device *device)
{
    (void)device;
     uint8 data;
     
    uint32 returncode;
    I2C_I2CMasterSendStart(I2CADDR,I2C_I2C_READ_XFER_MODE,I2CTIMEOUT);
    I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&data,I2CTIMEOUT);
    I2C_I2CMasterSendStop(I2CTIMEOUT);
    return data;
}


void my_gtt_event_key(gtt_device* device, uint8_t key, eKeypadRepeatMode type)
{
    UART_UartPutString("event key\r\n");
}
void my_gtt_event_sliderchange(gtt_device* device, eTouchReportingType type, uint8_t slider, int16_t value)
{
    UART_UartPutString("event slider change\r\n");
}
void my_gtt_event_touch(gtt_device* device, eTouchReportingType type, uint16_t x , uint16_t y)
{
    (void)device;
    sprintf(buff,"Event Touch %d x=%d y=%d\r\n",type,x,y); 
    UART_UartPutString(buff);
}

void my_gtt_event_regiontouch(gtt_device* device, eTouchReportingType type, uint8_t region)
{
    (void)device;
    sprintf(buff,"Region Touch %d region=%d\r\n",type,region); 
    UART_UartPutString(buff);
}

void my_gtt_event_baseobject_on_property_change(gtt_device* device, uint16_t ObjectID, uint16_t PropertyID)
{
    UART_UartPutString("event on property change\r\n");
}

void my_gtt_event_visualobject_on_key(gtt_device* device, uint16_t ObjectID, uint8_t Row, uint8_t Col, uint8_t ScanCode, uint8_t Down)
{
    UART_UartPutString("event on key\r\n");
}
void my_gtt_event_button_click(gtt_device* device, uint16_t ObjectID, uint8_t State)
{
    UART_UartPutString("event button click\r\n");
}
/*
gtt_event_key key;
	gtt_event_sliderchange sliderchange;
	gtt_event_touch touch;
	gtt_event_regiontouch regiontouch;
	gtt_event_baseobject_on_property_change baseobject_on_property_change;
	gtt_event_visualobject_on_key visualobject_on_key;
	gtt_event_button_click button_click;
*/

gtt_events myEvents = {
    .sliderchange = my_gtt_event_sliderchange,
    .touch = my_gtt_event_touch,
    .regiontouch = my_gtt_event_regiontouch,
    .baseobject_on_property_change = my_gtt_event_baseobject_on_property_change,
    .visualobject_on_key = my_gtt_event_visualobject_on_key,
    .button_click = my_gtt_event_button_click
};

    
gtt_device gtt_device_instance = {
        .Write = generic_write,
        .Read = generic_read,
        .rx_buffer = rx_buffer,
        .rx_buffer_size = sizeof(rx_buffer),
        .tx_buffer = tx_buffer,
        .tx_buffer_size = sizeof(tx_buffer),
        .events = {
            .sliderchange = my_gtt_event_sliderchange,
            .touch = my_gtt_event_touch,
            .regiontouch = my_gtt_event_regiontouch,
            .baseobject_on_property_change = my_gtt_event_baseobject_on_property_change,
            .visualobject_on_key = my_gtt_event_visualobject_on_key,
            .button_click = my_gtt_event_button_click
        },
        
};

int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */
        I2C_Start();
        UART_Start();
        UART_UartPutString("Started\r\n");
        gtt_device *gtt = &gtt_device_instance;
        
        //Clear the screen
        //gtt_clear_screen(gtt);

        //Draw a line in the default drawing color
        //
        
        int count=50;
        char c;
        gtt_text t = gtt_make_text_ascii("asdf");
        
        //Process any data coming in    
        while (1)
        {
            c = UART_UartGetChar();
            switch(c)
            {
                case 0:
                break;
                case 'l':
                gtt_draw_line(gtt, 0, 0, 480, 272);
                break;
                
                case 'a':
                UART_UartPutString("alive\r\n");
                break;
                case 'c':
                    UART_UartPutString("Clear Screen\r\n");
                    gtt_clear_screen(gtt);
                break;
                    
                case 'R':
                    gtt_reset(gtt);
                break;
                    
                case 'q':
                    
                    UART_UartPutString("Set Text\r\n");
                    gtt25_set_label_text(gtt,2,t);
                    break;
                       
                case '2':
                    gtt25_set_slider_value(gtt,3,2);
                break;
               
                case '9':
                    gtt25_set_slider_value(gtt,3,9);
                break;
                    
                case 'I':
                    //eChannel Channel;
                    gtt_set_default_channel(gtt, eChannel_I2C);
                break;
                    
                case '+':
                    count += 1;
                    if(count>100)
                        count = 100;
                    gtt25_set_gauge_value(gtt,9,count);
                break;    
                
                case '-':
                    if(count > 0)
                        count -= 1;
                    gtt25_set_gauge_value(gtt,9,count);
                break;
                    
                case 's': //print parser state
                    sprintf(buff,"Parser State = %d\r\n",gtt->Parser.state);
                    UART_UartPutString(buff);
                    break;
                
                case 'z':
                    UART_UartPutString("Resetting I2C\r\n");
                    I2C_Stop();
                    I2C_Start();
                    
                    
                    sprintf(buff,"sda =%d scl =%d\r\n",I2C_sda_Read(),I2C_scl_Read());
                    UART_UartPutString(buff);
                    
                    break;
                case '?':
                    UART_UartPutString("s\tPrint Parser State\r\n");
                    UART_UartPutString("l\tDraw a line\r\n");
                    UART_UartPutString("a\tPrint Alive Message\r\n");
                    UART_UartPutString("c\tClear Screen\r\n");
                    UART_UartPutString("R\tReset\r\n");
                    UART_UartPutString("q\tSet text label to asdf\r\n");
                    UART_UartPutString("2\tSet slider to 2\r\n");
                    UART_UartPutString("9\tSet slider to 9\r\n");
                    UART_UartPutString("I\tSet I2C as comm channel\r\n");
            
                    UART_UartPutString("+\tIncrement gauge\r\n");
                    UART_UartPutString("-\tDecrement gauge\r\n");
                break;    
            }
            gtt_parser_process(gtt);
        }

        return 0;
}
