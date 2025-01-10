/****************************************************/
/* This is only one example of code structure       */
/* OFFCOURSE this code can be optimized, but        */
/* the idea is let it so simple to be easy catch    */
/* where can do changes and look to the results     */
/****************************************************/
//set your clock speed
//#define F_CPU 16000000UL
//these are the include files. They are outside the project folder
#include <avr/io.h>
//#include <iom1284p.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define VFD_in 8// If 0 write LCD, if 1 read of LCD
#define VFD_clk 9 // if 0 is a command, if 1 is a data0
#define VFD_stb 10 // Must be pulsed to LCD fetch data of bus

#define AdjustPins    PIND // before is C, but I'm use port C to VFC Controle signals

unsigned char DigitTo7SegEncoder(unsigned char digit, unsigned char common);

/*Global Variables Declarations*/
unsigned char day = 7;  // start at 7 because the VFD start the day on the left side and move to rigth... grid is reverse way
unsigned char hours = 0;
unsigned char minutes = 0;
unsigned char seconds = 0;
unsigned char milisec = 0;
unsigned char points = 0;
unsigned char twoPoints = 0x00;

unsigned char secs;

unsigned char digit;
unsigned char number;
unsigned char numberA;
unsigned char numberB;
unsigned char numberC;
unsigned char numberD;
unsigned char numberE;
unsigned char numberF;

unsigned char wordA = 0;
unsigned char wordB = 0;
unsigned char wordC = 0;
unsigned char wordD = 0;
unsigned char wordE = 0;
unsigned char wordF = 0;

unsigned int val = 0;
//Please pay attention to the diferences on first digit and second digit of 
//same grid. They are total different segments allocated on weight of the Byte.
//By this reason I solve the difficult creating a second table to draw number's.
unsigned int segmentsA[] ={
  //  --habfgced  // This is the corresponding to the 0:7 of 7 seg digit of address 0 & 4 
      0b01110111, //0  // 
      0b00100100, //1  //   
      0b01101011, //2  // 
      0b01101101, //3  // 
      0b00111100, //4  // 
      0b01011101, //5  // 
      0b01011111, //6  // 
      0b01100100, //7  // 
      0b01111111, //8  // 
      0b01111100, //9  //
      0b00000000 //10 // empty display
  };

unsigned int segmentsB[] ={
  //  --gfbacedh  // This is the corresponding to the 0:7 of 7 seg digit of address 1 & 5 
      0b01111110, //0  // 
      0b00101000, //1  //   
      0b10110110, //2  //  
      0b10111010, //3  // 
      0b11101000, //4  // 
      0b11011010, //5  // 
      0b11011110, //6  // 
      0b00111000, //7  // 
      0b11111110, //8  // 
      0b11111000, //9  // 
      0b00000000 //10  //   // empty display
  };

