
#include "project.h"

#include <gtt_protocol.h>
#include <gtt_device.h>
#include <stdio.h>

typedef enum {
    MODE_IDLE,
    MODE_POLLING
} systemMode_t;


systemMode_t systemMode=MODE_IDLE;

// Buffer for incoming data
uint8_t rx_buffer[512];

// Buffer for outgoing data
uint8_t tx_buffer[512];

char buff[128];

typedef struct {
    uint32_t slaveAddress;
    uint32_t timeout;
} i2cContext_t;


i2cContext_t i2c1 = {
    .slaveAddress = 0x28,
    .timeout = 0x100,
};


int generic_write(gtt_device *device, uint8_t *data, size_t length)
{
    (void)device;
    uint32 returncode;
    
   
    sprintf(buff,"length = %d ",length);
    UART_UartPutString(buff);

            
    returncode = I2C_I2CMasterSendStart( ((i2cContext_t *)device->Context)->slaveAddress,I2C_I2C_WRITE_XFER_MODE , ((i2cContext_t *)device->Context)->timeout);
    if(returncode != I2C_I2C_MSTR_NO_ERROR)
    {
        sprintf(buff,"error = %X\r\n",(unsigned int)returncode);
        UART_UartPutString(buff);
    }
    
    for(size_t i=0;i<length;i++)
    {
        I2C_I2CMasterWriteByte(data[i],((i2cContext_t *)device->Context)->timeout);
        sprintf(buff,"%d ",data[i]);
        UART_UartPutString(buff);
    }
    
    I2C_I2CMasterSendStop(((i2cContext_t *)device->Context)->timeout);
    UART_UartPutString("\r\n");
    return length;
        
}

int generic_read(gtt_device *device)
{
    (void)device;
     uint8 data;
     
    //uint32 returncode;
    I2C_I2CMasterSendStart( ((i2cContext_t *)device->Context)->slaveAddress,I2C_I2C_READ_XFER_MODE,((i2cContext_t *)device->Context)->timeout);
    I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&data,((i2cContext_t *)device->Context)->timeout);
    I2C_I2CMasterSendStop(((i2cContext_t *)device->Context)->timeout);
    return data;
}


gtt_packet_error_t readPacketI2C(gtt_device *device) //,uint8_t *command, size_t *dataLength, uint8_t *inbuff, uint32_t buffSize)
{
    
    uint8_t data;
    uint32_t i2cerror;
    
    i2cerror = I2C_I2CMasterSendStart( ((i2cContext_t *)device->Context)->slaveAddress,I2C_I2C_READ_XFER_MODE , ((i2cContext_t *)device->Context)->timeout);
    i2cerror |= I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&data,((i2cContext_t *)device->Context)->timeout);
    i2cerror |= I2C_I2CMasterSendStop(((i2cContext_t *)device->Context)->timeout);
    
    
    // Something bad happened on the I2C Bus ....
    if(i2cerror)
    {
        sprintf(buff,"I2C Return Code %X\r\n",(unsigned int)i2cerror);
        UART_UartPutString(buff);
        return GTT_PACKET_I2CERROR;
    }
    
     // The screen returns a 0 when there is nothing in the buffer.
    if(data == 0)
    {
        return GTT_PACKET_NODATA;
    }

    // This is bad because there was something other than a packet start byte
    if(data != 252)
    {
        sprintf(buff,"bad data = %d\r\n",data);
        UART_UartPutString(buff);
        return GTT_PACKET_DATABAD;
    }
    
    // We know that we have a command
    i2cerror = I2C_I2CMasterSendStart( ((i2cContext_t *)device->Context)->slaveAddress,I2C_I2C_READ_XFER_MODE , ((i2cContext_t *)device->Context)->timeout);
    i2cerror |= I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA,&data,((i2cContext_t *)device->Context)->timeout); // command
    device->Parser.Command = data;

    // Read the Length
    i2cerror |= I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA,&data,((i2cContext_t *)device->Context)->timeout); // length
    device->Parser.Length = data<<8;
    i2cerror |= I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&data,((i2cContext_t *)device->Context)->timeout); // length
    device->Parser.Length += data;
    i2cerror |= I2C_I2CMasterSendStop(((i2cContext_t *)device->Context)->timeout);
    
    if(i2cerror)
        return GTT_PACKET_I2CERROR;
    
    if(device->Parser.Length > device->rx_buffer_size)
    {
        return GTT_PACKET_SIZE;
    }
    
    // If the packet has any data... then read it.
    if(device->Parser.Length != 0)
    {
        i2cerror |= I2C_I2CMasterSendStart( ((i2cContext_t *)device->Context)->slaveAddress,I2C_I2C_READ_XFER_MODE , ((i2cContext_t *)device->Context)->timeout);
    
        for(uint32_t i=0;i < device->Parser.Length-1; i++)
        {
            i2cerror |= I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA,&data,((i2cContext_t *)device->Context)->timeout); // length
            device->rx_buffer[device->Parser.Index+i] = data;
        }

        // Read the last byte
        i2cerror |= I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&data,((i2cContext_t *)device->Context)->timeout); // length
        device->rx_buffer[device->Parser.Index +device->Parser.Length - 1 ] = data;
        i2cerror |= I2C_I2CMasterSendStop(((i2cContext_t *)device->Context)->timeout);
        
        if(i2cerror)
            return GTT_PACKET_I2CERROR;
    }
      
    sprintf(buff,"command = %d length = %d bytes= ",device->Parser.Command,device->Parser.Length);
    UART_UartPutString(buff);
    for(uint32_t i=0;i<device->Parser.Length;i++)
    {
        //sprintf(buff,"%d ",inbuff[i]);
        sprintf(buff,"%d ",device->rx_buffer[device->Parser.Index+i]);
        UART_UartPutString(buff);
    }
    UART_UartPutString("\r\n");
    return GTT_PACKET_OK;
}


