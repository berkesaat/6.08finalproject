#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu9255_esp32.h>
#include<math.h>
#include<string.h> 
#include <FastLED.h> //led
#include <FS.h> //sd
#include <SD.h> //sd
#include <SPI.h> //sd
#include <string>
//#include <PulseSensorPlayground.h>


#define SD_CS 5 // SD chip select
#define SCREEN_CS 15 //screen chip select

#define NUM_LEDS 20 //1 Inch

// The LuMini rings need two data pins connected
#define DATA_PIN 12
#define CLOCK_PIN 0

#define USE_ARDUINO_INTERRUPTS false


/////////////////////////////////////////// MEMES /////////////////////////////////////////////////
char stressbmp[10][10] = {"/s1.bmp","/s2.bmp","/s3.bmp","/s4.bmp","/s5.bmp","/s6.bmp","/s7.bmp","/s8.bmp","/s9.bmp","/s10.bmp"};
char unstressbmp[10][10] = {"/n1.bmp","/n2.bmp","/n3.bmp","/n4.bmp","/n5.bmp","/n6.bmp","/n7.bmp","/n8.bmp","/n9.bmp","/n10.bmp"}; 


/////////////////////////////////////////// MOOD RING - lED /////////////////////////////////////////////////
// Define the array of leds
CRGB ring[NUM_LEDS];

void fadeAll(uint8_t scale = 250)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    ring[i].nscale8(scale);
  }
}


/////////////////////////////////////////// INITIALIZE ALL GLOBAL VARIABLES /////////////////////////////////////////////////
boolean stressed = true;
boolean old_stressed = false;

// for pulsesensor library use
//const int OUTPUT_TYPE = SERIAL_PLOTTER;
const int PULSE_INPUT = A0;
const int THRESHOLD = 2500;   // Adjust this number to avoid noise when idle
byte samplesUntilReport;
const byte SAMPLES_PER_SERIAL_SAMPLE = 10;
const float LOWER_BOUND = 5.5;
const float UPPER_BOUND = 6.2;
const float HRV_THRESHOLD = 20;
//PulseSensorPlayground pulseSensor;

TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int BUTTON_PIN = 17;
const int LOOP_PERIOD = 40;
int state;
int heartbeat;
float ibi;
float hrv;
int bpm;
float ibi_values[100]= {0};


//ADD MANUAL ENTRANCE OF THE RESTING HEART RATE
const int RESTING_HEARTRATE = 73;
const int RESTING_IBI = 60000/RESTING_HEARTRATE;
const int delaytime = 60000/(RESTING_HEARTRATE+60);

//Make an array of old ibi values, get the average of that at least
int old_reading = 0;
int very_old_reading = 0;
int super_old_reading = 0;

const int STRESS_LEVEL_THRESHOLD = 90;
char is_stressed[20] = "not stressed";

MPU9255 imu; //imu object called, appropriately, imu



char network[] = "6s08";  //SSID for 6.08 Lab
char password[] = "iesc6s08"; //Password for 6.08 Lab
//char network[] = "MIT";  //SSID for 6.08 Lab
//char password[] = ""; //Password for 6.08 Lab

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char old_response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request

unsigned long primary_timer;
unsigned long secondary_timer;
unsigned long minute_timer;

int old_val;
int stresscount = 0;
int totalcount = 0;


/////////////////////////////////////////// CALCULATE HRV /////////////////////////////////////////////////
float heartratevariable(float input, float* stored_values) {
    float output = 0;
    //Serial.println("ibi");
    //Serial.println(input);
    for (int i = 99; i>0; i--) {
      stored_values[i] = stored_values[i-1];
    }
    stored_values[0] = input;
    for (int x = 0; x < 99; x++) {
      output += (stored_values[x+1]-stored_values[x])*(stored_values[x+1]-stored_values[x]);
    }
    //Serial.println(output);
    //Serial.println(output);
    return output/(float) 100 / (float) 1000; //1000 to convert it into seconds
}


/////////////////////////////////////////// BUTTON CLASS /////////////////////////////////////////////////
class Button{
  public:
  uint32_t t_of_state_2;
  uint32_t t_of_button_change;    
  uint32_t debounce_time;
  uint32_t long_press_time;
  uint8_t pin;
  uint8_t flag;
  bool button_pressed;
  uint8_t state; // This is public for the sake of convenience
  Button(int p) {
  flag = 0;  
    state = 0;
    pin = p;
    t_of_state_2 = millis(); //init
    t_of_button_change = millis(); //init
    debounce_time = 10;
    long_press_time = 1000;
    button_pressed = 0;
  }
  void read() {
    uint8_t button_state = digitalRead(pin);  
    button_pressed = !button_state;
  }
  int update() {
    read();
    flag = 0;
    if (state==0) {
      if (button_pressed) {
        state = 1;
        t_of_button_change = millis();
      }
    } else if (state==1) {
      if (!button_pressed){
        state = 0;
        t_of_button_change = millis();
      } else if (millis()-t_of_button_change>debounce_time){
        state = 2;
        t_of_state_2 = millis();
      }
    } else if (state==2) {
      if (!button_pressed){
        state = 4;
        t_of_button_change = millis();
      } else if (millis()-t_of_state_2>long_press_time){
        state = 3;
  
      }
    } else if (state==3) {
      if (!button_pressed){
        state = 4;
        t_of_button_change = millis();
      }
    } else if (state==4) {        
      if (!button_pressed && (millis()-t_of_button_change)>debounce_time){
        state = 0;
        if (millis()-t_of_state_2 <= long_press_time){
          flag = 1;
        } else{
          flag = 2;
        }
      } else if (button_pressed){
        t_of_button_change = millis();
        if (millis()-t_of_state_2 <= long_press_time){
          state = 2;
        } else{
          state = 3;
        }
      }
    }
    return flag;
  }
};


