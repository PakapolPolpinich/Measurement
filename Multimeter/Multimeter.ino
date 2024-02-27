#define R1_V 2000
#define R2_V 10000
#define R2_R 45000

#include <Wire.h> //I2C
#include <Adafruit_GFX.h>    //library      
#include <Adafruit_SSD1306.h>
//#define SCREEN_WIDTH 128 
//#define SCREEN_HEIGHT 32
//#define OLED_RESET 4 //reset pin 
Adafruit_SSD1306 display(128, 32, &Wire, 4);

uint8_t MODE = 0;
bool buttom = 0;
float Measure[4] = { 0, 0, 0, 0 };

/////////////////////////////////////ADC part
void Adc_init() {
  ADMUX = 0x40;   
  ADCSRA = 0x87;  //set adc enable and set prescaler
}
uint16_t Adc_read(char ch) {
  char busy;
  ADMUX &= ~(0xF << MUX0);  //set pin0-3 bit x
  ADMUX |= ch;
  ADCSRA |= (1 << ADSC);  //start conversation ADSC
  while (ADCSRA & (1 << ADSC));   //check ADSC is 0 mean their finish convert  
  return (ADC);  //return digital
}
/////////////////////////////////////Debounce&External
uint8_t debounceButton(uint8_t pin) {
  if (!(PIND & (1 << pin))) {  // Check if button is pressed (active-low)
    //_delay_ms(DEBOUNCE_DELAY); // Wait for debounce delay
    delay(200);
    if (!(PIND & (1 << pin))) {  // Check if button is still pressed
      return 0;                  // Button is pressed
    }
  }
  return 1;  // Button is not pressed
}
void External_init() {
  EICRA = 0x0A;  //set falling INT0,1
  EIMSK = 0x03;  //enable INT0,1
  sei();
}

ISR(INT0_vect) {
  if (debounceButton(2) == 0) {
    MODE--;
    PORTD &= ~(1 << PD5);
    if(MODE == 0 || MODE > 254){
      MODE = 3;
      PORTD |= (1 << PD5);
    }
  }
  Serial.println(buttom);
}

ISR(INT1_vect) {
  if (debounceButton(3) == 0) {
    MODE++;
    PORTD |= (1<<PD5);
    if(MODE>3){
      MODE = 0;
      PORTD &= ~(1<<PD5);
    }
  Serial.println(MODE);  
  }
}
/////////////////////////////////////////////
float Mean(uint8_t num){
  uint16_t sum = 0,previous = 0;
  float mean = 0;
  for(uint8_t i = 0 ; i < 10 ;i++){
    previous = Adc_read(num);
    sum += previous;
  }
  mean = (float)sum/10.0;
  mean *= 0.004883;// change to voltage
  return mean;
}
//////////////////////////////////////////////Measure
float Volt() {
  float vout = Mean(1);
  float vin = vout * ((R1_V + R2_V) / (float)R1_V);
  return vin;
}

float Resistor() {
  float vout = Mean(2);
  float resistor = R2_R * ((float)(5.0 / vout) - 1);
   if (resistor > 1000000) {
     return 0;
   } else {
    return resistor;
  }
}

char* ContinuityCheck(float R) {
  if(R < 200 && R != 0){
    PORTB |= (1<<5);
    tone(4, 3000); 
    return "Connect";
  }else{
    PORTB &= ~(1<<5);
    noTone(4);
    delay(10);
    return "Not Connect";
  }
}
float Diode(){
  float vout = Mean(2);
  float vin = 5-vout;
  if(vin == 5.0 || vin >= 4.70){
    vin = 0;
  }
  return vin;
}  
/////////////////////////////////////////////////////////
void ControlOled(uint8_t textsize,int x,int y,const char* sentence,bool enter){
  display.clearDisplay();
  display.setTextSize(textsize);
  display.setTextColor(WHITE);
  display.setCursor(x,y);
  if(enter == 1){
    display.println(sentence);
  }else if(enter == 0){
    display.print(sentence);
  }
}
void ControlOled_value(uint8_t textsize,int x,int y,double value,bool enter){
  display.setTextSize(textsize);
  display.setTextColor(WHITE);
  display.setCursor(x,y);
  if(enter == 1){
    display.println(value);
  }else if(enter == 0){
    display.print(value);
  }
}


//////////////////////////////////////////////////////////


void setup() {
  Serial.begin(9600);
  Adc_init();//set adc
  DDRD &= ~((1<<PD2)|(1<<PD3));//set port
  PORTD |= (1<<PD2)|(1<<PD3);
  DDRD |= (1<<PD4)|(1<<PD5);
  DDRB |= (1<<PB5);
  External_init();//set external interrupt
  /*set oled*/
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(50);
  display.clearDisplay();
  ControlOled(1,0,20,"MULTIMETER",1);
  display.display();  //show
  delay(500);
  display.clearDisplay();
  display.display();
}

void loop() {
  if(MODE == 0){
    Measure[0] = Volt();
    if(Measure[0]<0.1){
      Measure[0] = 0;
    }
    ControlOled(2,0,0,"Volt",0);
    ControlOled_value(2,50,15,Measure[0],1);
    display.setCursor(95,15);
    display.print(" V"); //unit
    display.display(); 
    delay(100);
  }
  else if(MODE == 1){
    Measure[1] = Resistor();
    ControlOled(1,0,0,"ContinuityCheck",0);
    ControlOled(2,50,15,ContinuityCheck(Measure[1]),1);
    display.display();
    delay(300);
  }
  else if(MODE == 2){
    Measure[2] = Resistor();
    ControlOled(1,0,0,"Resistor",0);
    if(Measure[2] > 1000 && Measure[2] < 100000){
      Measure[2] /= 1000.0;
      display.setCursor(80,15);
      display.print("Kohm"); //unit
    } else {
      display.setCursor(80,15);
      display.print("ohm"); //unit
    }
    ControlOled_value(2,30,15,Measure[2],1);
    display.display();  
    Serial.println(Measure[2]);
  }
  else if(MODE == 3){
    Measure[3] = Diode();
    ControlOled(2,0,0,"Diode",0);
    ControlOled_value(2,50,15,Measure[3],1);
    display.display();
  }
}
