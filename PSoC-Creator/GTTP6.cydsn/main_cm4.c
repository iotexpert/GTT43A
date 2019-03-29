
#include "project.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "gtt_protocol.h"
#include "gtt_device.h"


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

} i2cSettings_t;


i2cSettings_t i2c1 = {
    .slaveAddress = 0x28,
    .timeout = 0x100,
};


int i2c_generic_write(gtt_device *device, uint8_t *data, size_t length)
{
    (void)device;
    uint32 returncode;
    
   
    printf("length = %d ",length);
    
            
    returncode = Cy_SCB_I2C_MasterSendStart (I2C_HW,((i2cSettings_t *)device->Context)->slaveAddress,CY_SCB_I2C_WRITE_XFER ,
        ((i2cSettings_t *)device->Context)->timeout,&I2C_context);
    
    if(returncode != CY_SCB_I2C_SUCCESS)
    {
        printf("error = %X\r\n",(unsigned int)returncode);
    }
    
    for(size_t i=0;i<length;i++)
    {
        Cy_SCB_I2C_MasterWriteByte(I2C_HW,data[i],((i2cSettings_t *)device->Context)->timeout,&I2C_context);
        printf("%d ",data[i]);
        
    }
    
    Cy_SCB_I2C_MasterSendStop(I2C_HW,((i2cSettings_t *)device->Context)->timeout,&I2C_context);
    printf("\r\n");
    return length;
        
}

int i2c_generic_read(gtt_device *device)
{
    (void)device;
     uint8 data;
    
    
    //uint32 returncode;
    Cy_SCB_I2C_MasterSendStart (I2C_HW, ((i2cSettings_t *)device->Context)->slaveAddress,CY_SCB_I2C_READ_XFER,((i2cSettings_t *)device->Context)->timeout,&I2C_context);
    
    Cy_SCB_I2C_MasterReadByte(I2C_HW,CY_SCB_I2C_NAK,&data,((i2cSettings_t *)device->Context)->timeout,&I2C_context);

    Cy_SCB_I2C_MasterSendStop(I2C_HW,((i2cSettings_t *)device->Context)->timeout,&I2C_context);
    return data;
}


int uart_generic_read(gtt_device *device)
{
    (void)device;
 
    if(Cy_SCB_GetNumInRxFifo(UART_GTT_HW) == 0)
        return -1;
    return (int)UART_GTT_Get();
    
}

int uart_generic_write(gtt_device *device, uint8_t *data, size_t length)
{
    (void)device;
    Cy_SCB_UART_PutArray(UART_GTT_HW,data,length);
    return length;
            
}



