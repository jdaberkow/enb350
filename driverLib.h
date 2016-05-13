#ifndef DRIVERLIB__H_
#define DRIVERLIB__H_

#include <stdbool.h>

struct ColorMaterial {
	bool metallic; 	/*true = metallic, false = other material*/
	bool black; 	/*true = orange, false = black*/
};

bool enableMovement;

/* Initialize all datastructures */
bool init();
/* Init Station, bring everything in a normal position (by caling the other functions) */
bool initStation();
/* Move Platform: Pass bool value as a parameter true=moveUp, false=moveDown */
bool movePlatform(bool up, bool secureMovement);
/* Control the ejector: Pass bool value as a parameter true=extend, false=retract */
bool controlEjector(bool extend, bool secureMovement);
/* Enable/Disable air slider: Pass bool value as a parameter true=enable, false=disable */
void controlAirSlider(bool enable);
/* Enable/Disable movements: Pass bool value as a parameter true=enable, false=disable */
void controlMovements(bool enable);
/* Sense Workpiece */
bool senseWorkpiece();
/* Sense Safety Barrier */
bool senseSafetyBarrierClear();
/* Return the height of workpiece as a human readable value (unit:mm) */
uint32_t getRawWorkpieceHeight();
/* Get Color and Material, returning a struct which contains two bool values (material and color)*/
struct ColorMaterial getMaterial();

#endif /* DRIVERLIB__H_ */
