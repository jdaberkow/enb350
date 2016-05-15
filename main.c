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
#include <ti/sysbios/knl/Mailbox.h>

#define ADC_SEQ_NUM	0

#define STARTING_TIME 1462372289

//Drawing defines
#define SPACING_X	38

#define LOC_X_0	27
#define LOC_X_1	LOC_X_0+SPACING_X
#define LOC_X_2	LOC_X_1+SPACING_X
#define LOC_X_3	LOC_X_2+SPACING_X
#define LOC_X_4	LOC_X_3+SPACING_X
#define LOC_X_5	LOC_X_4+SPACING_X
#define LOC_X_6	LOC_X_5+SPACING_X
#define LOC_X_7	LOC_X_6+SPACING_X

#define LOC_Y_0UTPUTS	 80
#define LOC_Y_INPUTS	160
#define LOC_Y_ANALOG	220

#define INPUT_STATUS_IS_ONE	1


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
Mailbox_Handle mbx;

uint32_t input_status[8];

uint32_t adc0_read;
uint32_t num_analog_pixels = 0;

tContext 	sContext;
uint32_t 	ui32SysClock;
uint32_t 	i;
uint32_t 	reg_read;

typedef enum{
	INIT,
	INIT_STATION,
	CHECK_WORKPIECE,
	CKECK_SAFETY_BARRIER,
	MEASURE_WORKPIECE,
	ACCEPT_WORKPIECE,
	REJECT_WORKPIECE,
	CALIBRATE_SENSOR
} state_type;

state_type currentState = INIT;
struct ColorMaterial latestColorMaterial;
float latestHeight = 0.0;

float slope = 0.1091107474;
float offset = 29.00981997;

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
 *  ======== taskUpdateScreen ========
 */
Void taskUpdateScreen(UArg a0, UArg a1)
{
	//
	// Loop forever, processing events.
	//
	UInt posted;

	while(1)
	{
		posted = Event_pend(evt,
			Event_Id_NONE, /* andMask */
			Event_Id_00,   /* orMask */
			BIOS_WAIT_FOREVER);
		if (posted & Event_Id_00) {
			festoData.theTime = time(NULL);
			festoData.operatingTime = time(NULL) - STARTING_TIME + 1;
			updateScreen();

		}
		// Just for debugging purposes to get the height values

		//Read the ADC0
		/*
		adc0_read = QUT_ADC0_Read();

		QUT_UART_Send( (uint8_t *)"\n\radc0_read=", 12 );
		QUT_UART_Send_uint32_t( adc0_read );
*/

		Task_sleep(10);
	}

}

bool checkWorkpiece() {
	if (latestColorMaterial.black) {
		festoData.currentColor = BLACK;
		festoData.countBlack++;
	} else {
		festoData.currentColor = NON_BLACK;
	}
	if (latestColorMaterial.metallic) {
		festoData.currentMaterial = METALLIC;
		festoData.countMetallic++;
	} else {
		festoData.currentMaterial = NON_METALLIC;
	}
	festoData.currentHeight = latestHeight;
	festoData.countTotal++;

	if (festoData.thresholdBottom <= latestHeight && festoData.thresholdTop >= latestHeight) {
		festoData.countAccepted++;
		return true;
	}
	return false;
}

bool calibrateSensor() {
	uint32_t height[2];
	uint32_t heightReference[2];
	for (int i = 0; i < 2; i++) {
		movePlatform(false, true);

		UInt posted;
		posted = Event_pend(evt,
					Event_Id_NONE, 				/* andMask */
					Event_Id_01,  				/* orMask */
					BIOS_WAIT_FOREVER);
		while (!(posted & Event_Id_01)) {
			Task_sleep(5);
		}
		while (!senseWorkpiece()) {
			Task_sleep(5);
		}

		height[i] = getRawWorkpieceHeight();
		movePlatform(true, true);
		while (!Mailbox_pend(mbx, &heightReference[i], BIOS_NO_WAIT)) {
			Task_sleep(5);
		}
	}

	float smaller;
	float bigger;
	float smallerReference;
	float biggerReference;
	if (heightReference[0] < heightReference[1]) {
		smallerReference = heightReference[0];
		biggerReference = heightReference[1];
		smaller = height[0];
		bigger = height[1];
	} else {
		smallerReference = heightReference[1];
		biggerReference = heightReference[0];
		smaller = height[1];
		bigger = height[0];
	}

	slope = ((float)(biggerReference - smallerReference)) / ((float)(bigger - smaller));
	offset = ((float)smallerReference) - (slope * ((float)smaller));
}

/*
 *  ======== taskStateMachine ========
 */
Void taskStateMachine(UArg a0, UArg a1) {
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
					Task_sleep(500);
					latestHeight = getWorkpieceHeight(slope, offset);
					if (checkWorkpiece()) {
						currentState = ACCEPT_WORKPIECE;
					} else {
						currentState = REJECT_WORKPIECE;
					}
				} else {
					currentState = INIT_STATION;
				}
				break;
			case ACCEPT_WORKPIECE:
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
			case REJECT_WORKPIECE:
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
			case CALIBRATE_SENSOR:

				break;
			default:
			    System_printf("Case Default\n");
				break;
		}
		UInt posted;
		posted = Event_pend(evt,
					Event_Id_NONE, 				/* andMask */
					Event_Id_02,  				/* orMask */
					BIOS_NO_WAIT);
		if (posted & Event_Id_02) currentState = CALIBRATE_SENSOR;
		Task_sleep(10);
	}
}

/*
 *  ======== main ========
 */
Int main()
{ 
    Task_Handle task;
    Task_Handle taskHandleStateMachine;
    Error_Block eb;

    System_printf("enter main()\n");

    Error_init(&eb);
    task = Task_create(taskUpdateScreen, NULL, &eb);
    if (task == NULL) {
        System_printf("Task_create() failed!\n");
        BIOS_exit(0);
    }

    taskHandleStateMachine = Task_create(taskStateMachine, NULL, &eb);
	if (taskHandleStateMachine == NULL) {
		System_printf("Task_create() failed!\n");
		BIOS_exit(0);
	}

	Mailbox_Params mbxParams;
	/* create a Mailbox Instance */
	Mailbox_Params_init(&mbxParams);
	mbxParams.readerEvent = evt;
	mbxParams.readerEventId = Event_Id_01;
	mbx = Mailbox_create((sizeof(uint32_t)), 1, &mbxParams, NULL);


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
	festoData.screenState     = STATUS;
	festoData.currentMaterial = UNKNOWN_MATERIAL;
	festoData.currentColor    = UNKNOWN_COLOR;
	festoData.currentHeight   = 0;
	festoData.countTotal      = 0;
	festoData.countAccepted   = 0;
	festoData.countMetallic   = 0;
	festoData.countBlack      = 0;
	festoData.thresholdTop    = 260;
	festoData.thresholdBottom = 240;

	//Initialize the screen
	initScreen(&festoData);

    BIOS_start();    /* does not return */
    return(0);
}