gtt_packet_error_t readPacketI2C(gtt_device *device) //,uint8_t *command, size_t *dataLength, uint8_t *inbuff, uint32_t buffSize)
{
    
    uint8_t data;
    uint32_t i2cerror;
    
    i2cerror = Cy_SCB_I2C_MasterSendStart (I2C_HW, ((i2cSettings_t *)device->Context)->slaveAddress,CY_SCB_I2C_READ_XFER , ((i2cSettings_t *)device->Context)->timeout,&I2C_context);
    i2cerror |= Cy_SCB_I2C_MasterReadByte(I2C_HW,CY_SCB_I2C_NAK,&data,((i2cSettings_t *)device->Context)->timeout,&I2C_context);
    i2cerror |= Cy_SCB_I2C_MasterSendStop(I2C_HW,((i2cSettings_t *)device->Context)->timeout,&I2C_context);    
    
    // Something bad happened on the I2C Bus ....
    if(i2cerror)
    {
        printf("I2C Return Code %X\r\n",(unsigned int)i2cerror);
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
        printf("bad data = %d\r\n",data);
        return GTT_PACKET_DATABAD;
    }
    
    // We know that we have a command
    i2cerror = Cy_SCB_I2C_MasterSendStart (I2C_HW, ((i2cSettings_t *)device->Context)->slaveAddress,CY_SCB_I2C_READ_XFER , ((i2cSettings_t *)device->Context)->timeout,&I2C_context);
    i2cerror |= Cy_SCB_I2C_MasterReadByte(I2C_HW,CY_SCB_I2C_ACK,&data,((i2cSettings_t *)device->Context)->timeout,&I2C_context); // command
    device->Parser.Command = data;

    // Read the Length
    i2cerror |= Cy_SCB_I2C_MasterReadByte(I2C_HW,CY_SCB_I2C_ACK,&data,((i2cSettings_t *)device->Context)->timeout,&I2C_context); // length
    device->Parser.Length = data<<8;
    i2cerror |= Cy_SCB_I2C_MasterReadByte(I2C_HW,CY_SCB_I2C_NAK,&data,((i2cSettings_t *)device->Context)->timeout,&I2C_context); // length
    device->Parser.Length += data;
    i2cerror |= Cy_SCB_I2C_MasterSendStop(I2C_HW,((i2cSettings_t *)device->Context)->timeout,&I2C_context); 
    if(i2cerror)
        return GTT_PACKET_I2CERROR;
    
    if(device->Parser.Length > device->rx_buffer_size)
    {
        return GTT_PACKET_SIZE;
    }
    
    // If the packet has any data... then read it.
    if(device->Parser.Length != 0)
    {
        i2cerror |= Cy_SCB_I2C_MasterSendStart (I2C_HW, ((i2cSettings_t *)device->Context)->slaveAddress,CY_SCB_I2C_READ_XFER , ((i2cSettings_t *)device->Context)->timeout,&I2C_context);
    
        for(uint32_t i=0;i < device->Parser.Length-1; i++)
        {
            i2cerror |= Cy_SCB_I2C_MasterReadByte(I2C_HW,CY_SCB_I2C_ACK,&data,((i2cSettings_t *)device->Context)->timeout,&I2C_context); // length
            device->rx_buffer[device->Parser.Index+i] = data;
        }

        // Read the last byte
        i2cerror |= Cy_SCB_I2C_MasterReadByte(I2C_HW,CY_SCB_I2C_NAK,&data,((i2cSettings_t *)device->Context)->timeout,&I2C_context); // length
        device->rx_buffer[device->Parser.Index +device->Parser.Length - 1 ] = data;
        i2cerror |= Cy_SCB_I2C_MasterSendStop(I2C_HW,((i2cSettings_t *)device->Context)->timeout,&I2C_context); 
        
        if(i2cerror)
            return GTT_PACKET_I2CERROR;
    }
      
    printf("command = %d length = %d bytes= ",device->Parser.Command,device->Parser.Length);
    for(uint32_t i=0;i<device->Parser.Length;i++)
    {
        //sprintf(buff,"%d ",inbuff[i]);
        printf("%d ",device->rx_buffer[device->Parser.Index+i]);
    }
    printf("\r\n");
    return GTT_PACKET_OK;
}


void my_gtt_event_key(gtt_device* device, uint8_t key, eKeypadRepeatMode type)
{
    (void)device;
    (void)key;
    (void)type;
    printf("event key\r\n");
}
void my_gtt_event_sliderchange(gtt_device* device, eTouchReportingType type, uint8_t slider, int16_t value)
{
    (void)device;
    (void)type;
    (void)slider;
    (void)value;
    printf("event slider change\r\n");
}
void my_gtt_event_touch(gtt_device* device, eTouchReportingType type, uint16_t x , uint16_t y)
{
    (void)device;
    printf("Event Touch %d x=%d y=%d\r\n",type,x,y); 
}

void my_gtt_event_regiontouch(gtt_device* device, eTouchReportingType type, uint8_t region)
{
    (void)device;
    printf("Region Touch %d region=%d\r\n",type,region); 
}

void my_gtt_event_baseobject_on_property_change(gtt_device* device, uint16_t ObjectID, uint16_t PropertyID)
{
    (void)device;
    (void)ObjectID;
    (void)PropertyID;
    printf("event on property change\r\n");
}

void my_gtt_event_visualobject_on_key(gtt_device* device, uint16_t ObjectID, uint8_t Row, uint8_t Col, uint8_t ScanCode, uint8_t Down)
{
    (void)device;
    (void)ObjectID;
    (void)Row;
    (void)Col;
    (void)Down;
    (void)ScanCode;
    printf("event on key\r\n");
}
void my_gtt_event_button_click(gtt_device* device, uint16_t ObjectID, uint8_t State)
{
    (void)device;
    (void)ObjectID;
    (void)State;
    printf("event button click\r\n");
}

