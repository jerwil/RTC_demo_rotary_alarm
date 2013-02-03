// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
 
#include <Wire.h>
#include "RTClib.h"
 
RTC_DS1307 RTC;

int adjust_amount = 60;    // how many seconds to adjust the time by
unsigned long currentTime;
unsigned long loopTime;
const int pin_A = 7;  // pin 12
const int pin_B = 6;  // pin 11
const int button_pin = 8;
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;
int current_count;
int mode = 1;
char* mode_str[] = {"Tacos","Clock","Alarm Set"};
double alarm = 10800; // Alarm default in seconds
int alarm_array[6];
int current_time_array[6];
int old_second = 0; //This is used for the tick mechanism
int unixtime_int = 0;
int display_array[6];
int time_format = 12;
boolean button_hold = false;
int button_state = 0;
int button_counter = 0; // This is used to detect how long the button is held for
int timeout = 0; // Time out for not pushing the button for a while
 
void setup () {
    pinMode(pin_A, INPUT);
    pinMode(pin_B, INPUT);
    pinMode(button_pin, INPUT);
    currentTime = millis();
    loopTime = currentTime; 
    Serial.begin(57600);
    Wire.begin();
    RTC.begin();
    //DateTime now = RTC.now();

 
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
  
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
}


void printtime(DateTime time){
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
    Serial.print(mode_str[mode]);
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
  if (time_format == 12 && hours > 12) {
   hours -= 12; 
  }
  if (hours < 10){
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
  
  Serial.print("Digit 1:");
  Serial.print(digit_array[0]);
  Serial.println(); 
  Serial.print("Digit 2:");
  Serial.print(digit_array[1]);
  Serial.println(); 
  Serial.print("Digit 3:");
  Serial.print(digit_array[2]);
  Serial.println(); 
  Serial.print("Digit 4:");
  Serial.print(digit_array[3]);
  Serial.println(); 
}

// _____________ PROGRAM STARTS HERE: _____________//

void loop () {

DateTime now = RTC.now();
time_to_ints(now, current_time_array);

button_state = digitalRead(button_pin);
if (button_state == HIGH){
 button_hold = true;
 timeout = 0; 
}
else {
 button_hold = false; 
}


//time_to_ints(now, current_time_array);

// The following checks the alarm condition:
if (alarm == time_to_double(now)){
  mode = 3;
}

 if(mode == 1){
   currentTime = millis();
 // get the current elapsed time
    // 5ms since last check of encoder = 200Hz  
    encoder_A = digitalRead(pin_A);    // Read encoder pins
    encoder_B = digitalRead(pin_B);   
    if((!encoder_A) && (encoder_A_prev)){
      // A has gone from high to low 
      if(encoder_B) {
        // B is high so clockwise
        // increase the brightness, dont go over 255
          timeout = 0; 
          now = RTC.now();
          DateTime then(now.unixtime() + adjust_amount); // one hour later
          RTC.adjust(then);
          now = RTC.now();
          printtime(now);   
      }
      else {
        // B is low so counter-clockwise      
        // decrease the brightness, dont go below 0
          timeout = 0; 
          now = RTC.now();
          DateTime then(now.unixtime() - adjust_amount); // one hour later
          RTC.adjust(then);
          now = RTC.now();
          printtime(now);        
      }   

    }
    encoder_A_prev = encoder_A;
    loopTime = currentTime;
// Previous tick mechanism:

/*
current_count += 1;
if (current_count%500==0){
  printtime(now);
  time_to_ints(now, current_time_array);
  current_count = 1;
  print_time_array_separated(current_time_array);
  Serial.print("Unix time: ");
  Serial.print(now.unixtime());
}
*/

// New tick mechanism:


int now_second = now.second();
if (old_second >= 59){
  old_second = 0;
}
if (now_second > old_second){
  Serial.print("Unix time: ");
  Serial.print(now.unixtime());
  Serial.println();
  printtime(now);
  old_second = now_second;
  Serial.print("Time as a double: ");
  Serial.print(time_to_double(now));
  Serial.println();
  time_array_to_digit_array(current_time_array, display_array); 
  if (button_hold == true){
    if (button_counter >= 3) {
      mode = 2;
    Serial.print("Switched mode to:");
    Serial.print(mode_str[mode]);
    Serial.println();
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

 if(mode == 2){


    // 5ms since last check of encoder = 200Hz  
 encoder_A = digitalRead(pin_A);    // Read encoder pins
 encoder_B = digitalRead(pin_B);   
    if((!encoder_A) && (encoder_A_prev)){
      // A has gone from high to low 
      if(encoder_B) {
        // B is high so clockwise
        timeout = 0; 
        alarm += adjust_amount; // adjust alarm by a set amount (minute)
      }
      else {
        // B is low so counter-clockwise
          timeout = 0;       
          alarm -= adjust_amount;     
      }   

    }
    encoder_A_prev = encoder_A;


  
  int now_second = now.second();
if (old_second >= 59){
  old_second = 1;
}
if (now_second > old_second){
  old_second = now_second;
  timeout += 1;
  if (timeout >= 10){ // If the butto
    mode = 1;
    timeout = 0;
    Serial.print("Timeout, switching to clock mode");
    Serial.println();
  }
  Serial.print("Alarm Set Mode");
  Serial.println();
  time_array_to_digit_array(alarm_array, display_array);
  secs_to_hms(alarm, alarm_array);
  print_time_array_separated(alarm_array);
}




   
 }
 
if(mode == 3){
  
int now_second = now.second();
if (old_second >= 59){
  old_second = 0;
}
if (now_second > old_second){
  Serial.print("Alarm!!!! This is testing using git gui. I checked out the alarm branch");
  Serial.println();
  old_second = now_second;
}

}

}


