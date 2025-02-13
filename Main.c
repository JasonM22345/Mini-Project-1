#include <stdint.h>
#include "PLL.h"
#include "LCD.h"
#include "os.h"
#include "joystick.h"
#include "FIFO.h"
#include "PORTE.h"
#include "tm4c123gh6pm.h"

// Constants
#define BGCOLOR     					LCD_BLACK
#define CROSSSIZE            			5

//------------------Defines and Variables-------------------
uint16_t origin[2]; // the original ADC value of x,y if the joystick is not touched
int16_t x = 63;  // horizontal position of the crosshair, initially 63
int16_t y = 63;  // vertical position of the crosshair, initially 63
int16_t prevx = 63;
int16_t	prevy = 63;
uint8_t select;  // joystick push

//---------------------User debugging-----------------------

#define TEST_TIMER 0		// Change to 1 if testing the timer
#define TEST_PERIOD 800000  // Defined by user
#define PERIOD 800000  		// Defined by user


unsigned long Count;   		// number of times thread loops


//--------------------------------------------------------------
void CrossHair_Init(void){
	BSP_LCD_FillScreen(LCD_BLACK);	// Draw a black screen
	BSP_Joystick_Input(&origin[0], &origin[1], &select); // Initial values of the joystick, used as reference
}

//******** Producer *************** 
void Producer(void){
#if TEST_TIMER
	PE1 ^= 0x02;	// heartbeat
	Count++;	// Increment dummy variable			
#else
	// Variable to hold updated x and y values
	int16_t newX = x;
	int16_t newY = y;
	int16_t deltaX = 0;
	int16_t deltaY = 0;
	
	uint16_t rawX, rawY; // To hold raw adc values
	uint8_t select;	// To hold pushbutton status
	rxDataType data;
	BSP_Joystick_Input(&rawX, &rawY, &select);
	
	// Your Code Here

    // Calculate deltas based on raw ADC values and origin
    deltaX = ((int16_t)rawX - (int16_t)origin[0]) / 512; // Adjust delta for joystick sensitivity
    deltaY = ((int16_t)rawY - (int16_t)origin[1]) / 512;

    // Update crosshair position based on deltas
    newX = x + deltaX;
    newY = y + deltaY;

    // Ensure the crosshair stays within screen boundaries
    if (newX < 0) newX = 0;
    if (newX > 127) newX = 127;
    if (newY < 0) newY = 0;
    if (newY > 127) newY = 127;

    // Update global crosshair position
    x = newX;
    y = newY;

    // Prepare data for FIFO
    data.x = x;
    data.y = y;

    // Push data into the FIFO
    RxFifo_Put(data);
#endif
}

//******** Consumer *************** 
void Consumer(void){
	rxDataType data;
	// Your Code Here
	
    // Check if there's new data in the FIFO
    if (RxFifo_Get(&data)) {
        // Erase the previous crosshair
        BSP_LCD_DrawCrosshair(prevx, prevy, BGCOLOR);

        // Draw the new crosshair
        BSP_LCD_DrawCrosshair(data.x, data.y, LCD_RED);

        // Display the X and Y positions at the bottom of the screen
        BSP_LCD_Message(0, 11, 0, "X:", data.x); // Top screen, line 11
        BSP_LCD_Message(0, 11, 10, "Y:", data.y); // Top screen, line 11 (shifted to column 10)

        // Update the previous position for the next iteration
        prevx = data.x;
        prevy = data.y;
    }
}

//******** Main *************** 
int main(void){
  PLL_Init(Bus80MHz);       // set system clock to 80 MHz
#if TEST_TIMER
	PortE_Init();       // profile user threads
	Count = 0;
	OS_AddPeriodicThread(&Producer, TEST_PERIOD, 1);
	while(1){}
#else
  	BSP_LCD_Init();        // initialize LCD
	BSP_Joystick_Init();   // initialize Joystick
  	CrossHair_Init();      
 	RxFifo_Init();
	OS_AddPeriodicThread(&Producer,PERIOD, 1);
	while(1){
		Consumer();
	}
#endif
} 
