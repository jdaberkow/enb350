/*
 *  ======== main.c ========
 */

#include <xdc/std.h>

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/hal/Seconds.h>

#define ADC_SEQ_NUM	0

#define STARTING_TIME 1462372289

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/tm4c129xnczad.h"

#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

#include "utils/uartstdio.h"

#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"

#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"

#include "driverLib.h"
#include <screen.h>

Void timeFxn(UArg arg0);

Event_Handle evt;
Clock_Handle clk1;

uint32_t input_status[8];

uint32_t adc0_read;
uint32_t num_analog_pixels = 0;

tContext 	sContext;
uint32_t 	ui32SysClock;
uint32_t 	i;
uint32_t 	reg_read;

uint32_t ticksUpButton = 0;
uint32_t ticksDownButton = 0;
uint32_t ticksSelectButton = 0;

typedef enum{
	INIT,
	INIT_STATION,
	CHECK_WORKPIECE,
	CKECK_SAFETY_BARRIER,
	MEASURE_WORKPIECE,
	ACCEPT_WORKPIECE,
	REJECT_WORKPIECE
} state_type;

state_type currentState = INIT;
struct ColorMaterial latestColorMaterial;
float latestHeight = 0.0;

festoData_type festoData;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

/*
 *  ======== timeFxn ========
 */
Void timeFxn(UArg arg0)
{
	/* Explicit posting of Event_Id_00 by calling Event_post() */
	Event_post(evt, Event_Id_00);
}


/*
 *  ======== taskEventHandler ========
 */
Void taskEventHandler(UArg a0, UArg a1)
{
	//
	// Loop forever, processing events.
	//
	UInt posted;

	while(1)
	{
		posted = Event_pend(evt,
			Event_Id_NONE, /* andMask */
			Event_Id_00 + Event_Id_01 + Event_Id_02 + Event_Id_03,   /* orMask */
			BIOS_WAIT_FOREVER);
		if (posted & Event_Id_00) {
			festoData.theTime = time(NULL);
			festoData.operatingTime = time(NULL) - STARTING_TIME + 1;
			Event_post(evt, Event_Id_06);
		}
		if (posted & Event_Id_01) {
			if (festoData.screenState == STATUS_SCREEN) {
				festoData.screenState = CALIBRATE;
			}
			else if (festoData.screenState == CALIBRATE) {
				festoData.screenState = STATUS_SCREEN;
			}
			else if (festoData.screenState == THRESHOLD) {
				festoData.screenState = CALIBRATE;
			}
		}
		if (posted & Event_Id_02) {
			if (festoData.screenState == STATUS_SCREEN) {
				festoData.screenState = THRESHOLD;
			}
			else if (festoData.screenState == CALIBRATE) {
				festoData.screenState = THRESHOLD;
			}
			else if (festoData.screenState == THRESHOLD){
				festoData.screenState = STATUS_SCREEN;
			}
		}
		if (posted & Event_Id_03) {
			toggleEnableMovement();
		}

		Task_sleep(10);
	}

}

/*
 *  ======== taskStateMachine ========
 */
Void taskStateMachine(UArg a0, UArg a1)
{
	while (true) {
		switch (currentState) {
			case INIT:
			    System_printf("Case INIT\n");
				if (init()) {
					currentState = INIT_STATION;
				} else {
					System_printf("Initiation failed!\n");
					BIOS_exit(0);
				}
				break;
			case INIT_STATION:
			    System_printf("Case INIT STATION\n");
				if (initStation()) {
					currentState = CHECK_WORKPIECE;
				}
				break;
			case CHECK_WORKPIECE:
			    System_printf("Case WORKPIECE\n");
				if (senseWorkpiece()) {
					currentState = CKECK_SAFETY_BARRIER;
				}
				break;
			case CKECK_SAFETY_BARRIER:
			    System_printf("Case SAFTEY BARRIER\n");
				if (senseSafetyBarrierClear()) {
					currentState = MEASURE_WORKPIECE;
				}
				break;
			case MEASURE_WORKPIECE:
			    System_printf("Case MEASURE\n");
				latestColorMaterial = getMaterial();
				if (movePlatform(true, true)) {
					latestHeight = getRawWorkpieceHeight();
					if (false) { //TODO: decide if accepted or not
						currentState = ACCEPT_WORKPIECE;
					} else {
						currentState = REJECT_WORKPIECE;
					}
				} else {
					currentState = INIT_STATION;
				}
				break;
			case ACCEPT_WORKPIECE:
			    System_printf("Case ACCEPT\n");
				if (movePlatform(false, true)) {
					if (controlEjector(true, true)) {
						if (controlEjector(false, true)) {
							currentState = CHECK_WORKPIECE;
							break;
						}
					}
				}
				currentState = INIT_STATION;
				break;
			case REJECT_WORKPIECE:
			    System_printf("Case REJECT\n");
				controlAirSlider(true);
				if (controlEjector(true, true)) {
					if (controlEjector(false, true)) {
						if (movePlatform(false,true)) {
							controlAirSlider(false);
							currentState = CHECK_WORKPIECE;
							break;
						}
					}
				}
				controlAirSlider(false);
				currentState = INIT_STATION;
				break;
			default:
			    System_printf("Case Default\n");
				break;
		}
		Task_sleep(10);
	}
}