void my_gtt_event_key(gtt_device* device, uint8_t key, eKeypadRepeatMode type)
{
    (void)device;
    (void)key;
    (void)type;
    UART_UartPutString("event key\r\n");
}
void my_gtt_event_sliderchange(gtt_device* device, eTouchReportingType type, uint8_t slider, int16_t value)
{
    (void)device;
    (void)type;
    (void)slider;
    (void)value;
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
    (void)device;
    (void)ObjectID;
    (void)PropertyID;
    UART_UartPutString("event on property change\r\n");
}

void my_gtt_event_visualobject_on_key(gtt_device* device, uint16_t ObjectID, uint8_t Row, uint8_t Col, uint8_t ScanCode, uint8_t Down)
{
    (void)device;
    (void)ObjectID;
    (void)Row;
    (void)Col;
    (void)Down;
    (void)ScanCode;
    UART_UartPutString("event on key\r\n");
}
void my_gtt_event_button_click(gtt_device* device, uint16_t ObjectID, uint8_t State)
{
    (void)device;
    (void)ObjectID;
    (void)State;
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
        .ReadPacket = readPacketI2C,
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
        .Context = &i2c1,
        
};

int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    I2C_Start();
    UART_Start();
    UART_UartPutString("Started\r\n");
    
    gtt_device *gtt = &gtt_device_instance;
        
    
    int count=50;
    char c;
    int16_t val;
    gtt_text t = gtt_make_text_ascii("asdf");
        
    //Process any data coming in    
    while (1)
    {
        c = UART_UartGetChar();
        switch(c)
        {
            case 0:
            break;
            
            ////////////////////// GTT25 Commands  /////////////////
            case 'l':
                gtt_draw_line(gtt, 0, 0, 480, 272);
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
                
            case 'v':
                gtt25_get_gauge_value(gtt,9,&val);
                sprintf(buff,"Gauge Value = %d\r\n",val);
                UART_UartPutString(buff);
                break;
                

             /************* Communication Control ****************/
            case 'a':
                UART_UartPutString("alive\r\n");
            break;

            case 'p':
                UART_UartPutString("Read Packet \r\n");
                gtt_parser_process(gtt);
            break;
                
            case 'z':
                UART_UartPutString("System Mode = IDLE\r\n");
                systemMode = MODE_IDLE;
                break;
            case 'Z':
                UART_UartPutString("System Mode = POLLING\r\n");
                systemMode = MODE_POLLING;
            break;
                
            case '?':
                UART_UartPutString("-------- GTT Display Functions -------\r\n");
                UART_UartPutString("l\tDraw a line\r\n");
                UART_UartPutString("c\tClear Screen\r\n");
                UART_UartPutString("R\tReset\r\n");
                UART_UartPutString("q\tSet text label to asdf\r\n");
                UART_UartPutString("2\tSet slider to 2\r\n");
                UART_UartPutString("9\tSet slider to 9\r\n");
                UART_UartPutString("I\tSet I2C as comm channel\r\n");
                UART_UartPutString("+\tIncrement gauge\r\n");
                UART_UartPutString("-\tDecrement gauge\r\n");
                UART_UartPutString("v\tGet and print value of gauge \r\n");
                UART_UartPutString("-------- System Control Functions -------\r\n");
                UART_UartPutString("p\tRead One Packet\r\n");
                UART_UartPutString("z\tSystemMode = IDLE\r\n");
                UART_UartPutString("Z\tSystemMode = POLLING\r\n");
                UART_UartPutString("a\tPrint Alive Message\r\n");
               
            break;    
        }
       
        if(systemMode == MODE_POLLING)
        {
            gtt_parser_process(gtt);       
        }
        
    }
    return 0;
}