void pt6312_init(void){
  delayMicroseconds(1000); //power_up delay
  // Note: Allways the first byte in the input data after the STB go to LOW is interpret as command!!!

  // Configure VFD display (grids)
  cmd_with_stb(0b00000001);//  (0b01000000)    cmd1 5 grids 16 segm
  delayMicroseconds(1);
  // Write to memory display, increment address, normal operation
  cmd_with_stb(0b01000000);//(BIN(01000000));
  delayMicroseconds(1);
  // Address 00H - 15H ( total of 11*2Bytes=176 Bits)
  cmd_with_stb(0b11000000);//(BIN(01100110)); 
  delayMicroseconds(1);
  // set DIMM/PWM to value
  cmd_with_stb((0b10001000) | 7);//0 min - 7 max  )(0b01010000)
  delayMicroseconds(1);
}
void cmd_without_stb(unsigned char a){
  // send without stb
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  //This don't send the strobe signal, to be used in burst data send
   for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
     digitalWrite(VFD_clk, LOW);
     if (data & mask){ // if bitwise AND resolves to true
        digitalWrite(VFD_in, HIGH);
     }
     else{ //if bitwise and resolves to false
       digitalWrite(VFD_in, LOW);
     }
    delayMicroseconds(5);
    digitalWrite(VFD_clk, HIGH);
    delayMicroseconds(5);
   }
   //digitalWrite(VFD_clk, LOW);
}
void cmd_with_stb(unsigned char a){
  // send with stb
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  
  //This send the strobe signal
  //Note: The first byte input at in after the STB go LOW is interpreted as a command!!!
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(1);
   for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
     digitalWrite(VFD_clk, LOW);
     delayMicroseconds(1);
     if (data & mask){ // if bitwise AND resolves to true
        digitalWrite(VFD_in, HIGH);
     }
     else{ //if bitwise and resolves to false
       digitalWrite(VFD_in, LOW);
     }
    digitalWrite(VFD_clk, HIGH);
    delayMicroseconds(1);
   }
   digitalWrite(VFD_stb, HIGH);
   delayMicroseconds(1);
}
void testSegments(void){
  /* 
  Here do a test for all segments of 5 grids
  each grid is controlled by a group of 2 bytes
  by these reason I'm send a burst of 2 bytes of
  data. 
  */
  clear_VFD();
      
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(0b00000001); // cmd 1 // 5 Grids & 16 Segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
        
         for (int i = 0; i < 5 ; i++){ // test base to 16 segm and 5 grids
         cmd_without_stb(0b11111111); // Data to fill table 5*16 = 80 bits
         cmd_without_stb(0b11111111); // Data to fill table 5*16 = 80 bits
         }
    
      //cmd_without_stb(0b00000001); // cmd1 Here I define the 5 grids and 16 Segments
      cmd_with_stb((0b10001000) | 7); //cmd 4
      digitalWrite(VFD_stb, HIGH);
      delay(1);
      delay(3000);  
}
void clear_VFD(void){
  /*
  Here I clean all registers 
  Could be done only on the number of grid
  to be more fast. The 12 * 2 bytes = 24 registers
  */
      for (int n=0; n < 10; n++){  // important be 10, if not, bright the half of wells./this on the VFD of 5 grids)
        cmd_with_stb(0b00000001); //       cmd 1 // 5 Grids & 16 Segments
        cmd_with_stb(0b01000000); //       cmd 2 //Normal operation; Set pulse as 1/16
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
            cmd_without_stb((0b11000000) | n); // cmd 3 //wich define the start address (00H to 15H)
            cmd_without_stb(0b00000000); // Data to fill table of 5 grids * 16 segm = 80 bits on the table
            //
            //cmd_with_stb((0b10001000) | 7); //cmd 4
            digitalWrite(VFD_stb, HIGH);
            delayMicroseconds(100);
     }
}
void testWeels(void){
  /* 
  Here do a test for all segments of 5 grids
  each grid is controlled by a group of 2 bytes
  by these reason I'm send a burst of 2 bytes of
  data. 
  */
 
  //clear_VFD();
      for(uint8_t i=0; i < 3; i++){
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(0b00000001); // cmd 1 // 5 Grids & 16 Segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11001000)); //cmd 3 wich define the start address (00H to 15H)
        
         
         cmd_without_stb(0b00100000 << i); // Weels stay at bit 5,6,7 of first byte.
         cmd_without_stb(0b00000000); // 
        
    
      //cmd_without_stb(0b00000001); // cmd1 Here I define the 5 grids and 16 Segments
      //cmd_with_stb((0b10001000) | 7); //cmd 4
      digitalWrite(VFD_stb, HIGH);
      delay(1);
      delay(150);
      }  
}

