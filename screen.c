#include <screen.h>

#include <xdc/runtime/System.h>

#include "grlib/grlib.h"

#include "inc/tm4c129xnczad.h"

#include "driverlib/sysctl.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/frame.h"

#include "utils/ustdlib.h"

tContext 	    sContext;
tContext 	    timeContext;
tContext 	    mediumContext;
tContext 	    ttContext;
tContext 	    ttBigContext;
tContext 	    ttGrayContext;
tContext 	    buttonContext;
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
char stringThresholdBottom [10];
char stringThresholdTop [10];
char stringCalibrateFirst [10];
char stringCalibrateSecond [10];

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

static const tRectangle thresholdBottomRect =
    {
        85 - 52,
        100 - 17,
        85 + 52,
        100 + 17,
    };

static const tRectangle thresholdTopRect =
    {
    		235 - 52,
			100  - 17,
			235 + 52,
			100  + 17,
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
	GrContextInit(&ttBigContext, &g_sKentec320x240x16_SSD2119);
	GrContextInit(&ttGrayContext, &g_sKentec320x240x16_SSD2119);
	GrContextInit(&buttonContext, &g_sKentec320x240x16_SSD2119);
	GrContextInit(&clearingContext, &g_sKentec320x240x16_SSD2119);

	//
	// Draw the application frame.
	//
	FrameDraw(&sContext,        "Festo Station");
	FrameDraw(&timeContext,     "Festo Station");
	FrameDraw(&mediumContext,   "Festo Station");
	FrameDraw(&ttContext,       "Festo Station");
	FrameDraw(&ttBigContext,    "Festo Station");
	FrameDraw(&ttGrayContext,   "Festo Station");
	FrameDraw(&buttonContext,   "Festo Station");
	FrameDraw(&clearingContext, "Festo Station");

	GrContextFontSet(&sContext, &g_sFontCm14);
	GrContextFontSet(&timeContext, &g_sFontCmss12);
	GrContextFontSet(&mediumContext, &g_sFontCmsc14);
	GrContextFontSet(&ttContext, &g_sFontCm14);
	GrContextFontSet(&ttBigContext, &g_sFontCm26);
	GrContextFontSet(&ttGrayContext, &g_sFontCm14);
	GrContextFontSet(&buttonContext, &g_sFontCmsc16);
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
	GrStringDraw(&buttonContext, "< CALIBRATE", -1, 10, 160, 0);
	GrStringDraw(&buttonContext, "< THRESHOLD", -1, 10, 214, 0);
	drawToggle();
}

void drawCalibrateScreen() {
	GrStringDrawCentered(&buttonContext, "CALIBRATION", -1, 160, 50, 0);

	GrStringDrawCentered(&mediumContext, "First", -1, 85, 75, 0);
	usprintf(stringCalibrateFirst, "%i.%i mm", festoData->calibrateFirst/10, festoData->calibrateFirst%10);
	GrStringDrawCentered(&ttBigContext, stringCalibrateFirst, -1, 85, 100, 0);

	GrStringDrawCentered(&mediumContext, "Second", -1, 235, 75, 0);
	usprintf(stringCalibrateSecond, "%i.%i mm", festoData->calibrateSecond/10, festoData->calibrateSecond%10);
	GrStringDrawCentered(&ttBigContext, stringCalibrateSecond, -1, 235, 100, 0);

	switch (festoData->calibrateState) {
		case REVIEW_C:
			if (!festoData->measuring) {
				GrStringDraw(&buttonContext, "< STATUS", -1, 10, 160, 0);
				GrStringDraw(&buttonContext, "< THRESHOLD", -1, 10, 214, 0);
				GrStringDrawCentered(&buttonContext, "CALIBRATE >", -1, 252, 221, 0);
			}
			break;
		case FIRST_VALUE:
			GrStringDraw(&buttonContext, "< UP", -1, 10, 160, 0);
			GrStringDraw(&buttonContext, "< DOWN", -1, 10, 214, 0);

			GrRectDraw(&mediumContext, &thresholdBottomRect);
			if (!festoData->measuring)
				GrStringDrawCentered(&buttonContext, "APPLY >", -1, 273, 221, 0);
			break;
		case SECOND_VALUE:
			GrStringDraw(&buttonContext, "< UP", -1, 10, 160, 0);
			GrStringDraw(&buttonContext, "< DOWN", -1, 10, 214, 0);

			GrRectDraw(&mediumContext, &thresholdTopRect);
			if (!festoData->measuring)
				GrStringDrawCentered(&buttonContext, "APPLY >", -1, 273, 221, 0);
			break;
		default:
			break;
	}
}

