#include <stdint.h>

#include "driverLib.h"
#include "qut_tiva.h"
#include <ti/sysbios/knl/Task.h>

/* Initialize all datastructures */
bool init() {
	enableMovement = true;
	return true;
}

/* Init Station, bring everything in a normal position (by caling the other functions) */
bool initStation() {
	bool success = true;
	success &= movePlatform(false, false);
	success &= controlEjector(false, false);
	return success;
}

/* Move Platform: Pass bool value as a parameter true=moveUp, false=moveDown */
bool movePlatform(bool up, bool secureMovement) {
	while (secureMovement && qut_get_gpio(2)) {
		if (!enableMovement) {
			return false;
		}
		Task_sleep(5);
	}
	if (up) {
		qut_set_gpio (0, 0);
		qut_set_gpio (1, 1);
		while (!qut_get_gpio(4)) {
			if (!enableMovement) {
				qut_set_gpio (1, 0);
				return false;
			}
			Task_sleep(5);
		}
		qut_set_gpio (1, 0);
	}
	else {
		qut_set_gpio (0, 1);
		qut_set_gpio (1, 0);
		int32_t counter = 0;
		while (counter < 500000) {
			if (qut_get_gpio(5)) {
				counter++;
			}
			if (!enableMovement) {
				qut_set_gpio (0, 0);
				return false;
			}
			Task_sleep(5);
		}
		qut_set_gpio (0, 0);
	}
	return true;
}

/* Control the ejector: Pass bool value as a parameter true=extend, false=retract */
bool controlEjector(bool extend, bool secureMovement) {
	while (secureMovement && qut_get_gpio(2)) {
		if (!enableMovement) {
			return false;
		}
		Task_sleep(5);
	}
	if (extend) {
		qut_set_gpio (2, 1);
		/*while (qut_get_gpio(6)) {
			if (!enableMovement) {
				qut_set_gpio (2, 0);
				return false;
			}
		}*/
		Task_sleep(500);
	}
	else {
		qut_set_gpio (2, 0);
		while (!qut_get_gpio(6)){
			if (!enableMovement) {
				return false;
			}
			Task_sleep(5);
		}
	}
	return true;
}
/* Enable/Disable air slider: Pass bool value as a parameter true=enable, false=disable */
void controlAirSlider(bool enable) {
	qut_set_gpio (3, enable);
}

/* Enable/Disable movements: Pass bool value as a parameter true=enable, false=disable */
void controlMovements(bool enable) {
	enableMovement = enable;
	qut_set_gpio (3, 0);
}

/* Sense Workpiece */
bool senseWorkpiece() {
	return qut_get_gpio(0);
}

/* Sense Safety Barrier */
bool senseSafetyBarrierClear() {
	return !qut_get_gpio(2);
}

/* Return the height of workpiece as a human readable value (unit:mm) */
uint32_t getRawWorkpieceHeight() {
	return QUT_ADC0_Read;
}

/* Get Color and Material, returning a struct which contains two bool values (material and color)*/
struct ColorMaterial getMaterial(){
	struct ColorMaterial curColorMaterial;
	curColorMaterial.black = !qut_get_gpio(1);
	curColorMaterial.metallic = qut_get_gpio(3);
	return curColorMaterial;
}

