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

#include "qut_tiva.h"
#include "screen.h"

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

festoData_type festoData;

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tPushButtonWidget g_sPushBtn_0;
extern tPushButtonWidget g_sPushBtn_1;
extern tPushButtonWidget g_sPushBtn_2;
extern tPushButtonWidget g_sPushBtn_3;
extern tPushButtonWidget g_sPushBtn_4;
extern tPushButtonWidget g_sPushBtn_5;
extern tPushButtonWidget g_sPushBtn_6;
extern tPushButtonWidget g_sPushBtn_7;

//*****************************************************************************
//
// Forward declarations for the globals required to define the widgets at
// compile-time.
//
//*****************************************************************************
void OnIntroPaint(tWidget *psWidget, tContext *psContext);

//*****************************************************************************
//
// Forward reference to the button press handler.
//
//*****************************************************************************
void OnButtonPress_0	(tWidget *psWidget);
void OnButtonPress_1	(tWidget *psWidget);
void OnButtonPress_2	(tWidget *psWidget);
void OnButtonPress_3	(tWidget *psWidget);
void OnButtonPress_4	(tWidget *psWidget);
void OnButtonPress_5	(tWidget *psWidget);
void OnButtonPress_6	(tWidget *psWidget);
void OnButtonPress_7	(tWidget *psWidget);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, 0,//&g_sPushBtn_0,
       &g_sKentec320x240x16_SSD2119, 10, 25, 300, (240 - 25 -10),
       CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnIntroPaint);


