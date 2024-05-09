/* ============================================================================ */
/*		  Exp01_3.c : Basic C Program (1) with header file		*/
/* ============================================================================ */
/*			    Designed and programmed by Duck-Yong Yoon in 2020.  */
/*	펌웨어 업데이트 내용	*/
/*
    Uart1 비활성화 및 Uart1 ISR 삭제
    Uart0 ISR 함수 편집
    
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include "OK-128TFT-V2.h"

#define Max_length 255

void USART_Init();
void Timer_Init();
void Play_note(char sound, char octave, char rest);
void USART0_Transmit(unsigned char data);
void USART1_Transmit(unsigned char data);

volatile unsigned char RXD, RXD1;
volatile char total[Max_length];
volatile char note[Max_length][2];
volatile char pitch[Max_length][3];

unsigned char flag = 0, Uart_flag = 0;
unsigned char idx = 0, j = 0;

/* ---------------------------------------------------------------------------- */
/*			  인터럽트 서비스 루틴 함수				*/
/* ---------------------------------------------------------------------------- */
ISR(USART0_RX_vect)
{	
   RXD = UDR0;
   if(flag == 0)
   {
     if(RXD == '.')
      {
       total[idx] = '\0';				// '.'이 전송되면 total 배열 마지막에 전송
       flag = 1;
      }
      else total[idx++] = UDR0;			// 들어오는 데이터 배열에 저장
   }
}
/* ---------------------------------------------------------------------------- */
/*				Main 함수					*/
/* ---------------------------------------------------------------------------- */
int main(void)
{
   // unsigned char k;
   // unsigned int glitch;
  unsigned char temp = 0; 
  MCU_initialize();                             // initialize ATmega128A MCU
  Delay_ms(50);
  LCD_initialize();
  LCD_string(0x80,"pitch:   note:  ");
  
  USART_Init();					// initializing Uart0
  Timer_Init();
  sei();					// global interrupt Enable
  
  while(1)
    { 
      if(flag)
      {
	unsigned char noteIdx = 0, pitchIdx = 0;
	unsigned char col = 0;
	for(unsigned char i = 0;total[i] != '\0';i++)
	{
	  if(i % 3 == 0)
	  {
	     pitch[col][pitchIdx++] = total[i];
	  }
	  else if(i % 3 == 1)
	  {
	    pitch[col][pitchIdx++] = total[i];
	    pitch[col][pitchIdx] = '\0';
	    pitchIdx = 0;
	  }
	  else if(i % 3 == 2)
	  {
	    note[col][noteIdx++] = total[i];
	    note[col++][noteIdx--] = '\0';
	  }
	}
	temp = col;
      }
      for(unsigned char i = 0; i < temp; i++)
      {
	LCD_command(0x86);
	LCD_data(pitch[i][0]);
	LCD_command(0x87);
	LCD_data(pitch[i][1]);
	LCD_command(0x8E);
	LCD_data(note[i][0]);
	Delay_ms(1000);
      }
    }
}


