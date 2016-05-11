#include <screen.h>

#include "grlib/grlib.h"

#include "driverlib/sysctl.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/frame.h"


tContext 	    sContext;
tContext 	    timeContext;
tContext 	    clearingContext;
uint32_t 	    ui32SysClock;
festoData_type *festoData;
uint32_t        mmToADCCoefficient;

struct tm *ltm;
char* curTime;

static const tRectangle sRect =
    {
        0,
        0,
        319,
        239,
    };

void initScreen(festoData_type *pFestoData) {
	/* Initialize pointer to Festo Data */
	festoData = pFestoData;

	/* Set up LCD */
	uint32_t ui32SysClock;

	//
	// Run from the PLL at 120 MHz.
	//
	ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
									  SYSCTL_OSC_MAIN |
									  SYSCTL_USE_PLL |
									  SYSCTL_CFG_VCO_480), 120000000);

	//
	// Initialize the display driver.
	//
	Kentec320x240x16_SSD2119Init(ui32SysClock);

	//
	// Initialize the graphics context.
	//
	GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);
	GrContextInit(&timeContext, &g_sKentec320x240x16_SSD2119);
	GrContextInit(&clearingContext, &g_sKentec320x240x16_SSD2119);

	//
	// Draw the application frame.
	//
	FrameDraw(&sContext,        "Festo Station");
	FrameDraw(&timeContext,     "Festo Station");
	FrameDraw(&clearingContext, "Festo Station");

	GrContextFontSet(&timeContext, &g_sFontCmss12);
	GrContextForegroundSet(&clearingContext, ClrBlack);
}

void updateScreen() {
	drawStatusScreen();
}

void drawStatusScreen() {
	//
	// Draw the application frame.
	//
	clearScreen();
	drawTime();

	/* Draw Header Line to indicate in which state / menu we are*/


	/* Draw the calendar time*/


	/* Read all variables and draw their labels and values to the correct positions*/


	/* Draw the hardware button labels*/


	/* Call the API function that copies the buffer to the screen*/
}

void drawCalibrateScreen() {

}

void drawThresholdScreen() {

}

void drawTime() {
	ltm = localtime(&festoData->theTime);
	curTime = asctime(ltm);
	GrStringDrawCentered(&timeContext, curTime, 24, 160, 30, 0);
}

void clearScreen()
{
    GrRectFill(&clearingContext, &sRect);
}