//*****************************************************************************
//
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void
OnIntroPaint(tWidget *psWidget, tContext *psContext)
{
	tRectangle sRect;

	//
    // Display the introduction text in the canvas.
    //
    GrContextFontSet		( psContext, g_psFontCm16);
    GrContextForegroundSet	( psContext, ClrSilver);

    GrStringDraw(psContext, "OUTPUTS", 	-1, 125, LOC_Y_0UTPUTS-50, 0);
    GrStringDraw(psContext, "0", 		-1, LOC_X_0-4, LOC_Y_0UTPUTS-35, 0);
    GrStringDraw(psContext, "1", 		-1, LOC_X_1-4, LOC_Y_0UTPUTS-35, 0);
    GrStringDraw(psContext, "2", 		-1, LOC_X_2-4, LOC_Y_0UTPUTS-35, 0);
    GrStringDraw(psContext, "3", 		-1, LOC_X_3-4, LOC_Y_0UTPUTS-35, 0);
    GrStringDraw(psContext, "4", 		-1, LOC_X_4-4, LOC_Y_0UTPUTS-35, 0);
    GrStringDraw(psContext, "5", 		-1, LOC_X_5-4, LOC_Y_0UTPUTS-35, 0);
    GrStringDraw(psContext, "6", 		-1, LOC_X_6-4, LOC_Y_0UTPUTS-35, 0);
    GrStringDraw(psContext, "7", 		-1, LOC_X_7-4, LOC_Y_0UTPUTS-35, 0);

    WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sPushBtn_0);
    WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sPushBtn_1);
    WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sPushBtn_2);
    WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sPushBtn_3);
    WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sPushBtn_4);
    WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sPushBtn_5);
    WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sPushBtn_6);
    WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sPushBtn_7);


	GrStringDraw(psContext, "INPUTS", 	-1, 130, LOC_Y_INPUTS-50, 0);
    GrStringDraw(psContext, "0", 		-1, LOC_X_0-4, LOC_Y_INPUTS-35, 0);
    GrStringDraw(psContext, "1", 		-1, LOC_X_1-4, LOC_Y_INPUTS-35, 0);
    GrStringDraw(psContext, "2", 		-1, LOC_X_2-4, LOC_Y_INPUTS-35, 0);
    GrStringDraw(psContext, "3", 		-1, LOC_X_3-4, LOC_Y_INPUTS-35, 0);
    GrStringDraw(psContext, "4", 		-1, LOC_X_4-4, LOC_Y_INPUTS-35, 0);
    GrStringDraw(psContext, "5", 		-1, LOC_X_5-4, LOC_Y_INPUTS-35, 0);
    GrStringDraw(psContext, "6", 		-1, LOC_X_6-4, LOC_Y_INPUTS-35, 0);
    GrStringDraw(psContext, "7", 		-1, LOC_X_7-4, LOC_Y_INPUTS-35, 0);

	//Draw all INPUT status circles
	//TBD Cant get a loop to work for this for some reason
	GrContextForegroundSet( psContext, ClrCyan);
    if ( input_status[0] == INPUT_STATUS_IS_ONE ){
    	GrContextForegroundSet( psContext, ClrRed);
    }
	GrCircleFill( psContext, LOC_X_0, LOC_Y_INPUTS, 15);

	GrContextForegroundSet( psContext, ClrCyan);
    if ( input_status[1] == INPUT_STATUS_IS_ONE ){
    	GrContextForegroundSet( psContext, ClrRed);
    }
	GrCircleFill( psContext, LOC_X_1, LOC_Y_INPUTS, 15);

	GrContextForegroundSet( psContext, ClrCyan);
    if ( input_status[2] == INPUT_STATUS_IS_ONE ){
    	GrContextForegroundSet( psContext, ClrRed);
    }
	GrCircleFill( psContext, LOC_X_2, LOC_Y_INPUTS, 15);

	GrContextForegroundSet( psContext, ClrCyan);
    if ( input_status[3] == INPUT_STATUS_IS_ONE ){
    	GrContextForegroundSet( psContext, ClrRed);
    }
	GrCircleFill( psContext, LOC_X_3, LOC_Y_INPUTS, 15);

	GrContextForegroundSet( psContext, ClrCyan);
    if ( input_status[4] == INPUT_STATUS_IS_ONE ){
    	GrContextForegroundSet( psContext, ClrRed);
    }
	GrCircleFill( psContext, LOC_X_4, LOC_Y_INPUTS, 15);

	GrContextForegroundSet( psContext, ClrCyan);
    if ( input_status[5] == INPUT_STATUS_IS_ONE ){
    	GrContextForegroundSet( psContext, ClrRed);
    }
	GrCircleFill( psContext, LOC_X_5, LOC_Y_INPUTS, 15);

	GrContextForegroundSet( psContext, ClrCyan);
    if ( input_status[6] == INPUT_STATUS_IS_ONE ){
    	GrContextForegroundSet( psContext, ClrRed);
    }
	GrCircleFill( psContext, LOC_X_6, LOC_Y_INPUTS, 15);

	GrContextForegroundSet( psContext, ClrCyan);
    if ( input_status[7] == INPUT_STATUS_IS_ONE ){
    	GrContextForegroundSet( psContext, ClrRed);
    }
	GrCircleFill( psContext, LOC_X_7, LOC_Y_INPUTS, 15);


	GrContextForegroundSet	( psContext, ClrSilver);
	GrStringDraw(psContext, "ANALOG", 	-1, 130, LOC_Y_ANALOG-30, 0);

	//Draw two rectangles, one filled left to right up to the analog value then an empty one to contain the information
    GrContextForegroundSet(psContext, ClrMidnightBlue);
    sRect.i16XMin = 20;
	sRect.i16XMax = 300;
	sRect.i16YMin = LOC_Y_ANALOG-10;
	sRect.i16YMax = LOC_Y_ANALOG+8;
    GrRectFill(psContext, &sRect);

    GrContextForegroundSet(psContext, ClrCyan);
	sRect.i16XMin = 20;
    sRect.i16XMax = num_analog_pixels + sRect.i16XMin;
	sRect.i16YMin = LOC_Y_ANALOG-10;
	sRect.i16YMax = LOC_Y_ANALOG+8;
    GrRectFill(psContext, &sRect);





}

