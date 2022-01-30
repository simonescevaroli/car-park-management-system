/* IoT Project
 * ELISA COMPOSTA and SIMONE SCEVAROLI
 * PROJECT DESCRIPTION IN A NUTSHELL:
 * In this project we realized an automated car park management system, 
 * to control the number of cars entering a car park to reduce useless traffic 
 * and to avoid people wasting their time searching for a (non-existing) park.
*/
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "HAL_I2C.h"
#include "HAL_OPT3001.h"
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include <stdio.h>

#define ON 1
#define OFF 0
#define RED 0
#define GREEN 1
#define WHITE 2
#define SERVO_MOTOR_START 3700
#define SERVO_MOTOR_END 7500
#define BUZZER_ON 5000
#define BUZZER_OFF 0

/* Graphic library context */
Graphics_Context g_sContext_msg;
Graphics_Context g_sContext_title;
Graphics_Context g_sContext_n;

/* Variable for storing lux value returned from OPT3001 */
float lux;
float lux2;

//Configuration of PWM for buzzer
/* Timer_A Up Configuration Parameter */
const Timer_A_UpModeConfig upConfigBuzzer = {
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK = 48 MHz (modified in hwInit function)
        TIMER_A_CLOCKSOURCE_DIVIDER_12,         // SMCLK/12
        10000,                                  // Tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,
        TIMER_A_DO_CLEAR
        };

/* Timer_A Compare Configuration Parameter  (PWM) */
Timer_A_CompareModeConfig compareConfig_PWMBuzzer = {
        TIMER_A_CAPTURECOMPARE_REGISTER_4,          // Buzzer uses CCR4
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
        TIMER_A_OUTPUTMODE_TOGGLE_SET,
        BUZZER_OFF                                           // Compare value
        };

//Configuration of PWM for servo motor
/* Timer_A Up Configuration Parameter */
const Timer_A_UpModeConfig upConfigServo = {
        TIMER_A_CLOCKSOURCE_SMCLK,                      // SMCLK
        TIMER_A_CLOCKSOURCE_DIVIDER_12,                 // SMCLK/12
        0xffff,
        TIMER_A_TAIE_INTERRUPT_DISABLE,
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,
        TIMER_A_DO_CLEAR
        };

/* Timer_A Compare Configuration Parameter  (PWM) */
Timer_A_CompareModeConfig compareConfig_PWMServo = {
TIMER_A_CAPTURECOMPARE_REGISTER_2,                  // Servo uses CCR2
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
        TIMER_A_OUTPUTMODE_TOGGLE_SET,
        0x0000
        };



/* Initialization of functions for hardware */

void _graphicsInit(){
    /* Initialize display */
    Crystalfontz128x128_Init();

    /* Set screen orientation upside down for a better usage of the light sensor */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_DOWN);

    /* Initialize graphics context */
    Graphics_initContext(&g_sContext_title, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_initContext(&g_sContext_msg, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_initContext(&g_sContext_n, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);

    /* Set string color */
    Graphics_setForegroundColor(&g_sContext_title, GRAPHICS_COLOR_GREEN);
    Graphics_setForegroundColor(&g_sContext_msg, GRAPHICS_COLOR_RED);
    Graphics_setForegroundColor(&g_sContext_n, GRAPHICS_COLOR_RED);


    Graphics_setBackgroundColor(&g_sContext_msg, GRAPHICS_COLOR_BLACK);     // Is set by default

    /* Set string font */
    GrContextFontSet(&g_sContext_msg, &g_sFontFixed6x8);
    GrContextFontSet(&g_sContext_title, &g_sFontCmtt26);
    GrContextFontSet(&g_sContext_n, &g_sFontCmtt22);

    Graphics_clearDisplay(&g_sContext_msg);
    Graphics_clearDisplay(&g_sContext_title);
    Graphics_clearDisplay(&g_sContext_n);
}

void _ledInit(){
    /* set LEDs in output mode */
    GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN6);
    GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN4);
    GPIO_setAsOutputPin(GPIO_PORT_P5,GPIO_PIN6);
}

void _buzzerInit(){
    /* Configures P2.7 to PM_TA0.4 for using Timer PWM to control buzzer */
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring Timer_A0 for Up Mode and starting */
    Timer_A_configureUpMode(TIMER_A0_BASE, &upConfigBuzzer);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    /* Initialize compare registers to generate PWM */
    Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMBuzzer);
}

void _servoInit(){
    /* Configures P2.5 to PM_TA0.2 for using Timer PWM to control servo motor */
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring Timer_A0 for Up Mode and starting */
    Timer_A_configureUpMode(TIMER_A0_BASE, &upConfigServo);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    /* Initialize compare registers to generate PWM */
    Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMServo);
}