void drawThresholdScreen() {
	GrStringDrawCentered(&buttonContext, "THRESHOLD", -1, 160, 50, 0);

	GrStringDrawCentered(&mediumContext, "Bottom", -1, 85, 75, 0);
	usprintf(stringThresholdBottom, "%i.%i mm", festoData->thresholdBottom/10, festoData->thresholdBottom%10);
	GrStringDrawCentered(&ttBigContext, stringThresholdBottom, -1, 85, 100, 0);

	GrStringDrawCentered(&mediumContext, "Top", -1, 235, 75, 0);
	usprintf(stringThresholdTop, "%i.%i mm", festoData->thresholdTop/10, festoData->thresholdTop%10);
	GrStringDrawCentered(&ttBigContext, stringThresholdTop, -1, 235, 100, 0);

	switch (festoData->thresholdState) {
		case REVIEW_T:
			GrStringDraw(&buttonContext, "< CALIBRATE", -1, 10, 160, 0);
			GrStringDraw(&buttonContext, "< STATUS", -1, 10, 214, 0);
			GrStringDrawCentered(&buttonContext, "ADJUST >", -1, 270, 221, 0);
			break;
		case ADJUST_BOTTOM:
			GrStringDraw(&buttonContext, "< UP", -1, 10, 160, 0);
			GrStringDraw(&buttonContext, "< DOWN", -1, 10, 214, 0);
			GrStringDrawCentered(&buttonContext, "APPLY >", -1, 273, 221, 0);

			GrRectDraw(&mediumContext, &thresholdBottomRect);
			break;
		case ADJUST_TOP:
			GrStringDraw(&buttonContext, "< UP", -1, 10, 160, 0);
			GrStringDraw(&buttonContext, "< DOWN", -1, 10, 214, 0);
			GrStringDrawCentered(&buttonContext, "APPLY >", -1, 273, 221, 0);

			GrRectDraw(&mediumContext, &thresholdTopRect);
			break;
		default:
			break;
	}
}

void drawTime() {
	ltm = localtime(&festoData->theTime);
	curTime = asctime(ltm);
	GrStringDrawCentered(&timeContext, curTime, 24, 160, 30, 0);
}

void drawToggle() {
	if (festoData->enableMovement) {
		GrStringDrawCentered(&buttonContext, "STOP >", -1, 280, 221, 0);
	}
	else {
		GrStringDrawCentered(&buttonContext, "START >", -1, 273, 221, 0);
	}
}

void clearScreen() {
    GrRectFill(&clearingContext, &sRect);
}

void updateLED() {
	if (festoData->enableMovement) {
		if (festoData->calibrateState != REVIEW_C || festoData->measuring) {
			GPIO_PORTQ_DATA_R |= 0x80;		//enable Green LED
			GPIO_PORTN_DATA_R |= 0x20;		//enable Red LED
		}
		else {
			GPIO_PORTQ_DATA_R |= 0x80;		//enable Green LED
			GPIO_PORTN_DATA_R &= ~(0x20);	//disable Red LED
		}
	}
	else {
		GPIO_PORTQ_DATA_R &= ~(0x80);	//disable Green LED
		GPIO_PORTN_DATA_R |= 0x20;		//enable Red LED
	}
}
