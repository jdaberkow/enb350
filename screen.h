#ifndef SCREEN_H_
#define SCREEN_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef enum {STATUS, CALIBRATE, THRESHOLD} screenState_type;
typedef enum {METALLIC, NON_METALLIC, UNKNOWN_MATERIAL} material_type;
typedef enum {BLACK, NON_BLACK, UNKNOWN_COLOR} color_type;

typedef struct
{
	 screenState_type screenState;
	 time_t theTime;
	 uint32_t operatingTime;
	 material_type currentMaterial;
	 color_type currentColor;
	 uint32_t currentHeight;
	 uint32_t countTotal;
	 uint32_t countAccepted;
	 uint32_t countMetallic;
	 uint32_t countBlack;
	 uint32_t thresholdTop;
	 uint32_t thresholdBottom;
} festoData_type;

void initScreen(festoData_type *festoData);
void updateScreen();
void drawStatusScreen();
void drawCalibrateScreen();
void drawThresholdScreen();
void drawTime();
void clearScreen();

#endif /* SCREEN_H_ */
