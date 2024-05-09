/* ============================================================================ */
/*			기타 자동 튜너 for Fast Fourier Transform		*/
/* ============================================================================ */
/*					   Programmed by Sun-Ho CHOI in 2016.   */

#include "stm32f091xc.h"
#include "OK-STM091.h"
#include  <arm_math.h>

void Draw_axis(void);				/* draw x-axis and y-axis */
void Draw_FFT(U16 index, float value);
void TIM7_IRQHandler(void);
void TIM1_PWM(U16 PWM);
void Generate_INA(void);
void TIM1_stop(void);
void TIM1_start(void);
void TIM7_stop(void);
void TIM7_start(void);


float ADC_buffer[1024], FFT_buffer[1024], FFT_input[1024], FFT_output[512], max_value;
unsigned int max_index;
unsigned short FFT_count, end_flag, FFT_flag;
arm_cfft_radix4_instance_f32 S;

/* ------------------------- 인터럽트 처리 프로그램 -------------------------- */

void TIM7_IRQHandler(void)
{
  TIM7->SR = 0x0000;
  
  ADC1->CHSELR = 0x00000040;			// channel 6 (TP13)
  ADC1->CR |= 0x00000004;			// start conversion by software
  while(!(ADC1->ISR & 0x00000004));		// wait for end of conversion
  FFT_buffer[FFT_count] =((float)ADC1->DR - 2048.)/2048.;
  FFT_buffer[FFT_count + 1] = 0.;
  
  FFT_count += 2;
  if(FFT_count >= 1024)
  {
    FFT_count = 0;
    FFT_flag = 1;
  }
}

/* ----------------------------- 메인 프로그램 -------------------------------- */

int main(void)
{
  unsigned short i, temp;
  float max;
  
  Initialize_MCU();				// initialize MCU and kit
  Delay_ms(50);					// wait for system stabilization
  Initialize_LCD();				// initialize text LCD module
  Initialize_TFT_LCD();				// initialize TFT-LCD module
  arm_cfft_radix4_init_f32(&S, 256, 0, 1);
  LCD_string(0x80, "  Guitar Tuner  ");		// display title
  LCD_string(0xC0, "   FFT Result   ");
  
  Draw_axis();  
  ADC1->CR = 0x80000000;			// ADC calibration
  while(ADC1->CR & 0x80000000);

  GPIOA->MODER |= 0x00003000;			// use ADC6
  RCC->APB2ENR |= 0x00000200;			// enable ADC clock

  ADC1->CR = 0x00000001;			// enable ADC
  ADC1->SMPR = 0x00000005;			// sampling time = 55.5 cycle = 4.625 us
  ADC1->CFGR1 = 0x00000000;			// 12-bit resolution, single conversion mode
  ADC1->CFGR2 = 0x80000000;			// ADC clock = PCLK/4 = 12MHz
  
  RCC->APB1ENR |= 0x00000020;			// enable TIM7 clock
  TIM7->PSC = 4;				// 48MHz/(4+1) = 9.6MHz
  TIM7->ARR = 1875;				// 9.6MHz/(1874+1) = 5120Hz
  TIM7->CNT = 0;				// clear counter
  TIM7->DIER = 0x0001;				// enable update interrupt
  TIM7->CR1 = 0x0085;				// enable TIM7, update event, and preload
  NVIC->ISER[0] |= 0x00040000;			// enable (55)TIM7 interrupt
  
  TFT_string(5,1,White,Black,"가장 높은 주파수 :     [Hz]");
  
  while(1)
    { 
      if(FFT_flag == 1)				// If complete Sampling, FFT calculate
      {
	FFT_flag = 0;
      for(i = 0; i < 1024; i++)FFT_input[i] = FFT_buffer[i];
      arm_cfft_radix4_f32(&S, FFT_input);
      arm_cmplx_mag_f32(FFT_input, FFT_output, 512);
      arm_max_f32(FFT_output,512, &max_value, &max_index);
      LED_toggle();
      	
      for(i = 1; i < 128; i++) Draw_FFT(i, FFT_output[i]);    // Drawing FFT Graph
      
      max = 0;
      
      for(i = 1; i < 512; i++)			// 가장 큰 진폭을 갖는 주파수 출력
      {
	if(max < FFT_output[i]) 
	{
	  max = FFT_output[i];
	  temp = i;
	}
      }
      
      TFT_xy(24, 1);
      TFT_color(Green, Black);
      TFT_unsigned_decimal(temp,1,4);
      }
    }
}

