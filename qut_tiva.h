#ifndef QUT_TIVA_H_
#define QUT_TIVA_H_

#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c129xnczad.h"
#include "driverlib/uart.h"

//*****************************************************************************
//
// Simple loop to delay the program by roughly x number of seconds
// TBD : Use the input to change the time delay
//*****************************************************************************
void qut_delay_secs ( uint32_t how_long )
{
	volatile uint32_t ui32Loop;

	//
    // Delay for a bit.
    //
    for(ui32Loop = 0; ui32Loop < 200000; ui32Loop++)
    {
    }
}


//*****************************************************************************
//
// Set or Clear the status of a GPIO
//
//*****************************************************************************
void qut_set_gpio ( uint32_t which_gpio, uint32_t new_state )
{
	switch ( which_gpio ) {
		case 0:										//L1
			if ( new_state == 0 ) {
				GPIO_PORTL_DATA_R &= ~(0x02);
			}
			else {
				GPIO_PORTL_DATA_R |= 0x02;
			}
			break;
		case 1:										//L0
			if ( new_state == 0 ) {
				GPIO_PORTL_DATA_R &= ~(0x01);
			}
			else {
				GPIO_PORTL_DATA_R |= 0x01;
			}
			break;
		case 2:										//L2
			if ( new_state == 0 ) {
				GPIO_PORTL_DATA_R &= ~(0x04);
			}
			else {
				GPIO_PORTL_DATA_R |= 0x04;
			}
			break;
		case 3:										//L3
			if ( new_state == 0 ) {
				GPIO_PORTL_DATA_R &= ~(0x08);
			}
			else {
				GPIO_PORTL_DATA_R |= 0x08;
			}
			break;
		case 4:										//L4
			if ( new_state == 0 ) {
				GPIO_PORTL_DATA_R &= ~(0x10);
			}
			else {
				GPIO_PORTL_DATA_R |= 0x10;
			}
			break;
		case 5:										//L5
			if ( new_state == 0 ) {
				GPIO_PORTL_DATA_R &= ~(0x20);
			}
			else {
				GPIO_PORTL_DATA_R |= 0x20;
			}
			break;
		case 6:										//P5
			if ( new_state == 0 ) {
				GPIO_PORTP_DATA_R &= ~(0x20);
			}
			else {
				GPIO_PORTP_DATA_R |= 0x20;
			}
			break;
		case 7:										//P4
			if ( new_state == 0 ) {
				GPIO_PORTP_DATA_R &= ~(0x10);
			}
			else {
				GPIO_PORTP_DATA_R |= 0x10;
			}
			break;


		default:
			// Code
			break;
	}
}

uint32_t qut_get_gpio ( uint32_t which_gpio ){

	uint32_t	ui32PinState;

	switch ( which_gpio ) {
		case 0:										//M3
			ui32PinState	= GPIOPinRead ( GPIO_PORTM_BASE, GPIO_PIN_3 );
			break;
		case 1:										//M2
			ui32PinState	= GPIOPinRead ( GPIO_PORTM_BASE, GPIO_PIN_2 );
			break;
		case 2:										//M1
			ui32PinState	= GPIOPinRead ( GPIO_PORTM_BASE, GPIO_PIN_1 );
			break;
		case 3:										//M0
			ui32PinState	= GPIOPinRead ( GPIO_PORTM_BASE, GPIO_PIN_0 );
			break;
		case 4:										//N4
			ui32PinState	= GPIOPinRead ( GPIO_PORTN_BASE, GPIO_PIN_4 );
			break;
		case 5:										//A7
			ui32PinState	= GPIOPinRead ( GPIO_PORTA_BASE, GPIO_PIN_7 );
			break;
		case 6:										//C6
			ui32PinState	= GPIOPinRead ( GPIO_PORTC_BASE, GPIO_PIN_6 );
			break;
		case 7:										//C5
			ui32PinState	= GPIOPinRead ( GPIO_PORTC_BASE, GPIO_PIN_5 );
			break;
		default:
			ui32PinState	= 0;
			break;
	}

	if ( ui32PinState != 0 ) {
		return( 1 );
	}
	else {
		return( 0 );
	}

}

void qut_wait_gpio_is ( uint32_t which_gpio, uint32_t new_state ){

	while ( qut_get_gpio( which_gpio ) != new_state ){}
}

//*****************************************************************************
//
// ADC Functions
//
//*****************************************************************************
void QUT_ADC0_Init()	//ADC0 on PE3
{
	SysCtlPeripheralEnable( SYSCTL_PERIPH_ADC0 );

	SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOE );
	GPIOPinTypeADC( GPIO_PORTE_BASE, GPIO_PIN_3 );													//Makes GPIO an INPUT and sets them to be ANALOG

	ADCSequenceConfigure( ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0 );									//uint32_t ui32Base, uint32_t ui32SequenceNum, uint32_t ui32Trigger, uint32_t ui32Priority
	ADCSequenceStepConfigure( ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_CH0  | ADC_CTL_END );			//uint32_t ui32Base, uint32_t ui32SequenceNum, uint32_t ui32Step,    uint32_t ui32Config
	ADCSequenceEnable( ADC0_BASE, 0 );
	ADCIntClear( ADC0_BASE, 0 );
}

uint32_t QUT_ADC0_Read()
{
	uint32_t pui32ADC0Value[1];

	// Trigger the ADC conversion.
    ADCProcessorTrigger( ADC0_BASE, 0 );

    // Wait for conversion to be completed.
    while(!ADCIntStatus( ADC0_BASE, 0, false) )
    {
    }

    //Clear ADC Interrupt
    ADCIntClear( ADC0_BASE, 0 );

    // Read ADC Value.
    ADCSequenceDataGet( ADC0_BASE, 0, pui32ADC0Value );

    return ( pui32ADC0Value[0] );
}


//*****************************************************************************
//
// UART Functions
//
//*****************************************************************************
void QUT_UART_Init( uint32_t ui32SysClock )
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	UARTConfigSetExpClk(UART0_BASE, ui32SysClock, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

void QUT_UART_Send( const uint8_t *pui8Buffer, uint32_t ui32Count )
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
    	UARTCharPut(UART0_BASE, *pui8Buffer++);
    }
}

void QUT_UART_Send_uint32_t( uint32_t val_to_send ){

	char digit;

	digit=(char)(((int)'0')+(val_to_send/1000));
	UARTCharPut(UART0_BASE,digit);

	digit=(char)(((int)'0')+((val_to_send/100)%10));
	UARTCharPut(UART0_BASE,digit);

	digit=(char)(((int)'0')+((val_to_send/10)%10));
	UARTCharPut(UART0_BASE,digit);

	digit=(char)(((int)'0')+(val_to_send%10));;
	UARTCharPut(UART0_BASE,digit);

}


#endif /* QUT_TIVA_H_ */