/******************************************************************/
/************************** Update Clock **************************/
/******************************************************************/
void send_update_clock(void){
  if (secs >=60){
    secs =0;
    minutes++;
  }
  if (minutes >=60){
    minutes =0;
    hours++;
  }
  if (hours >=24){
    hours =0;
  }
    //*************************************************************
    number = (secs%10);
    //Serial.println(secs, DEC);
    numberA=number;
    number = (secs/10);
    //Serial.println(secs, DEC);
    numberB=number;
    SegTo32Bits();
    //*************************************************************
    number=(minutes%10);
    numberC=number;
    number = (minutes/10);
    numberD=number;
    SegTo32Bits();
    //**************************************************************
    number = (hours%10);
    numberE=number;
    number = (hours/10);
    numberF=number;
    SegTo32Bits();
    //**************************************************************
}
void SegTo32Bits(){
  // This block is very important, it solve the difference 
  // between segments from grid 1 and grid 2(start 8 or 9)
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(10);
      cmd_with_stb(0b00000001); // cmd 1 // 5 Grids & 16 Segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(10);
        cmd_without_stb((0b11000000) | 4); //cmd 3 wich define the start address (00H to 15H)
        wordA=segmentsA[numberA];
        wordB=segmentsB[numberB];
        wordC=segmentsA[numberC];
        wordD=segmentsB[numberD];
        wordE=segmentsA[numberE];
        wordF=segmentsB[numberF];
            
          //cmd_without_stb(wordA); // seconds unit  
          //cmd_without_stb(wordB ); // seconds dozens 
          cmd_without_stb(wordC );   // Minuts unit
          cmd_without_stb(wordD | twoPoints);   // Minuts dozens // I axctivate the bit with weight 1 of second memory address to fill colon symbol
          cmd_without_stb(wordE);   // Hours unit
          cmd_without_stb(wordF);   // Hours dozens
           //If you not need use the colon, use only: cmd_without_stb(wordD); and comment the line with "twoPoints" at trigger (ISR) zone code!
      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(10);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delay(5);      
}
// void adjustHMS(){
//  // This function allow using 3 buttons connected to pins: 3, 4 & 5 to allow adjust the
//  // clock if you don't want use or don't have buttons connected to AD16312
//  // Important is necessary activate  internal pull-up or make use of resistor to the VCC(+5VDC) to this pins (3, 4, 5)
//  // if dont want adjust of the time comment the call of function on the loop
//   /* Reset Seconds to 00 Pin number 3 Switch to GND*/
//     if((AdjustPins & 0x08) == 0 )
//     {
//       _delay_ms(200);
//       secs=00;
//     }
    
