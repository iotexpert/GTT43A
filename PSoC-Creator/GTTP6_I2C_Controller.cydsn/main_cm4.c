
#include "project.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "I2CController.h"

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

//
int generic_write(gtt_device *device, uint8_t *data, size_t length)
{
//    I2CmlWriteTransaction(uint8_t i2cAddress,uint8_t *data,uint32_t length)
    I2CmlWriteTransaction( ((i2cSettings_t *)device->Context)->slaveAddress,data,length);
    return 0;
}

gtt_packet_error_t readPacketI2C(gtt_device *device)
{

    uint8_t buffer[32];
    uint32_t finalLength;
    uint32_t errorStatus;
    
    
    errorStatus = I2CmlReadGttTransaction(((i2cSettings_t *)device->Context)->slaveAddress,buffer,sizeof(buffer),&finalLength);
    if(errorStatus)
        return GTT_PACKET_I2CERROR;
    
    if(finalLength == 0)
        return GTT_PACKET_NODATA;
   
#if 0    
    printf("Read Packet Len=%d = ",finalLength);
    for(uint32_t i=0;i<finalLength;i++)
    {
        printf("%02X ",buffer[i]);  
    }
    printf("\r\n");
#endif

    device->Parser.Command = buffer[0];
    device->Parser.Length = buffer[1]<<8 | buffer[2];
    for(uint32_t i=0;i < device->Parser.Length; i++)
    {
        device->rx_buffer[device->Parser.Index+i] = buffer[3+i];
    }
    
    return GTT_PACKET_OK;
    
}
    
gtt_device gtt_device_instance = {
        .Write = generic_write,
        .Read = 0, // Not using this one
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
    
    UART_Start();
    setvbuf( stdin, NULL, _IONBF, 0 ); 
    
    gtt_device *gtt = &gtt_device_instance;
        
    systemMode = MODE_IDLE;
    
    int count=50;
    char c;
    int16_t val;
    gtt_text t = gtt_make_text_ascii("asdf");
        
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
                    
            case 'q':
                printf("Set Text\r\n");
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
                printf("Gauge Value = %d\r\n",val);
                break;
            case 's':
                gtt25_get_slider_value(gtt,3,&val);
                printf("Slider Value = %d\r\n",val);
                break;

             /************* Communication Control ****************/
            case 'a':
                printf("alive\r\n");
            break;

            case 'p':
                printf("Read Packet \r\n");
                gtt_parser_process(gtt);
            break;
            case 'P':    
                readPacketI2C(gtt);  
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
                printf("l\tDraw a line\r\n");
                printf("c\tClear Screen\r\n");
                printf("R\tReset\r\n");
                printf("q\tSet text label to asdf\r\n");
                printf("2\tSet slider to 2\r\n");
                printf("9\tSet slider to 9\r\n");
                printf("I\tSet I2C as comm channel\r\n");
                printf("+\tIncrement gauge\r\n");
                printf("-\tDecrement gauge\r\n");
                printf("v\tGet and print value of gauge \r\n");
                printf("-------- System Control Functions -------\r\n");
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

void TaskHandler_I2Cm1(void *);

int main(void)
{
    __enable_irq(); /* Enable global interrupts. */


    xTaskCreate(uartTask,"Uart Task",1000,0,1,0);
    xTaskCreate(TaskHandler_I2Cm1,"I2C Master",400,0,2,0);
    vTaskStartScheduler();
    for(;;)
    {
    }
}