//*****************************************************************************
//
// Define the buttons that trigger the OUTPUTS
//
//*****************************************************************************
CircularButton	(	g_sPushBtn_0, &g_sBackground, 0, 0, &g_sKentec320x240x16_SSD2119,
					LOC_X_0, LOC_Y_0UTPUTS, 15,																//x,y,radius
					(PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
					PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
					ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
					g_psFontCmss22b, "0", 0, 0, 0, 0, OnButtonPress_0 );

CircularButton	(	g_sPushBtn_1, &g_sBackground, 0, 0, &g_sKentec320x240x16_SSD2119,
					LOC_X_1, LOC_Y_0UTPUTS, 15,																//x,y,radius
					(PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
					PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
					ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
					g_psFontCmss22b, "0", 0, 0, 0, 0, OnButtonPress_1 );

CircularButton	(	g_sPushBtn_2, &g_sBackground, 0, 0, &g_sKentec320x240x16_SSD2119,
					LOC_X_2, LOC_Y_0UTPUTS, 15,															//x,y,radius
					(PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
					PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
					ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
					g_psFontCmss22b, "0", 0, 0, 0, 0, OnButtonPress_2 );

CircularButton	(	g_sPushBtn_3, &g_sBackground, 0, 0, &g_sKentec320x240x16_SSD2119,
					LOC_X_3, LOC_Y_0UTPUTS, 15,															//x,y,radius
					(PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
					PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
					ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
					g_psFontCmss22b, "0", 0, 0, 0, 0, OnButtonPress_3 );

CircularButton	(	g_sPushBtn_4, &g_sBackground, 0, 0, &g_sKentec320x240x16_SSD2119,
					LOC_X_4, LOC_Y_0UTPUTS, 15,															//x,y,radius
					(PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
					PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
					ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
					g_psFontCmss22b, "0", 0, 0, 0, 0, OnButtonPress_4 );

CircularButton	(	g_sPushBtn_5, &g_sBackground, 0, 0, &g_sKentec320x240x16_SSD2119,
					LOC_X_5, LOC_Y_0UTPUTS, 15,															//x,y,radius
					(PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
					PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
					ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
					g_psFontCmss22b, "0", 0, 0, 0, 0, OnButtonPress_5 );

CircularButton	(	g_sPushBtn_6, &g_sBackground, 0, 0, &g_sKentec320x240x16_SSD2119,
					LOC_X_6, LOC_Y_0UTPUTS, 15,															//x,y,radius
					(PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
					PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
					ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
					g_psFontCmss22b, "0", 0, 0, 0, 0, OnButtonPress_6 );

CircularButton	(	g_sPushBtn_7, &g_sBackground, 0, 0, &g_sKentec320x240x16_SSD2119,
					LOC_X_7, LOC_Y_0UTPUTS, 15,															//x,y,radius
					(PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
					PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
					ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
					g_psFontCmss22b, "0", 0, 0, 0, 0, OnButtonPress_7 );



//*****************************************************************************
//
// A global we use to keep track of whether or not the OUTPUT is enabled or not
//
//*****************************************************************************
bool g_bHelloVisible_0 	= false;
bool g_bHelloVisible_1 	= false;
bool g_bHelloVisible_2 	= false;
bool g_bHelloVisible_3 	= false;
bool g_bHelloVisible_4 	= false;
bool g_bHelloVisible_5 	= false;
bool g_bHelloVisible_6 	= false;
bool g_bHelloVisible_7 	= false;

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

//*****************************************************************************
//
// Function call when OUTPUT buttons are pressed
//
//*****************************************************************************
void OnButtonPress_0(tWidget *psWidget)
{
    g_bHelloVisible_0 = !g_bHelloVisible_0;

    if(g_bHelloVisible_0)
    {
        PushButtonTextSet(&g_sPushBtn_0, "1");
        WidgetPaint(WIDGET_ROOT);
		qut_set_gpio( 0, 1 );
    }
    else
    {
		PushButtonTextSet(&g_sPushBtn_0, "0");
        WidgetPaint(WIDGET_ROOT);
		qut_set_gpio( 0, 0 );
    }
}

void OnButtonPress_1 (tWidget *psWidget)
{
    g_bHelloVisible_1 = !g_bHelloVisible_1;

    if(g_bHelloVisible_1)
    {
        PushButtonTextSet(&g_sPushBtn_1, "1");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 1, 1 );
    }
    else
    {
        PushButtonTextSet(&g_sPushBtn_1, "0");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 1, 0 );
    }
}

void OnButtonPress_2 (tWidget *psWidget)
{
    g_bHelloVisible_2 = !g_bHelloVisible_2;

    if(g_bHelloVisible_2)
    {
        PushButtonTextSet(&g_sPushBtn_2, "1");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 2, 1 );
    }
    else
    {
        PushButtonTextSet(&g_sPushBtn_2, "0");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 2, 0 );
    }
}

void OnButtonPress_3 (tWidget *psWidget)
{
    g_bHelloVisible_3 = !g_bHelloVisible_3;

    if(g_bHelloVisible_3)
    {
        PushButtonTextSet(&g_sPushBtn_3, "1");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 3, 1 );
    }
    else
    {
        PushButtonTextSet(&g_sPushBtn_3, "0");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 3, 0 );
    }
}
void OnButtonPress_4(tWidget *psWidget)
{
    g_bHelloVisible_4 = !g_bHelloVisible_4;

    if(g_bHelloVisible_4)
    {
        PushButtonTextSet(&g_sPushBtn_4, "1");
        WidgetPaint(WIDGET_ROOT);
		qut_set_gpio( 4, 1 );
    }
    else
    {
		PushButtonTextSet(&g_sPushBtn_4, "0");
        WidgetPaint(WIDGET_ROOT);
		qut_set_gpio( 4, 0 );
    }
}

void OnButtonPress_5 (tWidget *psWidget)
{
    g_bHelloVisible_5 = !g_bHelloVisible_5;

    if(g_bHelloVisible_5)
    {
        PushButtonTextSet(&g_sPushBtn_5, "1");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 5, 1 );
    }
    else
    {
        PushButtonTextSet(&g_sPushBtn_5, "0");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 5, 0 );
    }
}

