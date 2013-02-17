// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
 
#include <Wire.h>
#include "RTClib.h"
 
RTC_DS1307 RTC;

int adjust_amount = 60;    // how many seconds to adjust the time by
int multiplier = 1; // This mutliplier is used to change to hour adjustment
unsigned long currentTime;
unsigned long loopTime;
const int pin_A = 11;  // Rotary encoder pin A
const int pin_B = 10;  // Rotary encoder pin B
const int PM_pin = 13;  // This indicates if time is PM
const int button_pin = 8;
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;
int current_count;
char* mode = "time_disp";
char* sub_mode = "minute_set";
char* mode_str[] = {"Tacos","Clock","Alarm Set"};
double alarm = 10800; // Alarm default in seconds
int alarm_array[6];
int current_time_array[6];
int old_second = 0; //This is used for the tick mechanism
int now_second = 0;
int unixtime_int = 0;
int display_array[6];
int time_format = 12;
boolean button_hi = false;
int button_state = 0;
int button_counter = 0; // This is used to detect how long the button is held for
int timeout = 0; // Time out for not pushing the button for a while
int blink = 1; // This is used for blinking numbers while adjusting time
double second_timer[1] = {0}; // This is use dto keep track of the timer used to tick for each second
double half_second_timer[1] = {0}; // This is use dto keep track of the timer used to tick for each second
int PM = 0; // This is the indicator that time is in PM
int button_press_initiate[1]; 
int button_press_completed[1];
int button_pushed = 0; // This is the indicator that the button was pushed and released
int alarm_tone = 1000; // This is the frequency for the alarm buzzer
int speakerPin = 9; // This is the pin used by the alarm buzzer

// Bit shifter pins:

const int  g_pinCommLatch = 6; // This pin gets sets low when I want the 595s to listen
const int  g_pinClock     = 7; // This pin is used by ShiftOut to toggle to say there's another bit to shift
const int  g_pinData    = 4; // This pin is used to pass the next bit
byte g_digits [11]; // Array of binary values for each digit
const int g_registers = 4; // Number of shift registers in use
byte g_registerArray [g_registers]; // Array of numbers to pass to shift registers
 
void setup () {
    pinMode(pin_A, INPUT);
    pinMode(pin_B, INPUT);
    pinMode(button_pin, INPUT);
	pinMode (g_pinCommLatch, OUTPUT);
	pinMode (g_pinClock, OUTPUT);
	pinMode (g_pinData, OUTPUT);
	pinMode (PM_pin, OUTPUT);
	pinMode (speakerPin, OUTPUT);

    Serial.begin(57600);
    Wire.begin();
    RTC.begin();
	DateTime now = RTC.now();
	int now_second = now.second();
	old_second = now_second;
    //DateTime now = RTC.now();
	
  // Setup the digits array
  // a = 8 b = 4 c = 2 d = 64 e = 32 f = 1 g = 16
  g_digits [0] = 8 + 4 + 2 + 64 + 32 + 1;
  g_digits [1] = 4 + 2;
  g_digits [2] = 8 + 4 + 16 + 32 + 64;
  g_digits [3] = 8 + 4 + 16 + 2 + 64;
  g_digits [4] = 1 + 16 + 4 + 2;
  g_digits [5] = 8 + 1 + 16 + 2 + 64;
  g_digits [6] = 8 + 1 + 16 + 2 + 64 + 32;
  g_digits [7] = 8 + 4 + 2;
  g_digits [8] = 8 + 4 + 2 + 64 + 32 + 1 + 16;
  g_digits [9] = 8 + 4 + 2 + 1 + 16 + 64;
  g_digits [10] = 0;

 
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
  
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
}


void printtime(DateTime time){ // This function mainly used for debugging purposes
    Serial.print(" time: ");
    Serial.print(time.year(), DEC);
    Serial.print('/');
    Serial.print(time.month(), DEC);
    Serial.print('/');
    Serial.print(time.day(), DEC);
    Serial.print(' ');
    Serial.print(time.hour(), DEC);
    Serial.print(':');
    Serial.print(time.minute(), DEC);
    Serial.print(':');
    Serial.print(time.second(), DEC);
    Serial.println();
    Serial.print("mode = ");    
    Serial.print(mode);
    Serial.println();  
}