/* ----- 사용자 정의 함수 ----------------------------------------------------- */

void Draw_axis(void)				/* draw x-axis and y-axis */
{
  unsigned short x;

  TFT_clear_screen();				// clear screen

  TFT_color(White,Black);
  TFT_English_pixel(26, 222, '0');		// 0
  TFT_English_pixel(46, 222, '1');		// 1
  TFT_English_pixel(66, 222, '2');		// 2
  TFT_English_pixel(86, 222, '3');		// 3
  TFT_English_pixel(106,222, '4');		// 4
  TFT_English_pixel(126,222, '5');		// 5
  TFT_English_pixel(146,222, '6');		// 6
  TFT_English_pixel(166,222, '7');		// 7
  TFT_English_pixel(186,222, '8');		// 8
  TFT_English_pixel(206,222, '9');		// 9
  TFT_English_pixel(222,222, '1');		// 10
  TFT_English_pixel(230,222, '0');
  TFT_English_pixel(242,222, '1');		// 11
  TFT_English_pixel(250,222, '1');
  TFT_English_pixel(262,222, '1');		// 12
  TFT_English_pixel(270,222, '2');
  TFT_color(Magenta,Black);		
  TFT_English_pixel(288,223, '[');		// [Hz]
  TFT_English_pixel(296,223, 'H');
  TFT_English_pixel(304,223, 'z');
  TFT_English_pixel(312,223, ']');

  Line(30, 220, 310, 220, White);		// x-axis line
  Line(305,215, 310, 220, White);
  Line(305,225, 310, 220, White);
  for(x = 50; x <= 309; x += 20)		// x-axis scale
    Line(x,218, x,222, White);
  
}
void Draw_FFT(U16 index, float value)
{
  unsigned int height;
  
  height = (unsigned int)(180.*value/max_value + 0.5);
  if(height >= 180.) height = 180.;
  
  Line(30+2*index, 219, 30+2*index, 219 - 180, Black);
  Line(30+2*index, 219, 30+2*index, 219 - height, Magenta);
  
}

void TIM1_PWM(U16 PWM)
{
  RCC->APB2ENR  |= 0x00000800;			// enable TIM1 clock
  
  TIM1->PSC = 47;				// 48MHz/(47+1) = 1MHz
  TIM1->ARR = 999;				// 1MHz / (999+1) = 1kHz
  TIM1->CCR4 = PWM;
  TIM1->CNT = 0;				// clear counter
  TIM1->CCMR2 = 0x6C00;				// OC4M = 110(PWM mode 1), CC4S = 00(output)
  TIM1->CCER = 0x1000;				// CC4E = 1(enable OC4 output)
  TIM1->BDTR = 0x8000;				// MOE = 1
  TIM1->CR1 = 0x0005;				// edge-aligned, up-counter, enable TIM1
}

void TIM1_stop(void)				//TIM1 Stop
{
  TIM1->CR1 &= 0x03FE;				
}
void TIM1_start(void)				//TIM1 Enable
{
  TIM1->CR1 |= 0x0001;
}
void TIM7_stop(void)				//TIM7 Stop
{
  TIM7->CR1 &= 0x00FE;
}
void TIM7_start(void)				//TIM1 Enable
{
  TIM7->CR1 |= 0x0001;
}
void Generate_INA(void)
{
  
  GPIOE->MODER  |= 0x00050000;			//PC8,9 = OUTPUT
  GPIOE->PUPDR  &= 0x11101111;			//PC8,9 pull,push x	
  GPIOE->OTYPER |= 0x00000000;
  
  GPIOE->ODR = 0x00000100;			//PC8 Low, PC9 High
  
}