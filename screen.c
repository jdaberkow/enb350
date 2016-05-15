#include <screen.h>

#include "grlib/grlib.h"

#include "driverlib/sysctl.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/frame.h"

#include "utils/ustdlib.h"

tContext 	    sContext;
tContext 	    timeContext;
tContext 	    mediumContext;
tContext 	    ttContext;
tContext 	    ttGrayContext;
tContext 	    clearingContext;
uint32_t 	    ui32SysClock;
festoData_type *festoData;
uint32_t        mmToADCCoefficient;

struct tm *ltm;
char* curTime;
char stringHeight [10];
char stringProcessed [10];
char stringAccepted [10];
char stringBlack [10];
char stringMetallic [10];
char stringRuntime [12];
char stringThroughput [10];

static const tRectangle sRect =
    {
        0,
        0,
        319,
        239,
    };

static const tRectangle currentRect =
    {
        15,
        58,
        155,
        130,
    };

static const tRectangle summaryRect =
    {
        165,
        58,
        305,
        192,
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
	GrContextInit(&mediumContext, &g_sKentec320x240x16_SSD2119);
	GrContextInit(&ttContext, &g_sKentec320x240x16_SSD2119);
	GrContextInit(&ttGrayContext, &g_sKentec320x240x16_SSD2119);
	GrContextInit(&clearingContext, &g_sKentec320x240x16_SSD2119);

	//
	// Draw the application frame.
	//
	FrameDraw(&sContext,        "Festo Station");
	FrameDraw(&timeContext,     "Festo Station");
	FrameDraw(&mediumContext,   "Festo Station");
	FrameDraw(&ttContext,       "Festo Station");
	FrameDraw(&ttGrayContext,   "Festo Station");
	FrameDraw(&clearingContext, "Festo Station");

	GrContextFontSet(&sContext, &g_sFontCm14);
	GrContextFontSet(&timeContext, &g_sFontCmss12);
	GrContextFontSet(&mediumContext, &g_sFontCmsc14);
	GrContextFontSet(&ttContext, &g_sFontCm14);
	GrContextFontSet(&ttGrayContext, &g_sFontCm14);
	GrContextForegroundSet(&ttGrayContext, ClrLightGrey);
	GrContextForegroundSet(&clearingContext, ClrBlack);
}

void updateScreen() {
	/* Clear the screen to allow painting of the new frame */
	clearScreen();

	/* Draw the calendar time*/
	drawTime();

	/* Draw the screen depending on the status we are in */
	switch (festoData->screenState) {
		case STATUS_SCREEN:
			drawStatusScreen();
			break;
		case CALIBRATE:
			drawCalibrateScreen();
			break;
		case THRESHOLD:
			drawThresholdScreen();
			break;
		default:
			break;
	}
}

void drawStatusScreen() {
	//
	// Draw the application frame.
	//

	GrStringDrawCentered(&mediumContext, "Current", -1, 85, 50, 0);
	GrRectDraw(&mediumContext, &currentRect);
	GrStringDraw(&sContext, "Color:", -1, 20, 65, 0);
	GrStringDraw(&sContext, "Material:", -1, 20, 87, 0);
	GrStringDraw(&sContext, "Height:", -1, 20, 109, 0);

	switch (festoData->currentColor) {
		case BLACK:
			GrStringDraw(&ttContext, "Black", -1, 85, 65, 0);
			break;
		case NON_BLACK:
			GrStringDraw(&ttContext, "Non-black", -1, 85, 65, 0);
			break;
		case UNKNOWN_COLOR:
			GrStringDraw(&ttGrayContext, "?", -1, 85, 65, 0);
			break;
		default:
			break;
	}

	switch (festoData->currentMaterial) {
		case METALLIC:
			GrStringDraw(&ttContext, "Metal", -1, 85, 87, 0);
			break;
		case NON_METALLIC:
			GrStringDraw(&ttContext, "Non-metal", -1, 85, 87, 0);
			break;
		case UNKNOWN_MATERIAL:
			GrStringDraw(&ttGrayContext, "?", -1, 85, 87, 0);
			break;
		default:
			break;
	}

	if (festoData->currentHeight == 0) {
		GrStringDraw(&ttGrayContext, "?", -1, 85, 109, 0);
	}
	else {
		usprintf(stringHeight, "%i mm", festoData->currentHeight);
		GrStringDraw(&ttContext, stringHeight, -1, 85, 109, 0);
	}


	GrStringDrawCentered(&mediumContext, "Summary", -1, 235, 50, 0);
	GrRectDraw(&mediumContext, &summaryRect);

	GrStringDraw(&sContext, "Processed:", -1, 170, 65, 0);
	usprintf(stringProcessed, "%i", festoData->countTotal);
	GrStringDrawCentered(&ttContext, stringProcessed, -1, 270, 70, 0);

	GrStringDraw(&sContext, "Accepted:", -1, 170, 87, 0);
	usprintf(stringAccepted, "%i (%i%%)", festoData->countAccepted, (festoData->countAccepted * 100) / festoData->countTotal);
	GrStringDrawCentered(&ttContext, stringAccepted, -1, 270, 92, 0);

	GrStringDraw(&sContext, "Black:", -1, 170, 109, 0);
	usprintf(stringBlack, "%i (%i%%)", festoData->countBlack, (festoData->countBlack * 100) / festoData->countTotal);
	GrStringDrawCentered(&ttContext, stringBlack, -1, 270, 114, 0);

	GrStringDraw(&sContext, "Metallic:", -1, 170, 131, 0);
	usprintf(stringMetallic, "%i (%i%%)", festoData->countMetallic, (festoData->countMetallic * 100) / festoData->countTotal);
	GrStringDrawCentered(&ttContext, stringMetallic, -1, 270, 136, 0);

	GrStringDraw(&sContext, "Runtime:", -1, 170, 153, 0);
	int seconds, minutes, hours;
	seconds = festoData->operatingTime%60;
	minutes = (festoData->operatingTime/60)%60;
	hours   = (festoData->operatingTime/3600)%60;
	usprintf(stringRuntime, "%ih %im %is", hours, minutes, seconds);
	GrStringDrawCentered(&ttContext, stringRuntime, -1, 270, 158, 0);

	GrStringDraw(&sContext, "Flow-rate:", -1, 170, 175, 0);
	usprintf(stringMetallic, "%i ppm", (festoData->countTotal*60) / festoData->operatingTime);
	GrStringDrawCentered(&ttContext, stringMetallic, -1, 270, 180, 0);


	/* Draw the hardware button labels*/


	/* Call the API function that copies the buffer to the screen*/
}

void drawCalibrateScreen() {
	GrStringDraw(&sContext, "CALIBRATE", -1, 170, 153, 0);
}

void drawThresholdScreen() {
	GrStringDraw(&sContext, "THRESHOLD", -1, 170, 153, 0);
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
