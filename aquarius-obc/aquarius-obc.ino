// Pressure / depth / altitude / external temperature sensor libraries
#include <MS5837.h>
#include <Wire.h>
#include <SparkFunTSL2561.h>

// RGB sensor library
#include <SFE_ISL29125.h>

// SD card libraries
#include <SD.h>
#include <SD_t3.h>
#include <SPI.h>

#define N 255

// Create an SFE_TSL2561 object, here called "light": (DO NOT DELETE)

SFE_TSL2561 light;

// Global variables:

boolean gain;     // Gain setting, 0 = X1, 1 = X16;
unsigned int ms;  // Integration ("shutter") time in milliseconds

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

  // ---- CODE FOR RGB SENSOR ----

  light.begin();

  // Get factory ID from sensor:
  // (Just for fun, you don't need to do this to operate the sensor)

  unsigned char ID;
  
  if (light.getID(ID))
  {
    Serial.print("Got factory ID: 0X");
    Serial.print(ID,HEX);
    Serial.println(", should be 0X5X");
  }
  // Most library commands will return true if communications was successful,
  // and false if there was a problem. You can ignore this returned value,
  // or check whether a command worked correctly and retrieve an error code:
  else
  {
    byte error = light.getError();
    printError(error);
  }

  // The light sensor has a default integration time of 402ms,
  // and a default gain of low (1X).
  
  // If you would like to change either of these, you can
  // do so using the setTiming() command.
  
  // If gain = false (0), device is set to low gain (1X)
  // If gain = high (1), device is set to high gain (16X)

  gain = 2;

  // If time = 0, integration will be 13.7ms
  // If time = 1, integration will be 101ms
  // If time = 2, integration will be 402ms
  // If time = 3, use manual start / stop to perform your own integration

  unsigned char time = 2;

  // setTiming() will set the third parameter (ms) to the
  // requested integration time in ms (this will be useful later):
  
  Serial.println("Set timing...");
  light.setTiming(gain,time,ms);

  // To start taking measurements, power up the sensor:
  
  Serial.println("Powerup...");
  light.setPowerUp();
  
  // The sensor will now gather light during the integration time.
  // After the specified time, you can retrieve the result from the sensor.
  // Once a measurement occurs, another integration period will start.

  // ---- END OF SETUP CODE FOR RGB SENSOR ----
}

void loop() {
  // ---- RGB SENSOR INITILIZATION ----
  // Wait between measurements before retrieving the result
  // (You can also configure the sensor to issue an interrupt
  // when measurements are complete)
  
  // This sketch uses the TSL2561's built-in integration timer.
  // You can also perform your own manual integration timing
  // by setting "time" to 3 (manual) in setTiming(),
  // then performing a manualStart() and a manualStop() as in the below
  // commented statements:
  
  // ms = 1000;
  // light.manualStart();
  delay(ms);
  // light.manualStop();
  
  // Once integration is complete, we'll retrieve the data.
  
  // There are two light sensors on the device, one for visible light
  // and one for infrared. Both sensors are needed for lux calculations.
  
  // Retrieve the data from the device:

  unsigned int data0, data1;

  // -- END OF RGB SENSOR INITILIZATION
   
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

  // ---- RGB SENSOR DO NOT DELETE ----
  if (light.getData(data0,data1))
  {
    // getData() returned true, communication was successful

    LOG = SD.open(FILENAME, FILE_WRITE);
    Serial.print("data0: ");
    Serial.print(data0);
    Serial.print(" data1: ");
    Serial.print(data1);
  
    // To calculate lux, pass all your settings and readings
    // to the getLux() function.
    
    // The getLux() function will return 1 if the calculation
    // was successful, or 0 if one or both of the sensors was
    // saturated (too much light). If this happens, you can
    // reduce the integration time and/or gain.
    // For more information see the hookup guide at: https://learn.sparkfun.com/tutorials/getting-started-with-the-tsl2561-luminosity-sensor
  
    double lux;    // Resulting lux value
    boolean good;  // True if neither sensor is saturated
    
    // Perform lux calculation:

    good = light.getLux(gain,ms,data0,data1,lux);
    
    // Print out the results:
  
    Serial.print(" lux: ");
    Serial.print(lux);
    if (good) Serial.println(" (good)"); else Serial.println(" (BAD)");
    Serial.println();
    LOG.close();
  }
  else
  {
    // getData() returned false because of an I2C error, inform the user.

    byte error = light.getError();
    printError(error);
  }
  // ---- END OF RGB SENSOR ----
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

// RGB SENSOR CLASS FOR WHEN ERRORS OCCUR
void printError(byte error)
  // If there's an I2C error, this function will
  // print out an explanation.
{
  Serial.print("I2C error: ");
  Serial.print(error,DEC);
  Serial.print(", ");
  
  switch(error)
  {
    case 0:
      Serial.println("success");
      break;
    case 1:
      Serial.println("data too long for transmit buffer");
      break;
    case 2:
      Serial.println("received NACK on address (disconnected?)");
      break;
    case 3:
      Serial.println("received NACK on data");
      break;
    case 4:
      Serial.println("other error");
      break;
    default:
      Serial.println("unknown error");
  }
}






















void loop()
{
  
  
  if (light.getData(data0,data1))
  {
    // getData() returned true, communication was successful
    
    Serial.print("data0: ");
    Serial.print(data0);
    Serial.print(" data1: ");
    Serial.print(data1);
  
    // To calculate lux, pass all your settings and readings
    // to the getLux() function.
    
    // The getLux() function will return 1 if the calculation
    // was successful, or 0 if one or both of the sensors was
    // saturated (too much light). If this happens, you can
    // reduce the integration time and/or gain.
    // For more information see the hookup guide at: https://learn.sparkfun.com/tutorials/getting-started-with-the-tsl2561-luminosity-sensor
  
    double lux;    // Resulting lux value
    boolean good;  // True if neither sensor is saturated
    
    // Perform lux calculation:

    good = light.getLux(gain,ms,data0,data1,lux);
    
    // Print out the results:
  
    Serial.print(" lux: ");
    Serial.print(lux);
    if (good) Serial.println(" (good)"); else Serial.println(" (BAD)");
  }
  else
  {
    // getData() returned false because of an I2C error, inform the user.

    byte error = light.getError();
    printError(error);
  }
}


