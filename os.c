// Modified by Mustafa Hotaki 8/1/2018
// MODIFIED BY SILE SHU 2017.6
// os.c
// Runs on LM4F120/TM4C123
// A very simple real time operating system with minimal features.
// Daniel Valvano
// January 29, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Programs 4.4 through 4.12, section 4.2
 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#include <stdint.h>
#include "os.h"
#include "PLL.h"
#include "tm4c123gh6pm.h"

// Functions implemented in assembly files
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// Configure Timer1A
void InitTimer1A(unsigned long period, unsigned long priority) {
	long sr;
	volatile unsigned long delay;
	
	sr = StartCritical();
  SYSCTL_RCGCTIMER_R |= 0x02;
	while((SYSCTL_RCGCTIMER_R & 0x02) == 0){} // allow time for clock to stabilize
		
	TIMER1_CTL_R &= ~TIMER_CTL_TAEN; 														            // 1) disable timer1A during setup, please complete and uncomment this line
  TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;   										// 2) configure for 32-bit timer mode
  TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD; 										// 3) configure for periodic mode, default down-count settings
	TIMER1_TAILR_R = period - 1;  															        // 4) reload value, please complete and uncomment this line       
  TIMER1_ICR_R = TIMER_ICR_TATOCINT; 													// 5) clear timer1A timeout flag
  TIMER1_IMR_R |= TIMER_IMR_TATOIM; 													// 6) arm timeout interrupt			   
  NVIC_PRI5_R = (NVIC_PRI5_R & 0xFFFF00FF)|(priority << 13);	// 7) priority shifted to bits 15-13 for timer1A
  NVIC_EN0_R = NVIC_EN0_INT21;     														// 8) enable interrupt 21 in NVIC
  TIMER1_TAPR_R = 0;
  TIMER1_CTL_R |= TIMER_CTL_TAEN;  														// 9) enable timer1A
  EndCritical(sr);
}

// Function Pointer
void (*PeriodicTask)(void);

// Timer1A_Handler *************** 
void Timer1A_Handler(void){ 
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer1A timeout
	(*PeriodicTask)();
}

//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
int OS_AddPeriodicThread(void(*task)(void), unsigned long period, unsigned long priority) {
  if (PeriodicTask != 0) {             // Checking if a periodic task is already running
    return 0;                          // Return 0 if the thread cannot be added
  }
  PeriodicTask = task;                 // Assign the task to the global function pointer
  InitTimer1A(period, priority);       // Initializing Timer1A with the specified period and priority
  return 1;   
}