void time_to_ints(DateTime time, int time_array[6]){
    int year_int, month_int, day_int, hour_int, minute_int, second_int;
    year_int = time.year();
    month_int = time.month();
    day_int = time.day();
    hour_int = time.hour();
    minute_int = time.minute();
    second_int = time.second();
    time_array[0] = year_int;
    time_array[1] = month_int;
    time_array[2] = day_int;
    time_array[3] = hour_int;
    time_array[4] = minute_int;
    time_array[5] =  second_int;
}

double time_to_double(DateTime time){
    double year_int, month_int, day_int, hour_int, minute_int, second_int;
    year_int = time.year();
    month_int = time.month();
    day_int = time.day();
    hour_int = time.hour();
    minute_int = time.minute();
    second_int = time.second();
    double seconds = hour_int*3600+minute_int*60+second_int;
    return seconds;
}

void print_time_array_separated(int time_array[6]){
  Serial.println();
  Serial.print("Hours: ");
  Serial.print(time_array[3]);
  Serial.println();
  Serial.print("Minutes: ");
  Serial.print(time_array[4]);
  Serial.println();
  Serial.print("Seconds: ");
  Serial.print(time_array[5]);
  Serial.println();
}

int tick(int delay, double timekeeper[1]){
currentTime = millis();
if(currentTime >= (timekeeper[0] + delay)){
	timekeeper[0] = currentTime;
	Serial.print("From within tick function, time keeper is now:");
  Serial.print(second_timer[0]);
  Serial.println();
  	Serial.print("From within tick function, currentTime is now:");
  Serial.print(currentTime);
  Serial.println();
	return 1;
  }
else {return 0;}
}

void secs_to_hms(double secs_in, int time_array[6]){
    double hours = floor(secs_in/3600);
    double minutes = floor(((secs_in - hours*3600)/60));
    double seconds = floor((secs_in - hours*3600 - minutes*60));
    int hour_int = hours;
    int minute_int = minutes;
    int second_int = seconds;
    time_array[3] = hour_int;
    time_array[4] = minute_int;
    time_array[5] = second_int;
}

double time_array_to_secs(int time_array[6]){
    return time_array[3]*60*60 + time_array[4]*60 + time_array[5];
}

void time_array_to_digit_array(int time_array[6], int digit_array[6]){
  int hours = time_array[3];
  int minutes = time_array[4];
  int seconds = time_array[5];
  PM = 0; // AM until proven PM
  if (hours == 0){
  hours = 12;
  }
  else if (time_format == 12 && hours > 12) {
   hours -= 12;
	PM = 1;
  }
  if (hours < 10 && hours > 0){
    digit_array[0] = 10; // 10 will be the designation for not displaying anything
    digit_array[1] = hours;
  }
  else {
    digit_array[0] = hours/10;
    digit_array[1] = hours%10;
  }
  if (minutes < 10){
  digit_array[2] = 0; // 10 will be the designation for not displaying anything
  digit_array[3] = minutes;
  }
  else {
    digit_array[2] = minutes/10;
    digit_array[3] = minutes%10;
  }
  if (time_format == 24){PM = 0;} // The PM LED is not needed in 24 hour time format
  
  // Serial.print("Digit 1:");
  // Serial.print(digit_array[0]);
  // Serial.println(); 
  // Serial.print("Digit 2:");
  // Serial.print(digit_array[1]);
  // Serial.println(); 
  // Serial.print("Digit 3:");
  // Serial.print(digit_array[2]);
  // Serial.println(); 
  // Serial.print("Digit 4:");
  // Serial.print(digit_array[3]);
  // Serial.println(); 
}

void sendSerialData (byte registerCount, byte *pValueArray)
{
  // Signal to the 595s to listen for data
  digitalWrite (g_pinCommLatch, LOW);
  
  for (byte reg = registerCount; reg > 0; reg--)
  {
    byte value = pValueArray [reg - 1];
    
    for (byte bitMask = 128; bitMask > 0; bitMask >>= 1)
    {
      digitalWrite (g_pinClock, LOW);
    
      digitalWrite (g_pinData, value & bitMask ? HIGH : LOW);
        
      digitalWrite (g_pinClock, HIGH);
    }
  }
  // Signal to the 595s that I'm done sending
  digitalWrite (g_pinCommLatch, HIGH);
}  // sendSerialData

int button_press (int button_indicator, int button_press_initiated[1], int button_press_complete[1]){
	if (button_indicator == 0 && button_press_initiated[0] == 1) {
	button_press_complete[0] = 1;
	Serial.print("Button press complete");
	Serial.println();
	button_press_initiated[0] = 0;
	}
	else if (button_indicator == 1){
	button_press_initiated[0] = 1;
	button_press_complete[0] = 0;
	}
	else {button_press_complete[0] = 0;}
return button_press_complete[0];
}

