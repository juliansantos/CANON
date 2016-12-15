#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */
#include <stdio.h>
#include <math.h>
// Preprocessor directives for PORT definition of a GLCD

#define GLCD_Enable PTFD_PTFD5 // LCD Enable
#define TRIS_Enable PTFDD_PTFDD5 // LCD Enable
#define GLCD_RS PTFD_PTFD1// LCD command/data mode
#define TRIS_RS PTFDD_PTFDD1
#define GLCD_BUS PTED //GLCD bus
#define TRIS_BUS PTEDD

// Preprocessor directives for SPECIFIC PIN function
#define FAN PTCD_PTCD2
#define TRIS_FAN PTCDD_PTCDD2

//Preprocessor directives {commands used for the controller of GLCD} 

#define cmd_clear 0x01 // Command to clear the display
#define cmd_8bitmode 0x38 // Command to set parallel mode at 8 bits
#define cmd_line1 0x80 // Command to set the cursor in the line 1
#define cmd_line3 0x88 // Command to set the cursor in the line 3
#define cmd_line2 0x90 // Command to set the cursor in the line 2
#define cmd_line4 0x98  // Command to set the cursor in the line 4
#define cmd_displayON 0x0C // Command to turn on the display
#define cmd_displayOFF 0x80 // Command to turn off the display
#define cmd_home 0x2 //set the cursor in the initial position

//Signature of the functions used in the program
void delayAx5ms(unsigned char var_delay); //Signature process for delay creation
void glcd_instruction(unsigned char instruction) ; //Signature process for instruction sending
void glcd_data(unsigned char data); //Signature process for data (1 character) sending
void glcd_init(void); //Signature process for GLCD initialization
void glcd_message(unsigned char message[]);  //Signature of process for send a string to GLCD
void init_ADC(void); // Signature of process to initialize the ADC
void mcu_config(void); // Signature of process to initialize the MCU
void init_PWM(void);
//void init_SCI(void);
void sampling_ADC(void); // Signature of process to sampling data
void show_data(void); // Signature of process to show the sampled data
void decision_data(void);
// Variable definition SECTION
float temperature,joystick_X,joystick_Y,SW,solar_panel;
unsigned char temporal_var;
unsigned char temp[7]="";
unsigned char temp2[7]=""; // temporal variable to send data to GLCD
int alfa,gamma;
double temporal;

void main(void) {
	mcu_config();
	init_ADC();
	init_PWM();
	//init_SCI();
	glcd_init();

    for(;;) {  
    	sampling_ADC();
    	decision_data();
    	show_data();
    } 
}

void mcu_config(void){ /* Process to initialize the MCU*/ 
	SOPT1 = 0;  // Turn off WDT
	TRIS_FAN = 1 ; // Data direction as output (Fan pin)
	FAN = 0; // Fan OFF
}

void init_PWM(void){
	TPM1SC = 0x03;
	TPM1MOD = 255; 
	TPM1C2SC =	0b00101000;
	TPM1C2V = 128;
	TPM1SC_CLKSA = 1;
}

void init_ADC(void){ /* Process to initialize the ADC*/  
	ADCCFG = 0b01110001; // High speed, Long sample, input clock/8 ,8 bit resolution ADC, Bus clock/2;
	APCTL1 = 0b00011111; // Channel selection
	//CH0 => X
	//CH1 => Y
	//CH2 => SW
	//CH3 => Temperature sensor
	//CH4 => Solar Panel
	ADCSC1 = 0b00000000; // 
}

void glcd_init(void){ /* Process to initialize the GLCD*/  
	TRIS_BUS = 0xFF;  //Setting data direction of GLCD data bus
	TRIS_RS = 1; // Setting data direction of  RS GLCD control pin 
	TRIS_Enable=1; // Setting data direction of Enable GLCD control pin
	delayAx5ms(8); // For Power up	
	glcd_instruction(cmd_8bitmode); // 8 bit operation 
	glcd_instruction(cmd_displayON); // Turn on the display
	glcd_instruction(cmd_clear); // Clearing display
	glcd_instruction(cmd_line1); // Setting cursor at first line
	glcd_message("XOXOXOXOXOXOXOXO");
	
	glcd_instruction(cmd_line2); // Setting cursor at second line+    
	glcd_message(" Delta=");
	
	glcd_instruction(cmd_line3); // Setting cursor at second line+    	
	glcd_message(" Gamma=");
}