gtt_events myEvents = {
    .sliderchange = my_gtt_event_sliderchange,
    .touch = my_gtt_event_touch,
    .regiontouch = my_gtt_event_regiontouch,
    .baseobject_on_property_change = my_gtt_event_baseobject_on_property_change,
    .visualobject_on_key = my_gtt_event_visualobject_on_key,
    .button_click = my_gtt_event_button_click
};

    
gtt_device gtt_device_instance = {
        .Write = uart_generic_write,
        .Read = uart_generic_read,
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


void uartTask(void *arg)
{
    (void)arg;
    UART_Start();
    printf("UART Task Started\r\n");
    I2C_Start();
    UART_Start();
    setvbuf( stdin, NULL, _IONBF, 0 ); 
    
    UART_GTT_Start();
  
    #if 0
        uint8_t x;
    while(1)
    {
        while(Cy_SCB_GetNumInRxFifo(UART_GTT_HW))
        {
            
            x = UART_GTT_Get();
            printf("%02X\r\n",x);
        }
        
    }

    #endif
    
    gtt_device *gtt = &gtt_device_instance;
        
    systemMode = MODE_IDLE;
    
 //   gtt_text t = gtt_make_text_ascii("asdf");
        
    char c;
    //Process any data coming in    
    while (1)
    {
        c=0;
        if(Cy_SCB_GetNumInRxFifo(UART_HW))
            c = getchar();

        switch(c)
        {
            case 0:
            break;
            
            ////////////////////// GTT25 Commands  /////////////////
            case '1':
                gtt_run_script(gtt,"MustangElectronicDisplay_SystemManagement\\SplashScreen3\\SplashScreen3.bin");
            break;
            case '2':
                gtt_run_script(gtt,"MustangElectronicDisplay_SystemManagement\\Screen_Login\\Screen_Login.bin");
            break;
            
            case '3':
                gtt_run_script(gtt,"MustangElectronicDisplay_SystemManagement\\Screen_DriveMode1\\Screen_DriveMode1.bin");
            break;
            case '4':
                gtt_run_script(gtt,"MustangElectronicDisplay_SystemManagement\\Screen_UserManagement1\\Screen_UserManagement1.bin");
            break;
            case '5':
                gtt_run_script(gtt,"MustangElectronicDisplay_SystemManagement\\Screen_UserManagement2\\Screen_UserManagement2.bin");
            break;   
                
            case '6':
                gtt_run_script(gtt,"MustangElectronicDisplay_SystemManagement\\Screen_Fingerprint1\\Screen_Fingerprint1.bin");
            break; 
                
            case '7':
                gtt_run_script(gtt,"MustangElectronicDisplay_SystemManagement\\Screen_RFID1\\Screen_RFID1.bin");
            break;
        
            
            
            case 'l':
                gtt_draw_line(gtt, 0, 0, 480, 272);
            break;
                
            
            case 'c':
                printf("Clear Screen\r\n");
                gtt_clear_screen(gtt);
            break;
                    
            case 'R':
                gtt_reset(gtt);
            break;
                    
    
             /************* Communication Control ****************/
            case 'I':
                gtt_set_default_channel(gtt, eChannel_I2C);
            break;

            case 'U':
                gtt_set_default_channel(gtt, eChannel_Serial);
            break;
            case 'a':
                printf("alive\r\n");
            break;

            case 'p':
                printf("Read Packet \r\n");
                gtt_parser_process(gtt);
            break;
                
            case 'z':
                printf("System Mode = IDLE\r\n");
                systemMode = MODE_IDLE;
                break;
            case 'Z':
                printf("System Mode = POLLING\r\n");
                systemMode = MODE_POLLING;
            break;
                
            case '?':
                printf("-------- GTT Display Functions -------\r\n");
                printf("1\tSplashScreen3\r\n");
                printf("2\tScreen_Login\r\n");
                printf("3\tScreen_DriveMode1\r\n");
                printf("4\tScreen_UserManagement1\r\n");
                printf("5\tScreen_UserManagement2\r\n");
                printf("6\tScreen_Fingerprint1\r\n");
                printf("7\tScreen_RFID1\r\n");
                
                printf("l\tDraw a line\r\n");
                printf("c\tClear Screen\r\n");
                printf("R\tReset\r\n");
                printf("-------- System Control Functions -------\r\n");
                printf("I\tSet I2C as comm channel\r\n");
                printf("U\tSet UART as comm channel\r\n");
                printf("p\tRead One Packet\r\n");
                printf("z\tSystemMode = IDLE\r\n");
                printf("Z\tSystemMode = POLLING\r\n");
                printf("a\tPrint Alive Message\r\n");
               
            break;    
        }
       
        if(systemMode == MODE_POLLING)
        {
            gtt_parser_process(gtt);       
        }
        
    }
}

int main(void)
{
    __enable_irq(); /* Enable global interrupts. */


    xTaskCreate(uartTask,"Uart Task",1000,0,1,0);
    vTaskStartScheduler();
    for(;;)
    {
    }
}
