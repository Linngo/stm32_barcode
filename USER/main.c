#include "delay.h"

#include <stdio.h>
#include <string.h>

#include <timers.h>
#include <led.h>

#include "uart2.h"
#include <zint.h>

#include <bmp.h>

uint32_t uartTimer;

void softTimerCallback(void);

uint8_t trans(uint8_t* in);
void recv(uint8_t x);

uint8_t trans(uint8_t* in){
	return *in = ' ';
}

uint8_t data_rev[800];
uint16_t rx = 0;
char Is_rev;

struct Rev_format {
		uint8_t symbology;
		uint8_t Use_scale;
		uint16_t bitmap_width;
		uint16_t bitmap_height;
}__attribute__((packed));

void recv(uint8_t x){
	data_rev[rx++] = x;
	uartTimer = TIMER_GetTime();
	Is_rev = 1;
}

char Save1bpp(struct zint_symbol *data)
{
	BITMAPFILEHEADERx head;
	memset(&head,0,sizeof(head));
	head.bfType = *((uint16_t *)"BM");
	head.bfOffBits = 0x36+4*2;//256;

	BITMAPINFOHEADERx info;
	memset(&info,0,sizeof(info));
	info.biSize = 40;
	info.biWidth =  data->bitmap_width;
	info.biHeight = data->bitmap_height;
	info.biPlanes = 1;
	info.biBitCount = 1;//8;

	uint32_t DataSizePerLine= (info.biWidth* info.biBitCount+31)/32*4;
	info.biSizeImage = DataSizePerLine*info.biHeight;

	RGBQUADx rgb[2];
	rgb[1]=(RGBQUADx){0,0,0,0};
	rgb[0]=(RGBQUADx){0xff,0xff,0xff,0};

	head.bfSize = info.biSizeImage + 14 + info.biSize +sizeof(rgb);

	uart_send((uint8_t *)&head,sizeof(BITMAPFILEHEADERx));
	uart_send((uint8_t *)&info,sizeof(BITMAPINFOHEADERx));
	uart_send((uint8_t *)&rgb,sizeof(rgb));

	uint16_t tmp = (info.biWidth+7)/8;
	uint16_t y;
	for(y=info.biHeight;y>0;y--)
	{
		uart_send((uint8_t *)(data->bitmap + tmp*(y-1)),DataSizePerLine);
	}
	return 1;
}

int main(void)
{ 
	int8_t timerID;
	uint32_t softTimer;

	struct zint_symbol *my_symbol;
	uint16_t error_number;
	uint16_t rotate_angle;
	
	Is_rev = 0;
	
	delay_init(168);		  //初始化延时函数
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2	

	UART2_Init(115200,recv,NULL);
	printf("test barcode\r\n");
	
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

        struct Rev_format *data = (struct Rev_format*)data_rev;
//			  printf("%d  %d  %d\r\n",data->symbology, data->bitmap_width, data->bitmap_height);
			
				my_symbol = ZBarcode_Create();
				if (!my_symbol) {
					printf("Initialize error");
					goto end;
				}
		 
				rotate_angle = 0;
			 // my_symbol->input_mode = UNICODE_MODE;//DATA_MODE; //
				                       //BARCODE_CODE128;//
				my_symbol->symbology = data->symbology;//data_rev[0];
				my_symbol->height = 30;
				my_symbol->show_hrt = 1;
			 // my_symbol->output_options += BARCODE_BOX+BOLD_TEXT;
				
				//my_symbol->dot_size = 7.5 / 5.0;

        my_symbol->Use_scale = data->Use_scale;//data_rev[1];
        my_symbol->scale = 2;			
				my_symbol->bitmap_height = data->bitmap_height;//*((uint16_t*)&data_rev[4]);
				my_symbol->bitmap_width  = data->bitmap_width;//*((uint16_t*)&data_rev[2]);

				//error_number = ZBarcode_Encode(my_symbol, (unsigned char*) &data_rev[6], 0);
				error_number = ZBarcode_Encode(my_symbol, (unsigned char*) (data_rev +sizeof( struct Rev_format)), 0);
				rx = 0;
				uint16_t i;
				for(i = 0;i<800;i++)
				 data_rev[i] = 0;
				 
				if (error_number != 0) {
					  printf("%x:%s\n", error_number,my_symbol->errtxt);
					  goto end;
				}			
//				printf("rows= %d and width=%d\n", my_symbol->rows, my_symbol->width);
				
				if (error_number < 5) {
					error_number = ZBarcode_Buffer(my_symbol,rotate_angle);
					if (error_number != 0) {
							printf("%s\n", my_symbol->errtxt);
							goto end;
					}
				}

//				printf("height= %d and width=%d\n", my_symbol->bitmap_height, my_symbol->bitmap_width);
/***************************绘点*********************************/				
/*				uint16_t x,y;
				uint16_t tmp;	
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
				printf("\r\n");*/
/***************BMP *********************************************/	
				Save1bpp(my_symbol);
/****************************************************************/	
end:		
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