/////////////////////////////////////////// MANUAL MOOD ENTRY /////////////////////////////////////////////////
class MoodPicker {
    char moods[5][10] = {"angry","happy","sad","groovy","tired"};
    char message[400] = {0}; //contains previous query response
    char mood_string[50] = {0};
    int mood_index;
    int state;
    unsigned long scrolling_timer;
    const int scrolling_threshold = 200;
    const float angle_threshold = 0.3;
  public:
    MoodPicker() {
      state = 0;
      memset(message, 0, sizeof(message));
      strcat(message, "Long Press to \nIndicate Mood!");
      mood_index = 0;
      scrolling_timer = millis();
    }
    void cappend(char* buff, char c, int buff_size) {
      int len = strlen(buff);
      buff[len] = c;
      buff[len + 1] = '\0';
    }
    void update(float angle, int button, char* output) {
      switch (state){
        case 0:
          if (button == 2){
            state = 1;
            mood_index = 0;
            strncpy(mood_string, "", 50);
            scrolling_timer = millis();
            strcpy(output,mood_string);
          } else {
            strcpy(output,"no change");
          }
          break;
        
        case 1:
          if (button == 2){
          state = 0;
          }
          if (button == 1){
            state = 2;
            Serial.println(mood_string);
          } else if (angle>angle_threshold && millis()-scrolling_timer>scrolling_threshold){
            scrolling_timer = millis();
            if (mood_index<=3){
              mood_index += 1;
            } else {
              mood_index = 0;
            }
          } else if (angle<-angle_threshold && millis()-scrolling_timer>scrolling_threshold){
            scrolling_timer = millis();
            if (mood_index>=1){
              mood_index -= 1;
            } else {
              mood_index = 4;
            }
          }
          strcpy(mood_string, moods[mood_index]);
          Serial.println(mood_string);
          strcpy(output, mood_string);
          break;
        case 2:
          sendmood(mood_string, true);
          strcpy(message, "");
          strcat(message, mood_string);
          strcpy(output,message);
          state = 0;
          break;
       }
    }
};


/////////////////////////////////////////// INITIALIZE BUTTON/MANUAL ENTRY OBJECTS /////////////////////////////////////////////////
MoodPicker mp; //wikipedia object
Button button(BUTTON_PIN); //button object!