//     /* Set Minutes when SegCntrl Pin 4 Switch is Pressed*/
//     if((AdjustPins & 0x10) == 0 )
//     {
//       _delay_ms(200);
//       if(minutes < 59)
//       minutes++;
//       else
//       minutes = 0;
//     }
//     /* Set Hours when SegCntrl Pin 5 Switch is Pressed*/
//     if((AdjustPins & 0x20) == 0 )
//     {
//       _delay_ms(200);
//       if(hours < 23)
//       hours++;
//       else
//       hours = 0;
//     }
// }
// 
void readButtons(){
  int val = 0;       // variable to store the read value
  byte array[8] = {0,0,0,0,0,0,0,0};
  unsigned char mask = 1; //our bitmask
  array[0] = 1;
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(2);
  cmd_without_stb(0b01000010); // cmd 2 //Read Keys;Normal operation; Set pulse as 1/16  
  pinMode(8, INPUT);  // Important this point! Here I'm changing the direction of the pin to INPUT data.
  delayMicroseconds(2);
  //PORTD != B01010100; // this will set only the pins you want and leave the rest alone at
  //their current value (0 or 1), be careful setting an input pin though as you may turn 
  //on or off the pull up resistor  
  //This don't send the strobe signal, to be used in burst data send
         for (int z = 0; z < 3; z++){
                   for (int h =8; h > 0; h--) {
                      digitalWrite(VFD_clk, HIGH);  // Remember wich the read data happen when the clk go from LOW to HIGH! Reverse from write data to out.
                      delayMicroseconds(2);
                     val = digitalRead(VFD_in);
                           if (val & mask){ // if bitwise AND resolves to true
                            array[h] = 1;
                           }
                           else{ //if bitwise and resolves to false
                           array[h] = 0;
                           }
                    digitalWrite(VFD_clk, LOW);
                    delayMicroseconds(2);
                   } 
             
              Serial.print(z);
              Serial.print(" - " );
                        
                                  for (int bits = 7 ; bits > -1; bits--) {
                                      Serial.print(array[bits]);
                                   }
                        
                        if (z==0){
                          if(array[6] == 1){
                           hours++;
                          }
                        }
                          if (z==0){
                          if(array[5] == 1){
                           hours--;
                          }
                          }
                          if (z==1){
                          if(array[2] == 1){
                           minutes++;
                          }
                        }
                        if (z==0){
                          if(array[2] == 1){
                           minutes--;
                          }
                        }
                        if (z==0){
                          if(array[1] == 1){
                           secs++;
                          }
                        }
                          if (z==1){
                            if(array[1] == 1){
                              hours = 0;
                              minutes = 0;
                             secs=0;  // Set count of secs to zero to be more easy to adjust with other clocl.
                            }
                          }
                         
                  Serial.println();
          }  // End of "for" of "z"
      Serial.println();

 digitalWrite(VFD_stb, HIGH);
 delayMicroseconds(2);
 cmd_with_stb((0b10001000) | 7); //cmd 4
 delayMicroseconds(2);
 pinMode(8, OUTPUT);  // Important this point!  // Important this point! Here I'm changing the direction of the pin to OUTPUT data.
 delay(1); 
}
void rnd(){
  uint8_t r1 = random(0xFF);
  uint8_t r2 = random(0xFF);
  uint8_t adr = 0x00;
  for(uint8_t i = 0x00; i < 8; i++){
    r1 = random(255);
    r2 = random(255);
    adr = 0x00;   //Position of memory 00 to write digit 0
    equalizerDummy(r1, r2, adr);
    r1 = random(255);
    r2 = random(255);
    adr = 0x02;   //Position of memory 00 to write digit 1
    equalizerDummy(r1, r2, adr);
  }
}
void equalizerDummy(uint8_t bar1, uint8_t bar2, uint8_t adr){
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(0b00000001); // cmd 1 // 5 Grids & 16 Segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11000000 | adr)); //cmd 3 wich define the start address (00H to 15H)
        cmd_without_stb(bar1); //
        cmd_without_stb(bar2); // 
    
      digitalWrite(VFD_stb, HIGH);
      delay(100);  
}
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  seconds = 0x00;
  minutes =0x00;
  hours = 0x00;

  /*CS12  CS11 CS10 DESCRIPTION
  0        0     0  Timer/Counter1 Disabled 
  0        0     1  No Prescaling
  0        1     0  Clock / 8
  0        1     1  Clock / 64
  1        0     0  Clock / 256
  1        0     1  Clock / 1024
  1        1     0  External clock source on T1 pin, Clock on Falling edge
  1        1     1  External clock source on T1 pin, Clock on rising edge
 */
  // initialize timer1 
  cli();           // disable all interrupts
  // initialize timer1 
  //noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;// This initialisations is very important, to have sure the trigger take place!!!
  TCNT1  = 0;
  // Use 62499 to generate a cycle of 1 sex 2 X 0.5 Secs (16MHz / (2*256*(1+62449) = 0.5
  OCR1A = 62498;            // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= ((1 << CS12) | (0 << CS11) | (0 << CS10));    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
 
  
  // Note: this counts is done to a Arduino 1 with Atmega 328... Is possible you need adjust
  // a little the value 62499 upper or lower if the clock have a delay or advnce on hours.
    
  //  a=0x33;
  //  b=0x01;

  CLKPR=(0x80);
  //Set PORT
  DDRD = 0xFF;  // IMPORTANT: from pin 0 to 7 is port D, from pin 8 to 13 is port B
  PORTD=0x00;
  DDRB =0xFF;
  PORTB =0x00;

  pt6312_init();

  clear_VFD();

  //only here I active the enable of interrupts to allow run the test of VFD
  //interrupts();             // enable all interrupts
  sei();
}
void loop() {
  // You can comment untill while cycle to avoid the test running.
  clear_VFD();
  testSegments();
  clear_VFD();
  for(uint8_t n = 0; n < 12; n++){
     rnd();
  } 
 
  clear_VFD();
  //
   while(1){
        send_update_clock();
        delay(200);
        readButtons();
        delay(200);
        testWeels();
   }
}

ISR(TIMER1_COMPA_vect)   {  //This is the interrupt request
                            // https://sites.google.com/site/qeewiki/books/avr-guide/timers-on-the-atmega328
      secs++;
      twoPoints = !twoPoints;  // Comment this line to don't use the twoPoints bright.
} 
