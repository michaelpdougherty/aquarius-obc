#include <SFE_ISL29125.h>

#include <SD.h>
#include <SD_t3.h>

#include <dht.h>
#include <SPI.h>

#define N 255

///// USER CAN CHANGE VALUES BELOW /////

// define pins
const unsigned int DHT_PIN = 2;

// set filename
char FILENAME[N] = "LOG.TXT";

unsigned int LOG_NUMBER = 0;

////////////////////////////////////////

String newFilename;

// set communication rate
const unsigned int BAUD_RATE = 9600;
const unsigned int DELAY = 1000;

// create objects
File LOG;
dht DHT;
SFE_ISL29125 RGB;


// init timer
unsigned int seconds = 0;

void setup() { 
  // begin serial communication
  Serial.begin(BAUD_RATE);

  // give serial time to initialize
  delay(DELAY * 2);
    
  // initialize SD card
  initSD();

  // initialize filename
  initFile();

  // ensure SD can be written to
  checkSD();

  // initialize RGB sensor
  if (RGB.init())
  {
    Serial.println("RGB sensor initialization successful");
  } else {
    Serial.println("RGB sensor initialization failed");
  }

  

  // will ya give it a SECOND?!
  delay(DELAY);
  seconds++;
}

void loop() {


  // INTERNAL TEMPERATURE
  // update DHT22 object
  DHT.read22(DHT_PIN);
  
  // get temperature in degrees Celsius
  float internalDegreesCelsius = DHT.temperature;

  // get internal humidity percentage
  float internalHumidityPercentage = DHT.humidity;

  // RGB LIGHT LEVELS
  unsigned int red = RGB.readRed();
  unsigned int green = RGB.readGreen();
  unsigned int blue = RGB.readBlue();





  
  
  // open file for writing
  LOG = SD.open(FILENAME, FILE_WRITE);

  // write data to file
  LOG.print(seconds);
  LOG.print(",");
  LOG.print(internalDegreesCelsius);
  LOG.print(",");
  LOG.print(internalHumidityPercentage);
  LOG.print(",");
  LOG.print(red);
  LOG.print(",");
  LOG.print(green);
  LOG.print(",");
  LOG.print(blue);

  LOG.println();

  // close file
  LOG.close();

  // write data to serial
  if (Serial) {
    Serial.print(seconds);
    Serial.print(",");
    Serial.print(internalDegreesCelsius);
    Serial.print(",");
    Serial.print(internalHumidityPercentage);
    Serial.print(",");
    Serial.print(red);
    Serial.print(",");
    Serial.print(green);
    Serial.print(",");
    Serial.print(blue);
    Serial.println();
  }

  // delay and increment timer
  delay(DELAY);
  seconds++;  
}

void initSD() {
  Serial.print("Initializing SD card... ");
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
}

void checkSD() {
  // open file
  LOG = SD.open(FILENAME, FILE_WRITE);
  
  if (LOG) {
    // print to console
    Serial.print("Writing to ");
    Serial.print(FILENAME);
    Serial.print("... ");
    
    // close the file
    LOG.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error
    Serial.print("Error opening ");
    Serial.println(FILENAME);
  }
}

void initFile() {
  // open root
  File root = SD.open("/");

  // create test filename
  String testFilename = String(FILENAME);

  // iterate over root
  while (true) {
    File entry = root.openNextFile();

    if (!entry) {
      // reached last file
      break;
    }

    if (  String(entry.name()).equals(testFilename)  ) {
      LOG_NUMBER++;
      testFilename = String(LOG_NUMBER) + String(FILENAME);
    }
  }
  for (unsigned int i = 0; i < N; i++) {
    FILENAME[i] = testFilename[i];
  }
}
