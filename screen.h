#ifndef SCREEN_H_
#define SCREEN_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef enum {STATUS_SCREEN, CALIBRATE, THRESHOLD} screenState_type;
typedef enum {REVIEW_T, ADJUST_BOTTOM, ADJUST_TOP} thresholdState_type;
typedef enum {REVIEW_C, FIRST_VALUE, SECOND_VALUE} calibrateState_type;
typedef enum {METALLIC, NON_METALLIC, UNKNOWN_MATERIAL} material_type;
typedef enum {BLACK, NON_BLACK, UNKNOWN_COLOR} color_type;

typedef struct
{
	 time_t theTime;
	 uint32_t operatingTime;
	 screenState_type screenState;
	 thresholdState_type thresholdState;
	 calibrateState_type calibrateState;
	 material_type currentMaterial;
	 color_type currentColor;
	 uint32_t currentHeight;
	 uint32_t countTotal;
	 uint32_t countAccepted;
	 uint32_t countMetallic;
	 uint32_t countBlack;
	 uint32_t thresholdTop;
	 uint32_t thresholdBottom;
	 uint32_t calibrateFirst;
	 uint32_t calibrateSecond;
	 bool enableMovement;
	 bool measuring;
} festoData_type;

void initScreen(festoData_type *festoData);
void updateScreen();
void drawStatusScreen();
void drawCalibrateScreen();
void drawThresholdScreen();
void drawTime();
void drawToggle();
void clearScreen();
void updateLED();

#endif /* SCREEN_H_ */