void setup() {
  Serial.begin(115200); //for debugging if needed.

  /////////////////////////// WIFI SETUP ///////////////////////////////
  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect 😕  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected 😕");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }

  /////////////////////////// SCREEN SETUP ///////////////////////////////
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  pinMode(BUTTON_PIN, INPUT_PULLUP);


  ///////////////////// PULSE SENSOR SETUP ///////////////////////////////
  //pulseSensor.analogInput(PULSE_INPUT);
  //pulseSensor.setSerial(Serial);
  //pulseSensor.setOutputType(OUTPUT_TYPE);
  //pulseSensor.setThreshold(THRESHOLD);

  // Skip the first SAMPLES_PER_SERIAL_SAMPLE in the loop().
  //samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

  // Now that everything is ready, start reading the PulseSensor signal.
  //if (!pulseSensor.begin()) {
    //Serial.println("pulse sensor error");
  //}

  ///////////////////// LED SETUP ///////////////////////////////
  LEDS.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(ring, NUM_LEDS);
  LEDS.setBrightness(5);
  for (int i = 0; i < NUM_LEDS; i++) {
    ring[i] = 0x4169E1;
    FastLED.show();
  }

  ///////////////////// SD CARD SETUP ////////////////////////////////
  // Initialise the SD library before the TFT so the chip select gets set
  if (!SD.begin(SD_CS)) {
    Serial.println("Initialization failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");

  /////////////////////////// VARIABLES SETUP ///////////////////////////////
  primary_timer = 0;
  secondary_timer = 0;
  minute_timer = 0;
  state = 0;
  heartbeat = 0;
  ibi = 0;
  hrv = 0;
  bpm = 0;
  stresscount = 0;
  totalcount = 0;
}

void loop() {
  uint16_t reading = analogRead(PULSE_INPUT);
  float scaled_reading = reading * 3.3 / 512;
  int r = (rand() % 10); //randomly pick a meme
  //Serial.print(scaled_reading);
  //Serial.print(" ");
  //Serial.println(heartbeat);
  switch(state) {
    case 0:
    //We have a peak!!
      if (scaled_reading >= UPPER_BOUND) {state = 1; primary_timer = millis(); } 
      break;
    case 1:
    //We get to a middle point!!!!!
      if (millis() - primary_timer > 500) {state = 0; } //noise elimination
      if (scaled_reading >= LOWER_BOUND && scaled_reading <= UPPER_BOUND) {state = 2; primary_timer = millis(); }
      break;
    case 2:
    //OMG!!!! WE GO OUTSIDE THE MIDDLE PART AGAIN!!!!!!!!!!
      if (scaled_reading <= LOWER_BOUND) {state = 3; primary_timer = millis(); } 
      break;
    case 3:
    //Okay this is ridiculous!!! WE HAVE A HEARTBEAT!!!!!!!!!
      if (millis() - primary_timer > 500) {state = 0; }
      if (scaled_reading >= LOWER_BOUND && scaled_reading <= UPPER_BOUND) {state = 4; primary_timer = millis(); }
      break;
    case 4:
      ibi = (float) (millis() - secondary_timer);
      //Serial.println("ibi");
      //Serial.println(ibi);
      bpm = 60000/ibi; //heartrate calculation (beats per min)
      heartbeat += 1; //for sanity checks
      if (bpm >= 60 && bpm <= 200) {
        Serial.print("bpm: ");
        Serial.println(bpm);
        hrv = heartratevariable(ibi, ibi_values);
        stressed = (hrv <= HRV_THRESHOLD);
        if (stressed){
          stresscount += 1; //sending once in a certain period, the percentage of being stressed
        }
        totalcount += 1;
        //Serial.println(totalcount);
        if (totalcount == 100){
           char buff[4];
           itoa(stresscount, buff, 10);
           sendmood(buff, false); //helper function, posts the stress data on the website
           totalcount = 0;
           stresscount = 0;
        }
        Serial.print("hrv: ");
        Serial.println(hrv);
      }
      primary_timer = millis();
      secondary_timer = millis();
      state = 0;
      break;
      
  }

  //if the mood changed between stressed and not stressed, the change is reflected on the mood ring / LED and the LCD screen with an appropriate meme
  if (stressed != old_stressed) { 
    // Now initialise the TFT
    r = (rand() % 10);
    if (stressed){
      drawBmp(stressbmp[r], 0, 0);
      for (int i = 0; i < NUM_LEDS; i++) {
        ring[i] = 0xDA70D6;
        FastLED.show();
      }
    } else{
      drawBmp(unstressbmp[r], 0, 0);
      for (int i = 0; i < NUM_LEDS; i++) {
        ring[i] = 0x4169E1;
        FastLED.show();
      }
    }
  }
  old_stressed = stressed;


  //MANUAL MOOD ENTRY - choose between moods via tilting the hardware, and submit the chosen mood via short/long presses
  float x, y;
  get_angle(&x, &y); //get angle values
  int bv = button.update(); //get button value
  boolean nochange = false;
  mp.update(-y, bv, response); //input: angle and button, output String to display on this timestep
  
  //After the manual entry is done, we show a new meme according to the mood.
  if (strcmp(response, old_response) != 0) {//only draw if changed!
    if (strcmp(response, "no change") == 0){
      r = (rand() % 10);
      if (stressed){
        drawBmp(stressbmp[r], 0, 0);//, 16);
      } else{
        drawBmp(unstressbmp[r], 0, 0);//, 16);
      }
    } else {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.println(response);
    }
    memset(old_response, 0, sizeof(old_response));
    strcat(old_response, response);
  } 

  //WE DECIDED TO NOT USE THE LIBRARY CODE!!
  //while (millis() - primary_timer < LOOP_PERIOD){
    //Serial.println("la");
    //if (pulseSensor.sawNewSample()) {
      //if (--samplesUntilReport == (byte) 0) {
        //samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;
        //int bpm = pulseSensor.getBeatsPerMinute();
        
        //int ibi = pulseSensor.getInterBeatIntervalMs();
        //old_old_ibi = old_ibi;
        //old_ibi = ibi;
        //stress_level_ibi = (ibi + old_ibi + old_old_ibi) / 3.0;
        //Serial.println(bpm);
        //Serial.println(ibi);
              //if (stress_level_ibi < STRESS_LEVEL_THRESHOLD) {
        //strcpy(is_stressed, "not stressed");
      //}
      //else {
        //strcpy(is_stressed, "stressed");
      //}
      //Serial.println(is_stressed);
      //}
    //}
  //}
  //primary_timer = millis();

}
