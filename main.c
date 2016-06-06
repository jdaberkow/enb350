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

uint32_t ticksUpButton = 0;
uint32_t ticksDownButton = 0;
uint32_t ticksSelectButton = 0;

typedef enum{
	INIT,
	INIT_STATION,
	CHECK_WORKPIECE,
	CHECK_SAFETY_BARRIER,
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

festoData_type festoDataObject;

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
			BIOS_NO_WAIT);
		if (posted & Event_Id_00) {
			festoDataObject.theTime = time(NULL);
			festoDataObject.operatingTime = time(NULL) - STARTING_TIME + 1;
			Event_post(evt, Event_Id_06);
		}
		if (posted & Event_Id_01) { // UP BUTTON
			if (festoDataObject.screenState == STATUS_SCREEN) {
				festoDataObject.screenState = CALIBRATE;
			}
			else if (festoDataObject.screenState == CALIBRATE) {
				switch (festoDataObject.calibrateState) {
					case REVIEW_C:
						if (!festoDataObject.measuring) {
							festoDataObject.screenState = STATUS_SCREEN;
						}
						break;
					case FIRST_VALUE:
						if (festoDataObject.calibrateFirst <= 495)
							festoDataObject.calibrateFirst += 5;
						break;
					case SECOND_VALUE:
						if (festoDataObject.calibrateSecond <= 495)
							festoDataObject.calibrateSecond += 5;
						break;
					default:
						break;
				}
			}
			else if (festoDataObject.screenState == THRESHOLD) {
				switch (festoDataObject.thresholdState) {
					case REVIEW_T:
						festoDataObject.screenState = CALIBRATE;
						break;
					case ADJUST_BOTTOM:
						if (festoDataObject.thresholdBottom <= 495)
							festoDataObject.thresholdBottom += 5;
						break;
					case ADJUST_TOP:
						if (festoDataObject.thresholdTop <= 495)
							festoDataObject.thresholdTop += 5;
						break;
					default:
						break;
				}
			}
			Event_post(evt, Event_Id_06);
		}
		if (posted & Event_Id_02) { // DOWN BUTTON
			if (festoDataObject.screenState == STATUS_SCREEN) {
				festoDataObject.screenState = THRESHOLD;
			}
			else if (festoDataObject.screenState == CALIBRATE) {
				switch (festoDataObject.calibrateState) {
					case REVIEW_C:
						if (!festoDataObject.measuring) {
							festoDataObject.screenState = THRESHOLD;
						}
						break;
					case FIRST_VALUE:
						if (festoDataObject.calibrateFirst >= 5)
							festoDataObject.calibrateFirst -= 5;
						break;
					case SECOND_VALUE:
						if (festoDataObject.calibrateSecond >= 5)
							festoDataObject.calibrateSecond -= 5;
						break;
					default:
						break;
				}
			}
			else if (festoDataObject.screenState == THRESHOLD){
				switch (festoDataObject.thresholdState) {
					case REVIEW_T:
						festoDataObject.screenState = STATUS_SCREEN;
						break;
					case ADJUST_BOTTOM:
						if (festoDataObject.thresholdBottom >= 5)
							festoDataObject.thresholdBottom -= 5;
						break;
					case ADJUST_TOP:
						if (festoDataObject.thresholdTop >= 5)
							festoDataObject.thresholdTop -= 5;
						break;
					default:
						break;
				}
			}
			Event_post(evt, Event_Id_06);
		}
		if (posted & Event_Id_03) { // SELECT BUTTON
			switch (festoDataObject.screenState) {
				case STATUS_SCREEN:
					festoDataObject.enableMovement = toggleEnableMovement();
					break;
				case CALIBRATE:
					switch (festoDataObject.calibrateState) {
						case REVIEW_C:
							if (!festoDataObject.measuring) {
								Event_post(evt, Event_Id_04);
								festoDataObject.calibrateState = FIRST_VALUE;
							}
							break;
						case FIRST_VALUE:
							if (!festoDataObject.measuring) {
								festoDataObject.measuring = true;
								Event_post(evt, Event_Id_05);
								festoDataObject.calibrateState = SECOND_VALUE;
							}
							break;
						case SECOND_VALUE:
							if (!festoDataObject.measuring) {
								festoDataObject.measuring = true;
								Event_post(evt, Event_Id_05);
								festoDataObject.calibrateState = REVIEW_C;
							}
							break;
						default:
							break;
					}
					break;
				case THRESHOLD:
					switch (festoDataObject.thresholdState) {
						case REVIEW_T:
							festoDataObject.thresholdState = ADJUST_BOTTOM;
							enableMovement(false);
							festoDataObject.enableMovement = false;
							break;
						case ADJUST_BOTTOM:
							festoDataObject.thresholdState = ADJUST_TOP;
							break;
						case ADJUST_TOP:
							festoDataObject.thresholdState = REVIEW_T;
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
			Event_post(evt, Event_Id_06);
		}
		Task_sleep(10);
	}

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
			Event_Id_06,   /* orMask */
			BIOS_NO_WAIT);
		if (posted & Event_Id_06) {
			updateScreen();
			updateLED();
		}

		Task_sleep(10);
	}

}