void OnButtonPress_6 (tWidget *psWidget)
{
    g_bHelloVisible_6 = !g_bHelloVisible_6;

    if(g_bHelloVisible_6)
    {
        PushButtonTextSet(&g_sPushBtn_6, "1");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 6, 1 );
    }
    else
    {
        PushButtonTextSet(&g_sPushBtn_6, "0");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 6, 0 );
    }
}

void OnButtonPress_7 (tWidget *psWidget)
{
    g_bHelloVisible_7 = !g_bHelloVisible_7;

    if(g_bHelloVisible_7)
    {
        PushButtonTextSet(&g_sPushBtn_7, "1");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 7, 1 );
    }
    else
    {
        PushButtonTextSet(&g_sPushBtn_7, "0");

        WidgetPaint(WIDGET_ROOT);

		qut_set_gpio( 7, 0 );
    }
}

Void timeFxn(UArg arg0)
{
	/* Explicit posting of Event_Id_00 by calling Event_post() */
	Event_post(evt, Event_Id_00);
}


/*
 *  ======== taskFxn ========
 */
Void taskFxn(UArg a0, UArg a1)
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
			updateScreen();

		}
		Task_sleep(10);
	}
}

/*
 *  ======== main ========
 */
Int main()
{ 
    Task_Handle task;
    Error_Block eb;

    System_printf("enter main()\n");

    Error_init(&eb);
    task = Task_create(taskFxn, NULL, &eb);
    if (task == NULL) {
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
	Seconds_set(1462372289);

	/* Initialize an Event Instance */
	evt = Event_create(NULL, NULL);

	//Initialize the screen
	initScreen(&festoData);

    BIOS_start();    /* does not return */
    return(0);
}
