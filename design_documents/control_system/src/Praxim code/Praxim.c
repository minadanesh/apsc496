#include <avr/interrupt.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <util/delay.h>
#include <math.h>
#include <avr/pgmspace.h>
#include "config.h"
#include <can_lib.h>
#include <can_drv.h>
#include <uart/uart_lib.h>
#include <uart/uart_drv.h>
#include "Constants.h"

// Define baud rate

#define	__AVR_AT90CAN128__	1
//#define BAUD 9600
#define CAN_BAUDRATE 1000

#define BSET(p,b)        ((p) |=  (1<<b))                  // Set bit
#define BCLR(p,b)       ((p) &= ~(1<<b))                  // Clear bit

#define ms		TCNT1

#define PI		3.141592654
#define rxtx	"R"
#define on		1
#define off		0

#define Reset_AVR() wdt_enable(WDTO_30MS); while(1) {}



int shape = 4;
int debug = 0;



int encA = 0;
int encB = 0;
int encC = 0;

long blocker = 0;
long posAddress = 0;



float Xm;
float Ym;

st_cmd_t message;


char ReceivedByte;
long motorpos_temp = 0;
long motorpos = 0;
int bytecount = 0;
int receive_error = 0;
unsigned char status_toggle = 0;







//**************************************************************//
//  SERIAL COMMUNICATION FUNCTIONS								//
//																//
//**************************************************************//

void UART_Init(uint32_t Baud)
{
	unsigned int BaudRate = F_CPU / (16 * Baud) - 1;	//calculate BaudRate
	//set BaudRate into registers
	UBRR0H = (unsigned char) (BaudRate>>8);
	UBRR0L = (unsigned char) BaudRate;

	UCSR0B = 0b00011000;		//enable TX(Bit4 = 1), RX(Bit3 = 1)

	UCSR0C = 0b00000110;		//set frame format (even parity, 8 bits, 1 stop bit)
}

void UART_Transmit(unsigned char Data)
{
	while (!(UCSR0A & 0b00100000));
	UDR0 = Data;
}

void sendString(char str[])
{

    if (!str) return;
    while (*str)
        UART_Transmit(*str++);
}






//**************************************************************//
//  CAN COMMUNICATION FUNCTIONS									//
//																//
//**************************************************************//

void quit()
{

	//Disengage Motor:
	setVal16(0x6040,0x00,0x000F,&message);

	setVal16(0x6040,0x00,0,&message);
	sendString("\r\n\n\nQuitting...");
	Reset_AVR();
}

void printCAN(st_cmd_t* message){
//printing:
	U8 indx;
	char x[1000];
	//char rxtx = "R";

    if (message->ctrl.ide) 
    {
        //Uart_select(UART_0);
        sprintf(x,"--- TxCAN @ %02X: 0x%08lX(Ext.), L=%d, ", CANSTML, message->id.ext, message->dlc);
		sendString(x);
        //Uart_select(UART_1);
        //printf(x,"-1- %cxCAN @ %02X%02X: 0x%08lX(Ext.), L=%d, ", rxtx, CANSTMH, CANSTML, message->id.ext, message->dlc);
		//sendString(x);
    }
    else
    {
        //Uart_select(UART_0);
        sprintf(x,"-0- RxCAN @ %02X%02X:      0x%03X(Std.), L=%d, ", CANSTMH, CANSTML, message->id.std, message->dlc);
		sendString(x);
        //Uart_select(UART_1);
        //printf(x,"-1- %cxCAN @ %02X%02X:      0x%03X(Std.), L=%d, ", rxtx, CANSTMH, CANSTML, message->id.std, message->dlc);
		//sendString(x);
    }
    if (message->ctrl.rtr)
    {
        //Uart_select(UART_0);
		sprintf(x,"Remote\r\n");
		sendString(x); 
        //Uart_select(UART_1); printf(x,"Remote\r\n"); 
		//sendString(x);
    } 
    else
    {
        for(indx=0; indx< (message->dlc-1); indx++)
        {
            //Uart_select(UART_0);
			sprintf(x,"%02X-", *(message->pt_data + indx)); 
			sendString(x);
            //Uart_select(UART_1); printf(x,"%02X-", *(message->pt_data + indx)); 
			//sendString(x);
        }
        //Uart_select(UART_0);
		sprintf(x,"%02X\r\n", *(message->pt_data + indx));
		sendString(x);                  
        //Uart_select(UART_1); printf(x,"%02X\r\n", *(message->pt_data + indx));
		//sendString(x);
    }   

}

