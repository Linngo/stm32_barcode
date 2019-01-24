#include "delay.h"

#include <stdio.h>
#include <string.h>

#include <timers.h>
#include <led.h>

#include "uart2.h"
#include <zint.h>

uint32_t uartTimer;

void softTimerCallback(void);

uint8_t trans(uint8_t* in);
void recv(uint8_t x);

uint8_t trans(uint8_t* in){
	return *in = 'A';
}

char data_rev[800];
uint8_t rx = 0;
char Is_rev = 0;

struct Rev_format {
		uint8_t symbology;
		uint16_t bitmap_width;
		uint16_t bitmap_height;
	  uint8_t *data;
};

void recv(uint8_t x){
	data_rev[rx++] = x;
	uartTimer = TIMER_GetTime();
	Is_rev = 1;
}

int main(void)
{ 
	int8_t timerID;
	uint32_t softTimer;

	struct zint_symbol *my_symbol;
	uint16_t error_number;
	uint16_t rotate_angle;
	uint16_t x,y;
	uint16_t tmp;

	//struct Rev_format *data=NULL;
	
	delay_init(168);		  //初始化延时函数
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2	

	UART2_Init(115200,recv,NULL);
	printf("test qrcode\r\n");
	
	TIMER_Init(1000); // Initialize timer Frequency of the SysTick set at 1kHz.
		// Add a soft timer with callback running every 1000ms
	timerID = TIMER_AddSoftTimer(1000, softTimerCallback);
	TIMER_StartSoftTimer(timerID); // start the timer
	
	LED_Init(LED0); // Add an LED
	LED_Init(LED1); // Add an LED
	LED_Init(LED2); // Add an LED
	LED_Init(LED3); // Add an LED
	
	  // test another way of measuring time delays
  softTimer = TIMER_GetTime(); // get start time for delay

	while (1) {
	  // test delay method
	  if (TIMER_DelayTimer(1000, softTimer)) {
	    LED_Toggle(LED3);
	    softTimer = TIMER_GetTime(); // get start time for delay
		}
		if(Is_rev && TIMER_DelayTimer(30, uartTimer))
		{
				Is_rev = 0;
			
//				data = (struct Rev_format*)data_rev;
//			  printf("%d  %d  %d\r\n",data->symbology, data->bitmap_width, data->bitmap_height);
			  printf("%d  %d  %d\r\n",data_rev[0], *((uint16_t*)&data_rev[1]), *((uint16_t*)&data_rev[3]));
			
				my_symbol = ZBarcode_Create();
				if (!my_symbol) printf("Initialize error");
		 
				rotate_angle = 0;
			 // my_symbol->input_mode = UNICODE_MODE;//DATA_MODE; //
				my_symbol->symbology = data_rev[0];//BARCODE_CODE128;//
				my_symbol->height = 30;
				my_symbol->show_hrt = 1;
			 // my_symbol->output_options += BARCODE_BOX+BOLD_TEXT;

				error_number = ZBarcode_Encode(my_symbol, (unsigned char*) &data_rev[5], 0);
				rx = 0;
				if (error_number != 0) {
								printf("%s\n", my_symbol->errtxt);
				}
				
				printf("rows= %d and width=%d\n", my_symbol->rows, my_symbol->width);
//				for(y=0;y<my_symbol->rows;y++){
//					for(x=0;x<my_symbol->width;x++){
//						if((my_symbol->encoded_data[y][x/7] >> (x % 7))&1)
//						{
//							printf("\xA8\x80");
//						}
//						else
//						{
//							printf("  ");
//						}
//					}
//					printf("\r\n");
//				}
//				printf("\r\n");
				my_symbol->bitmap_height = *((uint16_t*)&data_rev[3]);
				my_symbol->bitmap_width  = *((uint16_t*)&data_rev[1]);

				my_symbol->scale = 2;
//				if(my_symbol->rows==1)
//					my_symbol->bitmap_height = my_symbol->height*my_symbol->scale;
//				else
//					my_symbol->bitmap_height = my_symbol->rows*my_symbol->scale;
//				my_symbol->bitmap_width  = my_symbol->width*my_symbol->scale;
				
				if (error_number < 5) {
					error_number = ZBarcode_Buffer(my_symbol,rotate_angle);
						if (error_number != 0) {
								printf("%s\n", my_symbol->errtxt);
						}
				}

				printf("height= %d and width=%d\n", my_symbol->bitmap_height, my_symbol->bitmap_width);
				tmp = (my_symbol->bitmap_width+7)/8;
				for(y=0;y<my_symbol->bitmap_height;y++){
					for(x=0;x<my_symbol->bitmap_width;x++){
						if((*(my_symbol->bitmap+y*tmp+x/8) >> (7-x % 8))&1)
						{
							printf("\xA8\x80");
						}
						else
						{
							printf("  ");
						}
					}
					printf("\r\n");
				}
				printf("\r\n");
				
				ZBarcode_Delete(my_symbol);
		}
		
			TIMER_SoftTimersUpdate(); // run timers
	}
}


void softTimerCallback(void) {

  static uint8_t counter;
  switch (counter % 4) {

  case 0:
    LED_ChangeState(LED1, LED_OFF);
    LED_ChangeState(LED2, LED_OFF);
		LED_ChangeState(LED0, LED_OFF);
    break;

  case 1:
    LED_ChangeState(LED1, LED_ON);
    LED_ChangeState(LED2, LED_OFF);
		LED_ChangeState(LED0, LED_OFF);
    break;

  case 2:
    LED_ChangeState(LED1, LED_OFF);
    LED_ChangeState(LED2, LED_ON);
		LED_ChangeState(LED0, LED_OFF);
    break;
	
  case 3:
    LED_ChangeState(LED1, LED_OFF);
    LED_ChangeState(LED2, LED_OFF);
		LED_ChangeState(LED0, LED_ON);
    break;

  }

 // printf("Test string sent from STM32F4!!!"); // Print test string
	counter++;
}