void glcd_instruction(unsigned char instruction){ /* Process to send a instruction towards GLCD*/  
	GLCD_RS=0;	
	GLCD_BUS=instruction;
	GLCD_Enable=1;
	delayAx5ms(1);
	GLCD_Enable=0;
	delayAx5ms(1);
}

void glcd_data(unsigned char data){ /* Process to send one character to the GLCD*/  
	GLCD_RS=1;
	GLCD_BUS=data;
	GLCD_Enable=1;
	delayAx5ms(1);
	GLCD_Enable=0;
	delayAx5ms(1);
}

void glcd_message(unsigned char message[]){ /* Process to send a string to the GLCD*/  
	int size  = sizeof(message)/1;
	int counter=0;
	for(counter=0;message[counter]!='\0'; counter++){
		glcd_data(message[counter]);
	}
}

void delayAx5ms(unsigned char var_delay){  // Process of delay creation *DEVELOPED BY JULIAN SANTOS*
	asm{
		PSHH ; //save context H
		PSHX ; //save context X
		PSHA ; //save context A
		LDA var_delay ; // 2 cycles
		delay_2:    LDHX #$1387 ; //3 cycles 
		delay_1:    AIX #-1  //cycles
		CPHX #0 ; //3 cycles  
		BNE delay_1 ; // 3 cycles
		DECA ; //1 cycle
		CMP #0 ; // 2 cycles
		BNE delay_2  ; //3 cycles
		PULA ; // restore context A
		PULX ; // restore context X
		PULH ; // restore context H
	}
}

void sampling_ADC(void){
	ADCSC1 = 0b00000000; // Input channel 0
	while(ADCSC1_COCO == 0); // Waiting for end of conversion 
	joystick_X=ADCR;
	joystick_X=joystick_X*5/255;
	ADCSC1 = 0b00000001; // Input channel 1
	while(ADCSC1_COCO == 0); // Waiting for end of conversion 
	joystick_Y=ADCR;
	joystick_Y=joystick_Y*5/255;
	ADCSC1 = 0b00000010; // Input channel 2
	while(ADCSC1_COCO == 0); // Waiting for end of conversion 
	SW = ADCR;
	SW = SW*5/255;
	ADCSC1 = 0b00000011; // Input channel 3
	while(ADCSC1_COCO == 0); // Waiting for end of conversion 
	temperature = ADCR;
	temperature = temperature*100*5/255;
	ADCSC1 = 0b00000100; // Input channel 4
	while(ADCSC1_COCO == 0); // Waiting for end of conversion 
	solar_panel = ADCR;
	solar_panel = solar_panel*5/255;
}

void show_data(void){
	glcd_instruction(cmd_line2+4); // Setting cursor at second line+    
	sprintf(temp2,"%i",alfa);		
	glcd_message(temp2);
	glcd_message(" ");
	glcd_data(0x09); // Symbol Degree
	glcd_message(" ");

	glcd_instruction(cmd_line3+4); // Setting cursor at second line+    
	sprintf(temp2,"%i",gamma);		
	glcd_message(temp2);
	glcd_message(" ");
	glcd_data(0x09); // Symbol Degree
	glcd_message(" ");
}

void decision_data(void){
	temporal_var=(unsigned char)(solar_panel*51);
	TPM1C2V=255-temporal_var;
	
	if(temperature>30){
		FAN = 1;
	}
	else {
		FAN = 0;
	}
	
	if(joystick_Y>4.5){
			alfa ++;
			alfa =(alfa>360)?(360):(alfa);
	}
	else if(joystick_Y<0.1) {
			alfa --;
			alfa = (alfa<1)?(0):(alfa);
	}
	
	if(joystick_X>4.5){
			gamma ++;
			gamma =(gamma>90)?(90):(gamma);
	}
	else if(joystick_X<0.1) {
			gamma --;
			gamma = (gamma<1)?(0):(gamma);
	}
	
	if(SW<1){
		glcd_instruction(cmd_line4); // Setting cursor at second line+    	
		glcd_message("      FUEGO!");
		delayAx5ms(200);
	    delayAx5ms(200);
	    glcd_instruction(cmd_line4); // Setting cursor at second line+    	
	    glcd_message("            ");
	    //send_coordinates;
	}

}
