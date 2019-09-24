#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

using namespace std;
/*----------------------------------------*/
extern "C"
{
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
    #include "freertos/timers.h"
    #include "driver/gpio.h"
    #include "sdkconfig.h"

    /*----------------------------------------*/
    #include "esp_system.h"
    /*----------------------------------------*/
    void app_main();
}
/*----------------------------------------*/
/*----------------------------------------*/
#define delay_ms(x) pdMS_TO_TICKS(x)

#define CORE_APP 1 
#define CORE_PRO 0 
#define CORE_ALL tskNO_AFFINITY 

#define LED_D_1 GPIO_NUM_15
#define LED_V_1 GPIO_NUM_2
#define LED_X_1 GPIO_NUM_4

#define LED_D_2 GPIO_NUM_5
#define LED_V_2 GPIO_NUM_18
#define LED_X_2 GPIO_NUM_19

#define ON 0
#define OFF 1

/*----------------------------------------*/
/*----------------------------------------*/

/*----------------------------------------*/
/*----------------------------------------*/
uint32_t delay_(uint32_t t )
{
    return  ( t / portTICK_RATE_MS ) ; 
}

uint32_t delay_s(uint32_t t )
{
    return  (pdMS_TO_TICKS(t*1000)) ; 
}
/*----------------------------------------*/
/*----------------------------------------*/
void on_off_gpio_1(unsigned char k)
{
    gpio_set_level (LED_D_1, k);
    gpio_set_level (LED_V_1, k);
    gpio_set_level (LED_X_1, k);
}
void on_off_gpio_2(unsigned char k)
{
    gpio_set_level (LED_D_2, k);
    gpio_set_level (LED_V_2, k);
    gpio_set_level (LED_X_2, k);
}
/*----------------------------------------*/
static void task_1(void *pvParameter)
{
    gpio_pad_select_gpio( LED_D_1 );
    gpio_pad_select_gpio( LED_V_1 );
    gpio_pad_select_gpio( LED_X_1 );

    gpio_set_direction( LED_D_1 , GPIO_MODE_OUTPUT);
    gpio_set_direction( LED_V_1 , GPIO_MODE_OUTPUT);
    gpio_set_direction( LED_X_1 , GPIO_MODE_OUTPUT);

    on_off_gpio_1(OFF);

    while (1)
    {
        gpio_set_level (LED_D_1, ON );
        vTaskDelay(delay_s(5));
        gpio_set_level (LED_D_1, OFF );

        gpio_set_level (LED_X_1, ON );
        vTaskDelay(delay_s(3));
        gpio_set_level (LED_X_1, OFF );

        gpio_set_level (LED_V_1, ON );
        vTaskDelay(delay_s(2));
        gpio_set_level (LED_V_1, OFF );
    }
    
}

static void task_2(void *pvParameter)
{

    gpio_pad_select_gpio( LED_D_2 );
    gpio_pad_select_gpio( LED_V_2 );
    gpio_pad_select_gpio( LED_X_2 );

    gpio_set_direction( LED_D_2 , GPIO_MODE_OUTPUT);
    gpio_set_direction( LED_V_2 , GPIO_MODE_OUTPUT);
    gpio_set_direction( LED_X_2 , GPIO_MODE_OUTPUT);

    on_off_gpio_2(OFF);
    
    while (1)
    {
        gpio_set_level (LED_X_2, ON );
        vTaskDelay(delay_s(3));
        gpio_set_level (LED_X_2, OFF );

        gpio_set_level (LED_V_2, ON );
        vTaskDelay(delay_s(2));
        gpio_set_level (LED_V_2, OFF );

        gpio_set_level (LED_D_2, ON );
        vTaskDelay(delay_s(5));
        gpio_set_level (LED_D_2, OFF );
    }
    
}

/*----------------------------------------*/
/*----------------------------------------*/
/*----------------------------------------*/

void app_main() 
{
    printf("The app_main created\n");

    /*---------------------------*/
    /* ... Create tasks here ... */
    /*---------------------------*/

    xTaskCreatePinnedToCore (&task_1 , "task 1" , 1024*4 , NULL , 5 , NULL , CORE_APP );
    xTaskCreatePinnedToCore (&task_2 , "task 2" , 1024*4 , NULL , 5 , NULL , CORE_APP );

    vTaskSwitchContext(); /* or vTaskStartScheduler() */
}

/*----------------------------------------*/
/*----------------------------------------*/
/*----------------------------------------*/