void _lightSensorInit(){
    /* Initialize I2C communication */
    Init_I2C_GPIO();
    I2C_init();

    /* Initialize OPT3001 digital ambient light sensor */
    OPT3001_init();

    __delay_cycles(100000);
}

void _hwInit(){
    /* Halting WDT and disabling master interrupts */
    WDT_A_holdTimer();
    Interrupt_disableMaster();

    /* Set the core voltage level to VCORE1 */
    PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Initialize Clock System */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Initialize all the sensors */
    _graphicsInit();
    _lightSensorInit();
    _servoInit();
    _buzzerInit();
    _ledInit();
}



/* Some useful functions to make our code more readable */

void buzzer_servo_set(bool state){
    if(state){
        GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION);     // Turning the servo motor "on"
        compareConfig_PWMBuzzer.compareValue = BUZZER_ON;         // Turn the buzzer on (set duty cycle)

        /* Inverting the value of PWM (to move servo motor up and down) */
        if(compareConfig_PWMServo.compareValue==SERVO_MOTOR_END){
            compareConfig_PWMServo.compareValue=SERVO_MOTOR_START;
        }else{
            compareConfig_PWMServo.compareValue=SERVO_MOTOR_END;
        }

    }else{
        compareConfig_PWMBuzzer.compareValue = BUZZER_OFF;         // Turn the buzzer off (set duty cycle to 0)
    }

    /* Reset timer with PWM with new duty cycle value */
    Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMBuzzer);
    Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMServo);


    int j;
    for(j=0;j<2000000;j++);

    GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN5);    // Turning servo motor "off", to stop receiving PWM signal
}

void LED_set(int color){
    switch(color){
    /* Turn on the red LED and turn off all the others */
    case RED: GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN6);
              GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4);
              GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN6);
              break;
    /* Turn on the green LED and turn off all the others */
    case GREEN:   GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN6);
                  GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN4);
                  GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN6);
                  break;
    /* Turn on all the LEDs simultaneously to make white color */
    case WHITE: GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN6);
                GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN4);
                GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN6);
                break;
    }
}

void print_lastLine(char *string){
    /* Simple function to print the last line in LCD screen (the one that is changing more frequently) */
    char stringLastLine[20];
    sprintf(stringLastLine,"%s",string);
    Graphics_drawStringCentered(&g_sContext_n, (int8_t *) stringLastLine, AUTO_STRING_LENGTH, 65, 105, OPAQUE_TEXT);
}



/*
 * Main function
 */
int main(void){

    _hwInit();

    /* Setting title and first lines on LCD display */
    char string_n[20];
    Graphics_drawStringCentered(&g_sContext_title, (int8_t *) "WELCOME!", AUTO_STRING_LENGTH, 64, 20, OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext_msg, (int8_t *) "Available", AUTO_STRING_LENGTH, 64, 45, OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext_msg, (int8_t *) "parking spaces:", AUTO_STRING_LENGTH, 64, 55, OPAQUE_TEXT);

    /* Initial state: red LED on, buzzer off and servo motor in initial state (bar is closed) */
    int8_t counter=11;
    bool isWaiting=false;
    buzzer_servo_set(OFF);
    LED_set(RED);


    /* Obtain lux value from light sensor */
    lux = OPT3001_getLux();

    while (1){
        /* Update the counter on the LCD screen */
        sprintf(string_n, " %d ", counter);
        Graphics_drawStringCentered(&g_sContext_n, (int8_t *) string_n, AUTO_STRING_LENGTH, 65, 80, OPAQUE_TEXT);

        lux2 = OPT3001_getLux();    // Obtaining the current light value

        /* Control all the functionalities */
        if (lux - lux2 > 10) {         // Check if a car is waiting to enter

            /* CS that needs to be run only one time per cycle */
            /* Opening phase */
            if(!isWaiting){
                isWaiting=true;
                counter--;              // Decreasing counter of available parking spaces

                print_lastLine("OPENING...");
                LED_set(WHITE);
                buzzer_servo_set(ON);


                print_lastLine("   OPEN!   ");
                LED_set(GREEN);
                buzzer_servo_set(OFF);
            }
        } else {
            /* Closing phase */
            if(isWaiting){
                isWaiting=false;

                LED_set(WHITE);
                print_lastLine("CLOSING...");
                buzzer_servo_set(ON);

                print_lastLine("   CLOSED!   ");
                LED_set(RED);
                buzzer_servo_set(OFF);

                print_lastLine("           ");      // Blank spaces to clear the screen

                /* When the park is full, exit from while */
                if(!counter){
                    break;
                }
            }
        }
    }

    /* Final state (park is full) */
    while(1){
        print_lastLine("FULL PARK!");
        buzzer_servo_set(OFF);
        LED_set(RED);
    }
}
