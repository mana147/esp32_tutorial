
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

using namespace std;
/*----------------------------------------e*/
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

TimerHandle_t xAutoOneTimer_1s;

volatile uint32_t counter_1 = 0;

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
        printf ("counter_1 : %i \n" , counter_1);

        /*----------------------------------------*/

        if ( counter_1 < 5 ) 
            gpio_set_level (LED_D_1, ON );
        else 
            gpio_set_level (LED_D_1, OFF );
        
        /*----------------------------------------*/

        if ( counter_1 >= 5 &&  counter_1 < 8 ) 
            gpio_set_level (LED_X_1, ON );
        else 
            gpio_set_level (LED_X_1, OFF );

        /*----------------------------------------*/

        if ( counter_1 >= 8 &&  counter_1 <= 10 ) 
            gpio_set_level (LED_V_1, ON );
        else
            gpio_set_level (LED_V_1, OFF );
        
        /*----------------------------------------*/

        if (counter_1 == 10)
            counter_1 = 0;

        vTaskDelay(100 / portTICK_PERIOD_MS);
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
         printf ("counter_1 : %i \n" , counter_1);

        /*----------------------------------------*/

        if ( counter_1 < 3 ) 
            gpio_set_level (LED_X_2, ON );
        else 
            gpio_set_level (LED_X_2, OFF );
        
        /*----------------------------------------*/

        if ( counter_1 >= 3 &&  counter_1 < 5 ) 
            gpio_set_level (LED_V_2, ON );
        else 
            gpio_set_level (LED_V_2, OFF );

        /*----------------------------------------*/

        if ( counter_1 >= 5 &&  counter_1 <= 10 ) 
            gpio_set_level (LED_D_2, ON );
        else
            gpio_set_level (LED_D_2, OFF );
        
        /*----------------------------------------*/

        if (counter_1 == 10)
            counter_1 = 0;

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
}

/*----------------------------------------*/
void main_AutoOneTimer_1s (TimerHandle_t pxTimer)
{
    uint32_t xTimeNow = xTaskGetTickCount();
    printf (" timer one shot : %i \n" , xTimeNow );
    counter_1++;
}

/*----------------------------------------*/
/*----------------------------------------*/
/*----------------------------------------*/

void app_main() 
{
    printf("The app_main created\n");

    /*---------------------------*/
    /* ... Create Timer here ... */
    /*---------------------------*/

    xAutoOneTimer_1s =  xTimerCreate ("one shot time" , delay_s(1) , pdTRUE , 0 , &main_AutoOneTimer_1s ); 

    /*---------------------------*/
    /* ... Timer Start here ...  */
    /*---------------------------*/

    xTimerStart( xAutoOneTimer_1s , 0 );

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