/* ---------------------------------------------------------------------------- */
/*			  Uart, Timer1 초기화 함수				*/
/* ---------------------------------------------------------------------------- */
void USART_Init()
{
  // 0
  UBRR0H = 0;	  // baudrate = 19200 bps
  UBRR0L = 51;	  // UBRR1 = 16MHz / 16 / 19200 - 1 = 약 103
  UCSR0A = 0x00;  // 0000 0000	    asynchronous normal mode
  UCSR0B = 0x98;  // 1001 1000	    interrupt enable, 8 data, 수신부만 작동
  UCSR0C = 0x06;  // 0000 0110	    no parity, 1stop, 8 data
  // 1
  //UBRR1H = 0;	  // baudrate = 19200 bps
  //UBRR1L = 51;	  // UBRR1 = 16MHz / 16 / 19200 - 1 = 약 103
  //UCSR1A = 0x00;  // 0000 0000	    asynchronous normal mode
  //UCSR1B = 0x98;  // 1001 1000	    interrupt enable, 8 data, 수신부만 작동
  //UCSR1C = 0x06;  // 0000 0110	    no parity, 1stop, 8 data
  
}
void Timer_Init()
{
  sbi(DDRB,5);	  // PortB set		  OC1A output
  TCCR1A = 0x40;  // 0100 0000		  CTC mode(12), use OC1A
  TCCR1B = 0x18;  // 0001 1010		  f(OC1A) = 16Mhz/8/2/(1+TOP)
  TCCR1C = 0x00;  // 0000 0000
  TCNT1 = 0x0000; // 0000 0000 0000 0000  // clear Timer1
}
/*  TCCR1A 
    - COM1A0 -> 1 OC1A를 사용한다는 뜻 compare match에서 OC1A 출력을 토글  
    - WGM11 -> 0  동작모드 CTC
    - WGM10 -> 0 
    TCCR1B
    - WGM13 -> 1  동작모드 CTC	  (결과적으로 WGM13~WGM1~0 : 1100)
    - WGM12 -> 1	    
    - CS11  -> 1   clk_io / 8
    TCCR1C
*/
void USART0_Transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));  // 전송 버퍼가 빌 때까지 대기
    UDR0 = data;  // 데이터를 전송 버퍼에 넣고 전송
}
void USART1_Transmit(unsigned char data) {
	while (!(UCSR1A & (1 << UDRE1)));  // 전송 버퍼가 빌 때까지 대기
	UDR1 = data;  // 데이터를 전송 버퍼에 넣고 전송
}
/* ---------------------------------------------------------------------------- */
/*				  사용자 정의 함수				*/
/* ---------------------------------------------------------------------------- */
void Play_note(char sound, char octave, char rest)
{
  if(sound == 'C' && octave == '3')	 ICR1 = 7644;
  else if(sound == 'D' && octave == '3') ICR1 = 6810;
  else if(sound == 'E' && octave == '3') ICR1 = 6066;
  else if(sound == 'F' && octave == '3') ICR1 = 5726;
  else if(sound == 'G' && octave == '3') ICR1 = 5101;
  else if(sound == 'A' && octave == '3') ICR1 = 4544;
  else if(sound == 'B' && octave == '3') ICR1 = 4049;
  else if(sound == 'C' && octave == '4') ICR1 = 3821;
  else if(sound == 'D' && octave == '4') ICR1 = 3404;
  else if(sound == 'E' && octave == '4') ICR1 = 3033;
  else if(sound == 'F' && octave == '4') ICR1 = 2862;
  else if(sound == 'G' && octave == '4') ICR1 = 2550;
  else if(sound == 'A' && octave == '4') ICR1 = 2272;
  else if(sound == 'B' && octave == '4') ICR1 = 2024;
  else if(sound == 'C' && octave == '5') ICR1 = 1910;
  else if(sound == 'D' && octave == '5') ICR1 = 1702;
  else if(sound == 'E' && octave == '5') ICR1 = 1516;
  else if(sound == 'F' && octave == '5') ICR1 = 1431;
  else if(sound == 'G' && octave == '5') ICR1 = 1275;
  else if(sound == 'A' && octave == '5') ICR1 = 1135;
  else if(sound == 'B' && octave == '5') ICR1 = 1011;
  
  TCNT1 = 0x0000;
  TCCR1B = 0x1A;
  if(rest == '1')	Delay_ms(16*3*7*4);
  else if(rest == '2')	Delay_ms(24*3*7*4);
  else if(rest == '3')	Delay_ms(8*3*7*4);
  else if(rest == '4')	Delay_ms(12*3*7*4);
  else if(rest == '5')	Delay_ms(4*3*7*4);
  else if(rest == '6')	Delay_ms(6*3*7*4);
  else if(rest == '7')	Delay_ms(2*3*7*4);
  else if(rest == '8')	Delay_ms(3*3*7*4);
  else if(rest == '9')	Delay_ms(1*3*7*4);
  TCCR1B = 0x18;
}