bool checkWorkpiece() {
	if (latestColorMaterial.black) {
		festoDataObject.currentColor = BLACK;
		festoDataObject.countBlack++;
	} else {
		festoDataObject.currentColor = NON_BLACK;
	}
	if (latestColorMaterial.metallic) {
		festoDataObject.currentMaterial = METALLIC;
		festoDataObject.countMetallic++;
	} else {
		festoDataObject.currentMaterial = NON_METALLIC;
	}
	festoDataObject.currentHeight = latestHeight;
	festoDataObject.countTotal++;

	if (festoDataObject.thresholdBottom <= latestHeight && festoDataObject.thresholdTop >= latestHeight) {
		festoDataObject.countAccepted++;
		return true;
	}
	return false;
}

void calibrateSensor() {
	uint32_t height[2];
	uint32_t heightReference[2];
	enableMovement(true);
	festoDataObject.enableMovement = true;
	for (int i = 0; i < 2; i++) {
		festoDataObject.measuring = true;
		movePlatform(false, true);
		festoDataObject.measuring = false;

		UInt posted = 0;
		while (!(posted & Event_Id_05)) {
			posted = Event_pend(evt,
					Event_Id_NONE, 				/* andMask */
					Event_Id_05,  				/* orMask */
					BIOS_NO_WAIT);
			Task_sleep(5);
		}
		while (!senseWorkpiece()) {
			Task_sleep(5);
		}

		movePlatform(true, true);
		Task_sleep(500);
		height[i] = getRawWorkpieceHeight();
		if (i == 0) {
			heightReference[i] = festoDataObject.calibrateFirst;
		} else {
			heightReference[i] = festoDataObject.calibrateSecond;
		}
		festoDataObject.measuring = false;
	}
	movePlatform(false, true);

	float smaller;
	float bigger;
	float smallerReference;
	float biggerReference;
	if (heightReference[0] < heightReference[1]) {
		smallerReference = heightReference[0];
		biggerReference = heightReference[1];
		smaller = height[0];
		bigger = height[1];
	} else if (heightReference[0] > heightReference[1]){
		smallerReference = heightReference[1];
		biggerReference = heightReference[0];
		smaller = height[1];
		bigger = height[0];
	} else return;

	slope = (biggerReference - smallerReference) / (bigger - smaller);
	offset = smallerReference - (slope * smaller);
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
					currentState = CHECK_SAFETY_BARRIER;
				}
				break;
			case CHECK_SAFETY_BARRIER:
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
				System_printf("Case ACCEPT\n");
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
				System_printf("Case REJECT\n");
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
				calibrateSensor();
				enableMovement(false);
				festoDataObject.enableMovement = false;
				currentState = INIT_STATION;
				break;
			default:
			    System_printf("Case Default\n");
				break;
		}
		UInt posted;
		posted = Event_pend(evt,
					Event_Id_NONE, 				/* andMask */
					Event_Id_04,  				/* orMask */
					BIOS_NO_WAIT);
		if (posted & Event_Id_04) currentState = CALIBRATE_SENSOR;
		Task_sleep(10);
	}
}