void IntUpButton(void)
{
	GPIOIntClear(GPIO_PORTN_BASE, GPIO_INT_PIN_3);

	/* Explicit posting of Event_Id_00 by calling Event_post() */
	Event_post(evt, Event_Id_01);

}

void IntDownButton(void)
{
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_5);

	/* Explicit posting of Event_Id_00 by calling Event_post() */
	Event_post(evt, Event_Id_02);
}

void IntSelectButton(void)
{
	GPIOIntClear(GPIO_PORTP_BASE, GPIO_INT_PIN_1);

	/* Explicit posting of Event_Id_00 by calling Event_post() */
	Event_post(evt, Event_Id_03);
}

/*
 *  ======== main ========
 */
Int main()
{ 
    Task_Handle task;
    Task_Handle taskHandleStateMachine;
    Task_Handle taskHandleUpdateScreen;
    Error_Block eb;

    System_printf("enter main()\n");

    Error_init(&eb);
    task = Task_create(taskEventHandler, NULL, &eb);
    if (task == NULL) {
        System_printf("Task_create() failed!\n");
        BIOS_exit(0);
    }

    taskHandleStateMachine = Task_create(taskStateMachine, NULL, &eb);
	if (taskHandleStateMachine == NULL) {
		System_printf("Task_create() failed!\n");
		BIOS_exit(0);
	}

	taskHandleUpdateScreen = Task_create(taskUpdateScreen, NULL, &eb);
		if (taskHandleUpdateScreen == NULL) {
			System_printf("Task_create() failed!\n");
			BIOS_exit(0);
		}


    ui32SysClock = 120000000;


	//
	// Configure the device pins.
	//
	PinoutSet();

	GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_1);	//OUT 0 L1
	GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_0);	//OUT 1 L0
	GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_2);	//OUT 2 L2
	GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_3);	//OUT 3 L3
	GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_4);	//OUT 4 L4
	GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_5);	//OUT 5 L5
	GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_5);	//OUT 6 P5
	GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_4);	//OUT 7 P4

	GPIOPinTypeGPIOInput (GPIO_PORTM_BASE, GPIO_PIN_3);	//IN  0 M3
	GPIOPinTypeGPIOInput (GPIO_PORTM_BASE, GPIO_PIN_2);	//IN  1 M2
	GPIOPinTypeGPIOInput (GPIO_PORTM_BASE, GPIO_PIN_1);	//IN  2 M1
	GPIOPinTypeGPIOInput (GPIO_PORTM_BASE, GPIO_PIN_0);	//IN  3 M0
	GPIOPinTypeGPIOInput (GPIO_PORTN_BASE, GPIO_PIN_4);	//IN  4 N4
	GPIOPinTypeGPIOInput (GPIO_PORTA_BASE, GPIO_PIN_7);	//IN  5 A7
	GPIOPinTypeGPIOInput (GPIO_PORTA_BASE, GPIO_PIN_6);	//IN  6 C6
	GPIOPinTypeGPIOInput (GPIO_PORTC_BASE, GPIO_PIN_5);	//IN  7 C5

	//RGB LED
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_5);	//RED 	N5
	GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_7);	//GREEN Q7
	GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_4);	//BLUE 	Q4

	GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_5);
	GPIOIntEnable(GPIO_PORTN_BASE, GPIO_INT_PIN_3);
	GPIOIntEnable(GPIO_PORTP_BASE, GPIO_INT_PIN_1);

	GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_INT_PIN_5, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTN_BASE, GPIO_INT_PIN_3, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTP_BASE, GPIO_INT_PIN_1, GPIO_FALLING_EDGE);

	GPIOIntRegister(GPIO_PORTE_BASE, IntDownButton);
	GPIOIntRegister(GPIO_PORTN_BASE, IntUpButton);
	GPIOIntRegister(GPIO_PORTP_BASE, IntSelectButton);

	//Initialize the UART
	QUT_UART_Init( ui32SysClock );


	//Initialize AIN0
	QUT_ADC0_Init();
	QUT_UART_Send( (uint8_t *)"FestoTester/n", 11 );

	///* Create a one-shot Clock Instance with timeout = 120000000 system time units
	Clock_Params clkParams;
	Clock_Params_init(&clkParams);
	clkParams.startFlag = TRUE;
	clkParams.period = 1000;
	clk1 = Clock_create(timeFxn, 5, &clkParams, NULL);

	//Initialize the time
	Seconds_set(STARTING_TIME);

	// Initialize an Event Instance
	evt = Event_create(NULL, NULL);

	// Initialize the festoData struct
	festoData.theTime         = time(NULL);
	festoData.operatingTime   = 0;
	festoData.screenState     = STATUS_SCREEN;
	festoData.currentMaterial = UNKNOWN_MATERIAL;
	festoData.currentColor    = UNKNOWN_COLOR;
	festoData.currentHeight   = 0;
	festoData.countTotal      = 0;
	festoData.countAccepted   = 0;
	festoData.countMetallic   = 0;
	festoData.countBlack      = 0;
	festoData.thresholdTop    = 30;
	festoData.thresholdBottom = 20;

	//Initialize the screen
	initScreen(&festoData);

    BIOS_start();    /* does not return */
    return(0);
}