void buzz(int tone) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
}

// _____________ PROGRAM STARTS HERE: _____________//

void loop () {

DateTime now = RTC.now();
time_to_ints(now, current_time_array);

button_state = digitalRead(button_pin);
button_pushed = button_press (button_state, button_press_initiate, button_press_completed);
if (button_state == HIGH){
 button_hi = true;
 timeout = 0; 
}
else {
 button_hi = false; 
}

if (PM == 1){digitalWrite(PM_pin, HIGH);}
else{digitalWrite(PM_pin, LOW);}

//time_to_ints(now, current_time_array);

// The following checks the alarm condition:
if (alarm == time_to_double(now)){
  mode = "alarm_sound";
}

 if(mode == "time_disp"){ // This is current time mode
   currentTime = millis();
   time_array_to_digit_array(current_time_array, display_array); 
 // get the current elapsed time
    // 5ms since last check of encoder = 200Hz

// New tick mechanism:


if (tick(1000, second_timer) == 1){
  Serial.print("Unix time: ");
  Serial.print(now.unixtime());
  Serial.println();
  printtime(now);
  old_second = now_second;
  Serial.print("Time as a double: ");
  Serial.print(time_to_double(now));
  Serial.println();
  if (button_hi == true){ // This code checks to see if the button has been held down long enough to set alarm
    if (button_counter >= 3) {
      mode = "alarm_set";
	  sub_mode = "hour_set";
    Serial.print("Switched mode to:");
    Serial.print(mode);
    Serial.println();
	button_pushed = 0;
    }
    button_counter += 1;
  }
  else {
   button_counter = 0; 
  }
  Serial.print("Button held for:");
  Serial.print(button_counter);
  Serial.println();
}

}

 if(mode == "time_set"){ // This is time set mode 
	if (button_pushed == 1 && sub_mode == "minute_set") {sub_mode = "hour_set";}
	else if (button_pushed == 1 && sub_mode == "hour_set") {
		sub_mode = "minute_set";
		button_pushed = 0;
	}
	time_array_to_digit_array(current_time_array, display_array); 
   	if (sub_mode == "minute_set"){
	multiplier = 1;
	}
	else if (sub_mode == "hour_set"){
	multiplier = 60;
	}
    encoder_A = digitalRead(pin_A);    // Read encoder pins, this should be turned into a function!!!!!!!!!!!!!!!!!!!!!!!!!
    encoder_B = digitalRead(pin_B);   
		if((!encoder_A) && (encoder_A_prev)){
		  // A has gone from high to low 
		  if(encoder_B) {
			// B is high so clockwise
			// increase the brightness, dont go over 255
			timeout = 0;
			blink = 1; // This ensures the digits are visible while adjusting time			  
			now = RTC.now();
			DateTime then(now.unixtime() + adjust_amount*multiplier); // one hour later
			RTC.adjust(then);
			now = RTC.now();
			printtime(now);   
		  }
		  else {
			// B is low so counter-clockwise      
			// decrease the brightness, dont go below 0
			timeout = 0;
			blink = 1;			  
			now = RTC.now();
			DateTime then(now.unixtime() - adjust_amount*multiplier); // one hour later
			RTC.adjust(then);
			now = RTC.now();
			printtime(now);        
		  }   

		}
		encoder_A_prev = encoder_A;

	time_array_to_digit_array(current_time_array, display_array);
		
	if (blink == 0 && sub_mode == "minute_set"){
		display_array[2] = 10;
		display_array[3] = 10;
		}
	if (blink == 0 && sub_mode == "hour_set"){
		display_array[0] = 10;
		display_array[1] = 10;
		}

	if (tick(1000, second_timer) == 1){  
	  timeout += 1;
	  if (timeout >= 10){ // If the button is not pressed for 10 seconds
		mode = "time_disp";
		timeout = 0;
		Serial.print("Timeout, switching to clock mode");
		Serial.println();
	  }
	if (button_hi == true){ // This code checks to see if the button has been held down long enough to set alarm
		if (button_counter >= 3) {
			mode = "alarm_set";
			sub_mode = "hour_set";
			Serial.print("Switched mode to:");
			Serial.print(mode);
			Serial.println();
			button_pushed = 0;
			}
		button_counter += 1;
	}
	else {button_counter = 0;}
	  Serial.print("Time Set Mode");
	  Serial.println();  
	  print_time_array_separated(current_time_array);
	}
	
	if (tick(500, half_second_timer) == 1){  
		if ((sub_mode == "minute_set" || sub_mode == "hour_set") && blink == 0){
			blink = 1;
		}
		else if ((sub_mode == "minute_set" || sub_mode == "hour_set") && blink == 1){
			blink = 0;
		}
	}
}

 if(mode == "alarm_set"){ // This is alarm set mode
 
if (button_pushed == 1 && sub_mode == "minute_set") {sub_mode = "hour_set";}
else if (button_pushed == 1 && sub_mode == "hour_set") {sub_mode = "minute_set";}
 
 secs_to_hms(alarm, alarm_array);

	if (sub_mode == "minute_set"){
	multiplier = 1;
	}
	else if (sub_mode == "hour_set"){
	multiplier = 60;
	}

    // 5ms since last check of encoder = 200Hz  
 encoder_A = digitalRead(pin_A);    // Read encoder pins
 encoder_B = digitalRead(pin_B);   
    if((!encoder_A) && (encoder_A_prev)){
	timeout = 0;   
	blink = 1;
      // A has gone from high to low 
      if(encoder_B) {
        // B is high so clockwise
        alarm += adjust_amount*multiplier; // adjust alarm by a set amount (minute)
      }
      else {
        // B is low so counter-clockwise    
          alarm -= adjust_amount*multiplier;     
      }   

    }
    encoder_A_prev = encoder_A;

time_array_to_digit_array(alarm_array, display_array);

if (blink == 0 && sub_mode == "minute_set"){
	display_array[2] = 10;
	display_array[3] = 10;
	}
if (blink == 0 && sub_mode == "hour_set"){
	display_array[0] = 10;
	display_array[1] = 10;
	}

	
	if (tick(1000, second_timer) == 1){
  Serial.print("Unix time: ");
  Serial.print(now.unixtime());
  Serial.println();
  printtime(now);
  old_second = now_second;
  Serial.print("Time as a double: ");
  Serial.print(time_to_double(now));
  Serial.println();
  if (button_hi == true){ // This code checks to see if the button has been held down long enough to set alarm
    if (button_counter >= 3) {
      mode = "time_set";
	  sub_mode = "minute_set";
    Serial.print("Switched mode to:");
    Serial.print(mode);
    Serial.println();
	button_pushed = 0;
    }
    button_counter += 1;
  }
  else {
   button_counter = 0; 
  }
  Serial.print("Button held for:");
  Serial.print(button_counter);
  Serial.println();
}
	
	if (tick(1000, second_timer) == 1){  
	  timeout += 1;
	  if (timeout >= 10){ // If the button is not pressed for 10 seconds
		mode = "time_disp";
		timeout = 0;
		Serial.print("Timeout, switching to clock mode");
		Serial.println();
	  }
	if (button_hi == true){ // This code checks to see if the button has been held down long enough to set alarm
		if (button_counter >= 3) {
			mode = "time_set";
			sub_mode = "minute_set";
			Serial.print("Switched mode to:");
			Serial.print(mode);
			Serial.println();
			button_pushed = 0;
			}
		button_counter += 1;
	}
	else {button_counter = 0;}
	  Serial.print("Alarm Set Mode");
	  Serial.println();  
	  print_time_array_separated(alarm_array);
	}
	  
	if (tick(500, half_second_timer) == 1){  
		if ((sub_mode == "minute_set" || sub_mode == "hour_set") && blink == 0){
		blink = 1;
		}
		else if ((sub_mode == "minute_set" || sub_mode == "hour_set") && blink == 1){
		blink = 0;
		}
	}
}
 
if(mode == "alarm_sound"){ // This is alarm mode. Current problem is display_array doesn't get updated
  
time_array_to_digit_array(current_time_array, display_array);

	if (blink == 0){
		display_array[0] = 10;
		display_array[1] = 10;
		display_array[2] = 10;
		display_array[3] = 10;
		buzz(alarm_tone);
	}  
if (tick(500, second_timer) == 1){
	if (blink == 0){blink = 1;}
	else if (blink == 1){blink = 0;}
		Serial.print("Alarm!!!!");
		Serial.println();
	}
	if (button_pushed == 1){mode = "time_disp";}
}

// At the end of each cycle, send display array data to shift registers

	g_registerArray [3] = g_digits[display_array[0]];
	g_registerArray [2] = g_digits[display_array[1]];
	g_registerArray [1] = g_digits[display_array[2]];
	g_registerArray [0] = g_digits[display_array[3]];
	sendSerialData (g_registers, g_registerArray);
}


