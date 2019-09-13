#include <TaskScheduler.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <SD.h>
#include <DS3232RTC.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define LED_PIN    5
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 10
// In which intervall shall the Pixels blink
#define INTERVAL_BLINK 500

unsigned long time_1 = 0;
volatile byte state = LOW;
bool measuring = false;
volatile int fx = 0;
volatile int sy = 0;
volatile int tz = 0;
volatile int mode = 0;

//Here the variables for the measurements are initialized
long phWert = 0;
long o2Wert = 0;

//Create gps object
TinyGPS gps;
//SoftwareSerial on Pin 4 and 3
SoftwareSerial ss(4, 3); //RX, TX

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
File dataFile;

//Scheduler object
//loop2 runs twice a second
Scheduler ts;
void loop2();
Task t1 (500 * TASK_MILLISECOND, TASK_FOREVER, &loop2, &ts, true);

// setup() function -- runs once at startup --------------------------------

void setup() {
  Serial.begin(115200);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  randomSeed(analogRead(0));  //delete random later, just for testing purposes

  // Synchronizing with the RTC
  setSyncProvider(RTC.get);

  //Check if initializing SD card failed or succeeded
  Serial.print("Initializing SD card...");
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
}

void print_time(unsigned long time_millis);

//main loop
//here the battery status will be checked and the data of the measurement, that is to be triggered, will be saved onto a SD card
void loop() {
  ts.execute();
  mode = 1;
  if (digitalRead(2) == HIGH && measuring == false) {
    measuring = true;
    //Swap water. Blink blue
    //Give Command to swap the water Befehl
    //Eventually getting signal that swicthing water is finished
    mode = 1, fx = 0, sy = 0, tz = 255; //Blink blue
    delay(4000); //Adjust time for swap water
    //Getting data from sensors. Blink Green
    //Data will be transferred via Modbus / Wait for data
    fx = 0, sy = 255, tz = 0; //Blink green
    delay(2000); //Simulates sending of data
    phWert = random(100);
    o2Wert = random(100);
    //Now the data will be written onto the SD card. Before that data will be retrieved from the GPS and RTC
    //Blink yellow
    //If no mistake in reading data: Write into file.
    fx = 0, sy = 255, tz = 0;
    checkAndWriteData();
    delay(2000); //To see the blinking
    //IF read-out error: flash red
    //IF done: flash green for three seconds

    measuring = false;
  }
  checkBatteryStatus();       // Show Battery Status
}

//trigger the blinking of LED's twice a second or flash LED's
void loop2() {
  time_1 = millis(); //to check the timing of loop2 and the blinking
  if (measuring) {
    print_time(time_1);
    blink (strip.Color(fx,   sy,   tz), state, mode, 0);
  }
}

//Function for blinking or flashing the LED's
void blink (uint32_t color, byte _state, int mode, int wait) {
  if (mode == 1) {
    if (_state == HIGH) {
      strip.clear();
      for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
        strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
        strip.show();
      }
    } else {
      strip.clear();
      strip.show();
    }
    state = !state;
  }

  if (mode == 2) {
    for (int i = 0; i < strip.numPixels(); i++) {  // For each pixel in strip...
      strip.setPixelColor(i, color);               //  Set pixel's color (in RAM)
      strip.show();                                //  Update strip to match
    }
    delay(wait);
    strip.clear();
  }
}

//Function to check the battery status with LED's
void checkBatteryStatus (void) {
  strip.clear();
  strip.show();
  int charge = 4;
  int x = 0, y = 0, z = 0;
  if (charge < 4) {
    x = 255;
    z = 0;
  } else {
    x = 0;
    z = 255;
  }
  for (int i = 0; i < charge; i++) {                // For each pixel in strip...
    strip.setPixelColor(i, strip.Color(x, y, z));   //  Set pixel's color (in RAM)
  }
  strip.show();  //  Update strip to match
  delay(50);
}

//delete this function later, just for testing purposes
void print_time(unsigned long time_millis) {
  Serial.print("Time: ");
  Serial.print(time_millis / 1000);
  Serial.print("s - ");
}

//This function
void checkAndWriteData(void) {
  //Auf Fehler überprüfen: SDbegin 10 und timestatus = time set

  //This measurement is triggered

  String fileNameOne = fileName();
  dataFile = SD.open(fileNameOne, FILE_WRITE);

  // if the file opened okay, write to it:
  if (dataFile) {
    Serial.print("Writing to " + fileNameOne);
    dataFile.println(date() + "," + pos() + "," + phWert + "," + o2Wert);
    // close the file:
    dataFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening ");
  }
}

//This function adds a leading Zero to a integer, if the integer is smaller than 10
//Afterwards the integer is casted into a String
String leadingZero(int digits) {
  String temp;
  String temp2 = String(digits);
  String zero = "0";
  if (digits < 10) {
    temp = zero + temp2;
  } else {
    temp = temp2;
  }
  return temp;
}

//This funtion returns the fileName based on the current date
String fileName(void) {
  String x = leadingZero(day());
  String y = leadingZero(month());
  String z = String(year() % 100);
  String date = x + y + z + "M2.txt";
  return date;
}

//This function returns the current date with the format: dd.mm.yyyy.hh.mimi.ss
String date() {
  String x = leadingZero(day());
  String y = leadingZero(month());
  String z = String(year());
  String u = leadingZero(hour());
  String v = leadingZero(minute());
  String w = leadingZero(second());
  String point = ".";
  String date = x + point + y + point + z + point + u + point + v + point + w;
  return date;
}

//returns the current position in longitude and latitude as: "lon,lat"
String pos() {
  float flat, flon;
  gps.f_get_position(&flat, &flon);
  String comma = ",";
  float invalid = TinyGPS::GPS_INVALID_F_ANGLE;
  if (flat || flon == invalid)
  {
    return "********,********";
  } else {
    return flat + comma + flon;
  }
}

//This function sets the position of the comma
float floatPrec(float value) {
  value = round(value * 1000000) / 1000000.0;
  return value;
}
