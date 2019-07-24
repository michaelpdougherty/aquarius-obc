// Pressure / depth / altitude / external temperature sensor libraries
#include <MS5837.h>
#include <Wire.h>

// RGB sensor library
#include <SFE_ISL29125.h>

// SD card libraries
#include <SD.h>
#include <SD_t3.h>
#include <SPI.h>

#define N 255

///// USER CAN CHANGE VALUES BELOW /////

// set default filename structure
String FILE_BODY = "LOG";
String FILE_EXTENSION = ".CSV";

// declare internal LED
const unsigned int LED = 13;

////////////////////////////////////////

// init timer
int seconds = 0;

// init failchecks
int SDfailed = 0;
int RGBfailed = 0;
int sensorFailed = 0;

// Create filename vars
unsigned int FILE_NUMBER_INT = 0;
String FILE_NUMBER_STRING = "000";
String testFilename = FILE_BODY + FILE_NUMBER_STRING + FILE_EXTENSION;
char FILENAME[N] = {};

// set baud rate and delay
const unsigned int BAUD_RATE = 9600;
const unsigned int DELAY= 1000;

// create file and sensor objects
File LOG;
SFE_ISL29125 RGB;
MS5837 sensor;

void setup() { 
  // begin serial communication
  Serial.begin(BAUD_RATE);

  // init blink
  pinMode(LED, OUTPUT);

  // give serial time to initialize
  delay(DELAY * 2);
    
  // initialize SD card
  initSD();

  // initialize filename
  initFile();

  // ensure SD can be written to
  checkSD();

  // initialize RGB sensor
  initRGB();

  // initialize pressure, depth, altitude, and external temperature sensor
  initBlueRobotics();

  // print headers
  printHeaders();

  // will ya give it a SECOND?!
  delay(DELAY);
}

void loop() {
  // RGB LIGHT LEVELS
  unsigned int red = RGB.readRed();
  unsigned int green = RGB.readGreen();
  unsigned int blue = RGB.readBlue();
  

  // DEPTH / PRESSURE / ALTITUDE / EXTERNAL TEMPERATURE
  // Update pressure and temperature readings
  sensor.read();
  // get vars
  float pressure = sensor.pressure(); // mbar
  float externalTemperature = sensor.temperature(); // deg C
  float depth = sensor.depth(); // m
  float altitude = sensor.altitude(); // m above mean sea level

  // get time
  seconds = (millis() / 1000);
  
  
  // open file for writing
  LOG = SD.open(FILENAME, FILE_WRITE);

  // write data to file
  LOG.print(seconds);
  LOG.print(",");
  LOG.print(externalTemperature);
  LOG.print(",");
  LOG.print(pressure);
  LOG.print(",");
  LOG.print(depth);
  LOG.print(",");
  LOG.print(altitude);
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
    Serial.print(externalTemperature);
    Serial.print(",");
    Serial.print(pressure);
    Serial.print(",");
    Serial.print(depth);
    Serial.print(",");
    Serial.print(altitude);
    Serial.print(",");
    Serial.print(red);
    Serial.print(",");
    Serial.print(green);
    Serial.print(",");
    Serial.print(blue);
    Serial.println();
  }

  // attempt to initialize failed sensors
  checkSensors();

  // blink if SD card initialized
  if (!SDfailed) {
    digitalWrite(LED, HIGH);   // set the LED on
    delay(500);
    digitalWrite(LED, LOW);    // set the LED off
    delay(500);
  } else {
    delay(1000);
  }
}

void initSD() {
  Serial.print("Initializing SD card... ");
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("NO SD CARD");
    SDfailed = 1;
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

    LOG.println("Time (s), External Temperature (deg C), Pressure (mbar), Depth (m), Altitude (m), Red, Green, Blue");
    
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

  // iterate over root
  while (true) {
    File entry = root.openNextFile();

    if (!entry) {
      // reached last file and no match found
      break;
    }

    if ( String(entry.name()).equals(testFilename) ) {
      // current filename exists
      FILE_NUMBER_INT++;
      updateFileNumberString();
      testFilename = FILE_BODY + FILE_NUMBER_STRING + FILE_EXTENSION;
    }
  }
  // transfer string to char[]
  for (unsigned int i = 0; i < N; i++) {
    FILENAME[i] = testFilename[i];
  }
}

void updateFileNumberString() {
  // check digis of FILE_NUMBER_INT
  if (FILE_NUMBER_INT >= 100) {
    // three digits
    FILE_NUMBER_STRING = String(FILE_NUMBER_INT);
  } else if (FILE_NUMBER_STRING >= 10) {
    // two digits
    FILE_NUMBER_STRING = "0" + String(FILE_NUMBER_INT);
  } else {
    // one digit
    FILE_NUMBER_STRING = "00" + String(FILE_NUMBER_INT);
  }
}

void initRGB() {
  if (RGB.init()) {
    Serial.println("RGB sensor initialization successful");
  } else { 
    Serial.println("RGB sensor initialization failed");
    RGBfailed = 1;
  }
}

void initBlueRobotics() {
  Wire.begin();
  if (sensor.init()) {
    sensor.setModel(MS5837::MS5837_30BA);
    sensor.setFluidDensity(997); // kg/m^3 (freshwater, 1029 for seawater)
    Serial.println("Pressure/Depth/Altitude/Temperature sensor initialization successful");
  } else {
    Serial.println("Pressure/Depth/Altitude/Temperature sensor initialization failed");
    sensorFailed = 1;
  }
    
  }

void printHeaders() {
    Serial.println("Time (s), External Temperature (deg C), Pressure (mbar), Depth (m), Altitude (m), Red, Green, Blue");
}

void checkSensors() {
  if (RGB.readRed() == 65535 || RGB.readRed() == 0 || RGB.readGreen() == 65535 || RGB.readGreen() == 0 || RGB.readBlue() == 65535 || RGB.readBlue() == 0) {
    RGBfailed = 1;
  }
  
  if (RGBfailed) {
    if (RGB.init()) {
      Serial.println("RGB sensor initialized");
      RGBfailed = 0;
    }
  }

  if (sensor.altitude() < -25000) {
    sensorFailed = 1;
  }

  if (sensorFailed) {
    if (sensor.init()) {
      Serial.println("Pressure/depth/altitude/external temperature sensor initialized");
      sensorFailed = 0;
    }
  }
}