void setOpMode(st_cmd_t* message)
{
// --- ENTER OPERATIONAL MODE

	U8 u8_temp;
	U8 buffer[8];
	//st_cmd_t message;
	buffer[0] = 0x01;
	buffer[1] = 0x00;
	buffer[2] = 0x00;
	buffer[3] = 0x00;
	buffer[4] = 0x00;
	buffer[5] = 0x00;
	buffer[6] = 0x00;
	buffer[7] = 0x00;
            
	message->pt_data = &buffer[0];

    // --- Tx Command
	message->id.std = (int)0;
    message->dlc = 1;
    message->cmd = CMD_TX;
            
    // --- Enable Tx
    while(can_cmd(message) != CAN_CMD_ACCEPTED);
    // --- Wait for Tx completed        
    while(1)
    {
         u8_temp = can_get_status(message);
         if (u8_temp != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }
    //- CAN ECHO: PRINT-UART OF CAN FRAME TRANSMITTED
	if (debug) sendString("\n\n\rInitializing CANbus:\n\r");
    if (debug) printCAN(message);

}

void faultReset(st_cmd_t* message)
{
/*
	U8 u8_temp;
	U8 buffer[8];
	//st_cmd_t message;
	buffer[0] = 0x03;
	buffer[1] = 0x40;
	buffer[2] = 0x60;
	buffer[3] = 0x00;
	buffer[4] = 0x00;
	buffer[5] = 0x80;
	buffer[6] = 0x00;
	buffer[7] = 0x00;
            
	message->pt_data = &buffer[0];

    // --- Tx Command
	message->id.std = (int)(0x601);//(int)0;
    message->dlc = 8;//1;
    message->cmd = CMD_TX;
            
    // --- Enable Tx
    while(can_cmd(message) != CAN_CMD_ACCEPTED);
    // --- Wait for Tx completed        
    while(1)
    {
         u8_temp = can_get_status(message);
         if (u8_temp != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }
    //- CAN ECHO: PRINT-UART OF CAN FRAME TRANSMITTED
	if (debug) sendString("\n\n\rFAULT RESET\n\r");
    if (debug) printCAN(message);

	*/
	//setVal16(0x6040,0x00,271,&message);

	int val = (int)0x080;

	setVal16(0x6040,0x00,val,&message);
}

void setVal8(U16 index, U8 subindex, U8 val, st_cmd_t* msg)
{
	

	U8 u8_temp;
	//st_cmd_t msg;
	U8 buff[8];
	// --- Init Tx data


	buff[0] = 0x2F;
	buff[1] = (unsigned char) (index);
	buff[2] = (unsigned char) (index>>8);
	buff[3] = (unsigned char) (subindex);
	buff[4] = val;
	buff[5] = 0x00;
	buff[6] = 0x00;
	buff[7] = 0x00;
    
	msg->pt_data = &buff[0];

    // --- Tx Command
    //message.id.ext++;   // Incrementation of ID to revome possible clashes
	U16 COB = (int)(0x601);

	msg->id.std = COB;
    msg->dlc = 8;
    msg->cmd = CMD_TX;
    
	if (debug) sendString("\n\rSending:\n\r");
	if (debug) printCAN(msg);
    // --- Enable Tx
    while(can_cmd(msg) != CAN_CMD_ACCEPTED);
    // --- Wait for Tx completed        
    while(1)
    {
        u8_temp = can_get_status(msg);
        if (can_get_status(msg) != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }

    // ---- Exit if CAN error(s)
    if (u8_temp == CAN_STATUS_ERROR) quit(); // Out of function

   //- CAN ECHO: WAIT FOR RECEIVED
    // --- Init Rx data
    msg->pt_data = &buff[0];
    for(int i=0; i<8; i++) buff[i]=0;

    // --- Rx Command
    msg->cmd = CMD_RX;
    
    // --- Enable Rx
    while(can_cmd(msg) != CAN_CMD_ACCEPTED);
    // --- Wait for Rx completed
    while(1)
    {
        u8_temp = can_get_status(msg);
        if (u8_temp != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }
    if (u8_temp == CAN_STATUS_ERROR) quit(); // Out of the function

	if (debug) sendString("Received:\n\r");
    if (debug) printCAN(msg);

}


void setVal16(U16 index, U8 subindex, U16 val, st_cmd_t* msg)
{
	

	U8 u8_temp;
	//st_cmd_t msg;
	U8 buff[8];
	// --- Init Tx data


	buff[0] = 0x2B;
	buff[1] = (unsigned char) (index);
	buff[2] = (unsigned char) (index>>8);
	buff[3] = (unsigned char) (subindex);
	buff[4] = (unsigned char) (val);
	buff[5] = (unsigned char) (val>>8);
	buff[6] = 0x00;
	buff[7] = 0x00;
    
	msg->pt_data = &buff[0];

    // --- Tx Command
    //message.id.ext++;   // Incrementation of ID to revome possible clashes
	U16 COB = (int)(0x601);

	msg->id.std = COB;
    msg->dlc = 8;
    msg->cmd = CMD_TX;
    
	if (debug) sendString("\n\rSending:\n\r");
	if (debug) printCAN(msg);
    // --- Enable Tx
    while(can_cmd(msg) != CAN_CMD_ACCEPTED);
    // --- Wait for Tx completed        
    while(1)
    {
        u8_temp = can_get_status(msg);
        if (can_get_status(msg) != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }

    // ---- Exit if CAN error(s)
    if (u8_temp == CAN_STATUS_ERROR) quit(); // Out of function

   //- CAN ECHO: WAIT FOR RECEIVED
    // --- Init Rx data
    msg->pt_data = &buff[0];
    for(int i=0; i<8; i++) buff[i]=0;

    // --- Rx Command
    msg->cmd = CMD_RX;
    
    // --- Enable Rx
    while(can_cmd(msg) != CAN_CMD_ACCEPTED);
    // --- Wait for Rx completed
    while(1)
    {
        u8_temp = can_get_status(msg);
        if (u8_temp != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }
    if (u8_temp == CAN_STATUS_ERROR) quit(); // Out of the function

	if (debug) sendString("Received:\n\r");
    if (debug) printCAN(msg);

}

void setVal32(U16 index, U8 subindex, U32 val, st_cmd_t* msg)
{
	

	U8 u8_temp;
	//st_cmd_t msg;
	U8 buff[8];
	// --- Init Tx data


	buff[0] = 0x23;
	buff[1] = (unsigned char) (index);
	buff[2] = (unsigned char) (index>>8);
	buff[3] = (unsigned char) (subindex);
	buff[4] = (unsigned char) (val);
	buff[5] = (unsigned char) (val>>8);
	buff[6] = (unsigned char) (val>>16);
	buff[7] = (unsigned char) (val>>24);
    
	msg->pt_data = &buff[0];

    // --- Tx Command
    //message.id.ext++;   // Incrementation of ID to revome possible clashes
	U16 COB = (int)(0x601);

	msg->id.std = COB;
    msg->dlc = 8;
    msg->cmd = CMD_TX;
    
	if (debug) sendString("\n\rSending:\n\r");
	if (debug) printCAN(msg);
    // --- Enable Tx
    while(can_cmd(msg) != CAN_CMD_ACCEPTED);
    // --- Wait for Tx completed        
    while(1)
    {
        u8_temp = can_get_status(msg);
        if (can_get_status(msg) != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }

    // ---- Exit if CAN error(s)
    if (u8_temp == CAN_STATUS_ERROR) quit(); // Out of function

   //- CAN ECHO: WAIT FOR RECEIVED
    // --- Init Rx data
    msg->pt_data = &buff[0];
    for(int i=0; i<8; i++) buff[i]=0;

    // --- Rx Command
    msg->cmd = CMD_RX;
    
    // --- Enable Rx
    while(can_cmd(msg) != CAN_CMD_ACCEPTED);
    // --- Wait for Rx completed
    while(1)
    {
        u8_temp = can_get_status(msg);
        if (u8_temp != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }
    if (u8_temp == CAN_STATUS_ERROR) quit(); // Out of the function

	if (debug) sendString("Received:\n\r");
    if (debug) printCAN(msg);

}

int checkFault(st_cmd_t* msg)
{
	

	U8 u8_temp;
	//st_cmd_t msg;
	U8 buff[8];
	// --- Init Tx data

	U16 index = 0x6041;

	buff[0] = 0x40;
	buff[1] = (unsigned char) (index);
	buff[2] = (unsigned char) (index>>8);
	buff[3] = 0;
	buff[4] = 0;//(unsigned char) (val);
	buff[5] = 0;//(unsigned char) (val>>8);
	buff[6] = 0;//(unsigned char) (val>>16);
	buff[7] = 0;//(unsigned char) (val>>24);
    
	msg->pt_data = &buff[0];

    // --- Tx Command
    //message.id.ext++;   // Incrementation of ID to revome possible clashes
	U16 COB = (int)(0x601);

	msg->id.std = COB;
    msg->dlc = 8;
    msg->cmd = CMD_TX;
    
	if (debug) sendString("\n\rSending:\n\r");
	if (debug) printCAN(msg);
    // --- Enable Tx
    while(can_cmd(msg) != CAN_CMD_ACCEPTED);
    // --- Wait for Tx completed        
    while(1)
    {
        u8_temp = can_get_status(msg);
        if (can_get_status(msg) != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }

    // ---- Exit if CAN error(s)
    if (u8_temp == CAN_STATUS_ERROR) quit(); // Out of function

   //- CAN ECHO: WAIT FOR RECEIVED
    // --- Init Rx data
    msg->pt_data = &buff[0];
    for(int i=0; i<8; i++) buff[i]=0;

    // --- Rx Command
    msg->cmd = CMD_RX;
    
    // --- Enable Rx
    while(can_cmd(msg) != CAN_CMD_ACCEPTED);
    // --- Wait for Rx completed
    while(1)
    {
        u8_temp = can_get_status(msg);
        if (u8_temp != CAN_STATUS_NOT_COMPLETED) break; // Out of while
    }
    if (u8_temp == CAN_STATUS_ERROR) quit(); // Out of the function

	if (debug) sendString("Received:\n\r");
    if (debug) printCAN(msg);

	int faultByte = msg->pt_data[4];
	int fault = 0;

	if(bit_is_set(faultByte,3)) fault = 1;


	//char x[128];
	//sprintf(x,"\nFault State: %d",fault);
	//sendString("\r");
	//sendString(x);
	

	return fault;

}












//**************************************************************//
//  INITIALIZATION FUNCTIONS									//
//																//
//**************************************************************//

void interruptInit()
{
		cli();

		// Set Direction bit to Input
		BCLR(DDRD,INT0);
		BCLR(DDRD,INT1);
		BCLR(DDRD,INT2);
		BCLR(DDRD,INT3);
		BCLR(DDRE,INT5);
		BCLR(DDRE,INT6);

        // Set External Interrupt Control Register B to trigger on both edges
        BCLR(EICRA,ISC01);	// ISCn1 = 0
        BSET(EICRA,ISC00); 	// ISCn0 = 1
		BCLR(EICRA,ISC11);	// ISCn1 = 0
        BSET(EICRA,ISC10); 	// ISCn0 = 1
		BCLR(EICRA,ISC21);	// ISCn1 = 0
        BSET(EICRA,ISC20); 	// ISCn0 = 1
		BCLR(EICRA,ISC31);	// ISCn1 = 0
        BSET(EICRA,ISC30); 	// ISCn0 = 1
		BCLR(EICRB,ISC51);	// ISCn1 = 0
        BSET(EICRB,ISC50); 	// ISCn0 = 1
		BCLR(EICRB,ISC61);	// ISCn1 = 0
        BSET(EICRB,ISC60); 	// ISCn0 = 1

		//BCLR(PORTE,4);
		BSET(DDRE, 4);

		// Enable INT0, INT1 Interrupt
        BSET(EIMSK,INT0);
		BSET(EIMSK,INT1);
		BSET(EIMSK,INT2);
		BSET(EIMSK,INT3);
		BSET(EIMSK,INT5);
		BSET(EIMSK,INT6);

		UCSR0B |= (1 << RXCIE0); // Enable the USART Recieve Complete interrupt (USART_RXC) 		

		sei();
}

void canBusInit()
{

//--- I N I T
    // Clock prescaler Reset
    CLKPR = 0x80;  CLKPR = 0x00;
    DDRA =0xFF;

    //- Pull-up on TxCAN & RxCAN one by one to use bit-addressing
    CAN_PORT_DIR &= ~(1<<CAN_INPUT_PIN );
    CAN_PORT_DIR &= ~(1<<CAN_OUTPUT_PIN);
    CAN_PORT_OUT |=  (1<<CAN_INPUT_PIN );
    CAN_PORT_OUT |=  (1<<CAN_OUTPUT_PIN);

   // --- Init CAN (special AUTOBAUD)  
        //- Wait until activity on RxCAN
    //while ((CAN_PORT_IN & (1<<CAN_INPUT_PIN)) != 0);
	//_delay_ms(100);
        //- Reset CAN peripheral
	
    Can_reset();
    //- Set CAN Bit-timming
    can_init(CAN_BAUDRATE);        // c.f. macro in "can_drv.h"
    //- Set CAN Timer Prescaler
    CANTCON = CANBT1;                   // Why not !


}

void initButtons()
{
	//Stop:
	BSET(PORTC,0);
	BCLR(DDRC,0);
	//Button 1:
	BSET(PORTC,1);
	BCLR(DDRC,1);
	//Button 2:
	BSET(PORTC,2);
	BCLR(DDRC,2);
	//Button 3:
	BSET(PORTC,3);
	BCLR(DDRC,3);
	//Start:
	BSET(PORTC,5);
	BCLR(DDRC,5);

	//LED's:
	PINA = 0x00;
}


void faultState()
{
	LED1(0);
	LED2(0);
	LED3(0);
	while (bit_is_set(PINC,5))
	{
		_delay_ms(100);
		LED1(1);
		LED2(1);
		LED3(1);
		_delay_ms(100);
		LED1(0);
		LED2(0);
		LED3(0);
		if (bit_is_set(PINC,1)) quit();
	}
	LED1(0);

	//Fault Reset
	//faultReset(&message);
	setVal16(0x6040,0x00,0x0080,&message);
	_delay_ms(500);
	selectShape();

}









//**************************************************************//
//  ENCODER INTERRUPT COMMANDS									//
//																//
//**************************************************************//


ISR(INT0_vect)
{
	int a = 0;
	int b = 0;
	if(bit_is_set(PIND,0)) a =1;
	if(bit_is_set(PIND,1)) b =1;

    if (a==b) encA--;
    else encA++;

}

ISR(INT1_vect)
{
	int a = 0;
	int b = 0;
	if(bit_is_set(PIND,0)) a =1;
	if(bit_is_set(PIND,1)) b =1;

    if (a==b) encA++;
    else encA--;

}


ISR(INT2_vect)
{
		int a=0;
		int b=0;
		if (bit_is_set(PIND,2)) a = 1;
		if (bit_is_set(PIND,3)) b = 1;

	    if (a==b) encB++;
		else encB--;

//		char x[30];
//	sprintf(x,"\n\rA - a:%d, b:%d",a,b);
//	sendString(x);
}

ISR(INT3_vect)
{
		int a=0;
		int b=0;
		if (bit_is_set(PIND,2)) a = 1;
		if (bit_is_set(PIND,3)) b = 1;

	    if (a==b) encB--;
    	else encB++;

		

//	char x[30];
//	sprintf(x,"\n\rB - a:%d, b:%d",a,b);
//	sendString(x);

}

ISR(INT5_vect)
{
	int a = 0;
	int b = 0;
	if(bit_is_set(PINE,5)) a =1;
	if(bit_is_set(PINE,6)) b =1;

    if (a==b) encC--;
    else encC++;

	

}


ISR(INT6_vect)
{

	int a = 0;
	int b = 0;
	if(bit_is_set(PINE,5)) a =1;
	if(bit_is_set(PINE,6)) b =1;

    if (a==b) encC++;
    else encC--;

	
}


ISR(USART0_RX_vect)
{
	//char ReceivedByte;
	char index = 0;
	char fragment = 0;

	ReceivedByte = UDR0; // Fetch the recieved byte value into the variable "ByteReceived"

	
//	motorpos_temp |= ((long)ReceivedByte << bytecount*8);

//	if (++bytecount == 4) {
//		motorpos = motorpos_temp;
//		bytecount = 0;
//		motorpos_temp = 0;
//	}

	index = ReceivedByte >> 5;
	fragment = ReceivedByte & 0b11111;

	if (index == 0)
		bytecount = 0;
	
	if (index == bytecount) {
		motorpos_temp |= ((long)fragment << bytecount*5);

		if (++bytecount == 7) {
			motorpos = motorpos_temp;
			bytecount = 0;
			motorpos_temp = 0;
		}
	} else {
		receive_error++;
	}
   
   //UDR = ReceivedByte; // Echo back the received byte back to the computer
} 









//**************************************************************//
//  BUTTON AND LED COMMANDS										//
//																//
//**************************************************************//

void LED1(int p)
{
	if (p==0) { BCLR(PORTA,7); }
	else { BSET(PORTA,7); }
}

void LED2(int p)
{
	if (p==0) { BCLR(PORTA,6); }
	else { BSET(PORTA,6); }
}

void LED3(int p)
{
	if (p==0) { BCLR(PORTA,5); }
	else { BSET(PORTA,5); }
}

void STATUS_LED_TOGGLE()
{
//	if (status_toggle)
//		PORTE = PORTE | 0b00010000;	
//	else
//		PORTE = PORTE & 0b11101111;	
	if (status_toggle++ > 200) {
		BCLR(PORTE, 4);
	} else {
		BSET(PORTE, 4);
	}
}







//**************************************************************//
//  MAIN FUNCTIONS												//
//																//
//**************************************************************//

long getBlockerPos()
{
	long oldblocker = blocker;
	double Angle2m;

	// find offset values for all three encoders and read encoders to feed signals to EncoderA,B,C etc.

	// Use calibration to determine offset for each encoder so that 0 degrees corresponds to horizontal */
//	float tempa = (encA + offsetA)*2.0*PI;
	float EncoderA = ((encA + offsetA)*2.0*PI)/(float)cprA;
	float EncoderB = ((encB + offsetB)*2.0*PI)/(float)cprB;
	float Zm = (encC + offsetC)*25.4/cpiC;		//360 counts/inch, 2.54 cm/inch, 10mm/cm, 12cm strip

		Xm = 50.0*cos(EncoderA)+45.0*cos(EncoderA + EncoderB - PI);  //50*cos(EncoderA) + 45*cos(theta2);
		

	//	double Angle2m = acos(((50*50)+(45*45)-(Xm*Xm)-(Ym*Ym))/(2*50*45));	actual equation
	//	double Angle2m = acos((2500 + 2025 - (Xm*Xm) - (Ym*Ym)) / 4500);	simplified numbers

	if(shape == 1)		// Ellipse X-Y, linear extrusion along Z axis
	{
		if (1)
		{
			Ym = sqrt((Y_stretch*Y_stretch) - (Y_stretch*Y_stretch*(Xm-X_base)*(Xm-X_base))/(X_stretch*X_stretch)) + Y_base;
			
		}
		else
		{
			Ym = Y_base;
		}
		float temp =(2500 + 2025 - (Xm*Xm) - (Ym*Ym)) / 4500.0;
		Angle2m = acos(temp);
	}


	else if(shape == 2)		// Ellipse Z-Y, linear extrusion along X axis
	{
		Ym = sqrt((Y_stretch*Y_stretch) - (Y_stretch*Y_stretch*(Zm-Z_base)*(Zm-Z_base))/(Z_stretch*Z_stretch)) + Y_base;
		float temp = (2500 + 2025 - (Xm*Xm) - (Ym*Ym)) / 4500.0;
		Angle2m = acos(temp);
	}


	else if(shape == 3)		// X-Z Plane at Ym=60mm above base point
	{
		Ym = flatHeight;
		float temp = (2500 + 2025 - (Xm*Xm) - (Ym*Ym)) / 4500.0;
		Angle2m = acos(temp);

	}

	else if(shape == 4)		// Sine Bump X-Y
	{
		//double Ym;

		if(Xm >= -20 && Xm <= 20 && Zm >= (Z_base-20) && Zm <= (Z_base+20))
			{	
				Ym = flatHeight+sineHeight + sineHeight*cos(sqrt(Xm*Xm+(Zm-Z_base)*(Zm-Z_base)) * PI / 20.0);
			}
		else
			{	
				Ym = flatHeight;
			}
		float temp = (2500 + 2025 - (Xm*Xm) - (Ym*Ym)) / 4500.0;
		Angle2m = acos(temp);

		//char x[128];
		//sprintf(x,"Enc A: %d  Enc B: %d  Enc C: %d  Blocker: %ld  Xm: %d  Ym: %d      " ,(encA*360/cprA), (encB*360/cprB), encC, (blocker*360/motorCpr), (int)Xm, (int)Ym);
	    //sendString("\r");
		//sendString(x);

	}
	else if(shape == 5)		// Sine Bump Y-Z
	{
		//double Ym;

		if(Zm >= (Z_base-20) && Zm <= (Z_base+20))
			{	
				Ym = 40 + 10*cos((Zm-Z_base) * PI / 20.0);
			}
		else
			{	
				Ym = 30;
			}
		float temp = (2500 + 2025 - (Xm*Xm) - (Ym*Ym)) / 4500.0;
		Angle2m = acos(temp);

		//char x[128];
		//sprintf(x,"Enc A: %d  Enc B: %d  Enc C: %d  Blocker: %ld  Xm: %d  Ym: %d      " ,(encA*360/cprA), (encB*360/cprB), encC, (blocker*360/motorCpr), (int)Xm, (int)Ym);
	    //sendString("\r");
		//sendString(x);

	}

	else if(shape == 6)		// 3D Ellipse, linear extrusion along X axis
	{
		Ym = sqrt((Y_stretch*Y_stretch) - (Y_stretch*Y_stretch*(Zm-Z_base)*(Zm-Z_base))/(Z_stretch*Z_stretch) - (Y_stretch*Y_stretch*(Xm-X_base)*(Xm-X_base))/(X_stretch*X_stretch)) + Y_base;
		float temp = (2500 + 2025 - (Xm*Xm) - (Ym*Ym)) / 4500.0;
		Angle2m = acos(temp);
	}
	else
	{
		Angle2m = 0;	
	}

	if (Angle2m > PI) Angle2m = PI;
	if (Angle2m < 0) Angle2m = 0;

	long pos = (Angle2m/2.0/PI)*motorCpr + motorOffset - blockerOffset;
	//char x[100];
	//sprintf(x,"Angle: %d, motor: %d\n\r",(int)(Angle2m*100),pos);	
	//sendString(x);


	if (pos < 1 || pos > motorCpr) pos = oldblocker;

	return pos;


}

void selectShape()
{

	//Disengage Motor:
	//setVal16(0x6040,0x00,0x000F,&message);
	setVal16(0x6040,0x00,0x0000,&message);

	if (checkFault(&message)) faultState();

//	int startTime = TCNT1
	TCNT1 = 0;
	int LEDon = 0;
	int LEDnum = 1;

	int selected = 0;
	while(selected == 0)
	{
		if (TCNT1>2000 && selected == 0) {
			TCNT1 = 0;

			LED1(0);
			LED2(0);
			LED3(0);
			if (LEDnum==1) LED1(1);
			else if (LEDnum==2) LED2(1);
			else if (LEDnum==3) LED3(1);
			
			if (LEDnum>3) LEDnum=1;
			else LEDnum++;

		}

		if (bit_is_set(PINC,0)==0)
		{
			sendString("A\n");
			if (dimension ==2) shape = 1;
			else shape = 6;
			PORTA = 0x00;
			_delay_ms(100);
			LED1(1);
			selected = 1;
		}

		if (bit_is_set(PINC,3)==0)
		{
			sendString("B\n");
			shape = 3;
			PORTA = 0x00;
			_delay_ms(100);
			LED2(1);
			selected = 1;
		}

		if (bit_is_set(PINC,2)==0)
		{
			sendString("C\n");
			shape = 4;
			//if (dimension == 2) shape = 4;
			//else shape = 5;
			PORTA = 0x00;
			_delay_ms(100);
			LED3(1);
			selected = 1;
		}
		else {
			//BCLR(PINA,5);
		}

		if (bit_is_set(PINC,1)) quit();
//		if (bit_is_set(PINC,5)==0) sendString("START!");
	}

	blocker = getBlockerPos();

	//Clear Faults
	setVal16(0x6040,0x00,0x0080,&message);
	_delay_ms(500);

	//Engage Motor
	setVal16(0x6040,0x00,0x0006,&message);
	_delay_ms(500);

	//Enable Following:
	setVal16(0x6040,0x00,271,&message);

}










//**************************************************************//
//  MAIN FUNCTION												//
//																//
//**************************************************************//

int main(void)
{

	//UART_Init(38400);
	UART_Init(57600);
	//UART_Init(115200); //doesn't work =(
	interruptInit();
	canBusInit();
	
	_delay_ms(500);
	//sendString("\n\rSerial Init");

//	initButtons();


	//sendString("\n\rDevice is Ready");

//	while (bit_is_set(PINC,5))
//	{
//		if (bit_is_set(PINC,1)) quit();
//	}

	encA = 0;
	encB = 0;
	encC = 0;

	//sendString("\n\rInitializing...");	

	
	//if (debug) sendString("\n\rInit\n\r");

	//Setup timer with 1024 prescaler: (64us per tick)101
	BSET(TCCR1B,CS12);
	BCLR(TCCR1B,CS11);
	BSET(TCCR1B,CS10);

//	CAN Stuff
	//Set EPOS to Operational Mode
	setOpMode(&message);
	//Turn on:
	setVal16(0x6040,0x00,3,&message);

	//Set to Profile Position Mode
	setVal8(0x6060,0x00,Mode, &message);
	//Initialize Constants:
		//Position Regulator P-Gain:
		setVal16(0x60FB,0x01,position_Pgain,&message);
		//Position Regulator I-Gain:
		setVal16(0x60FB,0x02,position_Igain,&message);

	LED1(1);
	//sendString("1...");

		//Position Regulator D-Gain:
		setVal16(0x60FB,0x03,position_Dgain,&message);
		//Continuous Current Limit:
		setVal16(0x6410,0x01,continuousCurrentLimit,&message);
		//Output Current Limit:
		setVal16(0x6410,0x02,outputCurrentLimit,&message);
		//Min Position Limit:
		setVal32(0x607D,0x01,minPositionLimit,&message);
		//Max Position Limit:
		setVal32(0x607D,0x02,maxPositionLimit,&message);

	LED2(1);
	_delay_ms(500);
	//sendString("2...");


		//Max Profile Velocity:
		setVal32(0x607F,0x00,maxProfileVelocity,&message);
		//Profile Velocity:
		setVal32(0x6081,0x00,profileVelocity,&message);
		//Profile Acceleration:
		setVal32(0x6083,0x00,profileAcceleration,&message);
		//Profile Deceleration:
		setVal32(0x6084,0x00,profileDeceleration,&message);
		//Maximum Following Error:
		setVal32(0x6065,0x00,maxFollowingError,&message);
		//Profile Type:
		//setVal16(0x6086,0x00,profileType,&message);

		//Position Demand Value:
		if (Mode == -1) posAddress = 0x2062;
		if (Mode == 1) posAddress = 0x6062;
		
		setVal32(posAddress,0x00,90000,&message);
		

	LED3(1);
	_delay_ms(500);
	//sendString("3...");

	LED1(0);
	LED2(0);
	LED3(0);
	_delay_ms(1000);


	int z = 0;

	//sendString("\n\rSelect Shape...");
	//selectShape();
	//blocker = getBlockerPos();

	//Clear Faults
	setVal16(0x6040,0x00,0x0080,&message);
	_delay_ms(500);

	//Engage Motor
	setVal16(0x6040,0x00,0x0006,&message);
	_delay_ms(500);

	//Enable Following:
	setVal16(0x6040,0x00,271,&message);

	//sendString("\n\rReady\n\r");



	//ACTUAL CODE:
    while(1)
    {
		STATUS_LED_TOGGLE();
		//Check Error State and Reset counter after 1s
		//if (TCNT1 > (15625*2))
		//{
		//	if (checkFault(&message)) faultState();
		//	TCNT1 = 0;
		//}

		//char x[128];
		
		//blocker = getBlockerPos();
	//	_delay_ms(100);

		
		debug = 0;
		//Position Demand Value:
		if (blocker != motorpos) {
			blocker = motorpos;
			if ((blocker > 80000) & (blocker < 180000))
				setVal32(posAddress,0x00,blocker,&message);		//Profile position = 0x607a, 0x00
		}
		//int block = ((long)(blocker - motorOffset + blockerOffset)*360/motorCpr);	

		//char x[128];
		char encbyte[6];
		char bytetosend[8];
		char encvalues[8];
		//sprintf(x,"A:%+.4d B:%+.4d C:%+.4d Block:%+3.d X:%+2.d Y:%+2.d Z:%+2.d  \r",/*((encA+offsetA)/cprA))*360, (encB+offsetB)*360/cprB,*/(encA+offsetA),(encB+offsetB), (encC+offsetC), block, (int)Xm, (int)Ym, (int)((encC+offsetC)*25.4/cpiC)); //(Angle2m/2/PI)*motorCpr + motorOffset - blockerOffset;
		//unsigned char pos1 = (unsigned char) (motorpos >> 24);
		//unsigned char pos2 = (unsigned char) (motorpos >> 16);
		//unsigned char pos3 = (unsigned char) (motorpos >> 8);
		//unsigned char pos4 = (unsigned char) (motorpos >> 0);
		//sprintf(x,"A:%+.4d hex:0x%02X%02X%02X%02X byte:%02X bytecount:%i pos:%li er:%i \r", encA+offsetA, pos1,pos2,pos3,pos4, ReceivedByte, bytecount, motorpos, receive_error);
		//sprintf(x,"A:%+.4d byte:%c \r", encA+offsetA, ReceivedByte);
		//sprintf(x,"Blocker: %+3.d  Xm: %+2.d  Ym: %+2.d  Zm:%+2.d       \r", block, (int)Xm, (int)Ym*10, (int)((encC+offsetC)*25.4/cpiC));
		//sendString("\r\n");
		//sendString(x);
		int enc12 = encA + offsetA;
		int encM = encB + offsetB;
		int enc5 = encC + offsetC;
		
		encbyte[0] = (enc5 >> 0);
		encbyte[1] = (enc5 >> 8);
		encbyte[2] = (encM >> 0);
		encbyte[3] = (encM >> 8);
		encbyte[4] = (enc12 >> 0);
		encbyte[5] = (enc12 >> 8);
		
		bytetosend[0] = (0b000 << 5) | (encbyte[0] >> 3);
		bytetosend[1] = (0b001 << 5) | ((encbyte[0] & 0b111) << 2) | ((encbyte[1] >> 2) & 0b11);
		bytetosend[2] = (0b010 << 5) | ((encbyte[1] & 0b11) << 3) | (encbyte[2] >> 5);
		bytetosend[3] = (0b011 << 5) | (encbyte[2] >> 3);
		bytetosend[4] = (0b100 << 5) | ((encbyte[3] & 0b1111) << 1) | (encbyte[4] >> 7);
		bytetosend[5] = (0b101 << 5) | ((encbyte[4] >> 2) & 0b11111);
		bytetosend[6] = (0b110 << 5) | ((encbyte[4] & 0b11) << 3) | ((encbyte[5] >> 3) & 0b111);
		bytetosend[7] = (0b111 << 5) | ((encbyte[5] & 0b111) << 2);
			

		sprintf(encvalues, "%c%c%c%c%c%c%c%c", bytetosend[0],bytetosend[1],bytetosend[2],bytetosend[3],bytetosend[4],bytetosend[5],bytetosend[6],bytetosend[7]);
		sendString(encvalues);
		//sprintf(x, "sizeofint:%i \r", sizeof(encA));
		//sendString(x);




	   	//_delay_ms(100);
//	  sprintf(x,"A:%d B:%d C:%d A-B:%d B-C%d\n\r" ,a,b,c,b-a,c-b);	
//	  sendString(x);
//

		//if (bit_is_set(PINC,5)==0) selectShape();
		//if (bit_is_set(PINC,1)) quit();

    }

	quit();
    return 0;
}