void IntPortN(void)
{
	if (GPIOIntStatus(GPIO_PORTN_BASE, false) & GPIO_PIN_3)
	{
		// Up Button
		GPIOIntClear(GPIO_PORTN_BASE, GPIO_INT_PIN_3);

		uint32_t currentTicks = Clock_getTicks();

		if (currentTicks - ticksUpButton > 200) {
			ticksUpButton = currentTicks;
			Event_post(evt, Event_Id_01);
		}
	}
	if (GPIOIntStatus(GPIO_PORTN_BASE, false) & GPIO_INT_PIN_4)
	{
		// Platform Raised
		GPIOIntClear(GPIO_PORTN_BASE, GPIO_INT_PIN_4);
		qut_set_gpio(1, 0);
		Event_post(evt, Event_Id_07);
	}
}

void IntDownButton(void)
{
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_5);

	uint32_t currentTicks = Clock_getTicks();

	if (currentTicks - ticksDownButton > 200) {
		ticksDownButton = currentTicks;
		Event_post(evt, Event_Id_02);
	}
}

void IntSelectButton(void)
{
	GPIOIntClear(GPIO_PORTP_BASE, GPIO_INT_PIN_1);

	uint32_t currentTicks = Clock_getTicks();

	if (currentTicks - ticksSelectButton > 200) {
		ticksSelectButton = currentTicks;
		Event_post(evt, Event_Id_03);
	}
}

void IntLoweredPositionSensor(void)
{
	GPIOIntClear(GPIO_PORTA_BASE, GPIO_INT_PIN_7);
	qut_set_gpio(0, 0);
	Event_post(evt, Event_Id_08);
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

	Mailbox_Params mbxParams;
	/* create a Mailbox Instance */
	Mailbox_Params_init(&mbxParams);
	mbxParams.readerEvent = evt;
	mbxParams.readerEventId = Event_Id_05;
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

	GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_5);
	GPIOIntEnable(GPIO_PORTN_BASE, GPIO_INT_PIN_3);
	GPIOIntEnable(GPIO_PORTP_BASE, GPIO_INT_PIN_1);
	GPIOIntEnable(GPIO_PORTN_BASE, GPIO_INT_PIN_4);
	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_INT_PIN_7);

	GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_INT_PIN_5, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTN_BASE, GPIO_INT_PIN_3, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTP_BASE, GPIO_INT_PIN_1, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTN_BASE, GPIO_INT_PIN_4, GPIO_RISING_EDGE);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_INT_PIN_7, GPIO_RISING_EDGE);

	GPIOIntRegister(GPIO_PORTE_BASE, IntDownButton);
	GPIOIntRegister(GPIO_PORTN_BASE, IntPortN);
	GPIOIntRegister(GPIO_PORTP_BASE, IntSelectButton);
	GPIOIntRegister(GPIO_PORTA_BASE, IntLoweredPositionSensor);
	/* TODO:
	 * Add Interrupt Registrations for the Sensor Interrupts.
	 * 1) Up Sensor
	 * 2) Down Sensor
	 * In the ISRs stop movement of the platform immediately.
	 * And send an event which can be received by the driverlib.
	 * Maybe also introduce bool values to indicate platform position.
	 */

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

	// Initialize the festoDataObject struct
	festoDataObject.theTime         = time(NULL);
	festoDataObject.operatingTime   = 0;
	festoDataObject.screenState     = STATUS_SCREEN;
	festoDataObject.thresholdState  = REVIEW_T;
	festoDataObject.calibrateState  = REVIEW_C;
	festoDataObject.currentMaterial = UNKNOWN_MATERIAL;
	festoDataObject.currentColor    = UNKNOWN_COLOR;
	festoDataObject.currentHeight   = 0;
	festoDataObject.countTotal      = 0;
	festoDataObject.countAccepted   = 0;
	festoDataObject.countMetallic   = 0;
	festoDataObject.countBlack      = 0;
	festoDataObject.thresholdTop    = 260;
	festoDataObject.thresholdBottom = 240;
	festoDataObject.calibrateFirst  = 225;
	festoDataObject.calibrateSecond = 275;
	festoDataObject.enableMovement  = false;
	festoDataObject.measuring       = false;

	//Initialize the screen
	initScreen(&festoDataObject);

    BIOS_start();    /* does not return */
    return(0);
}
