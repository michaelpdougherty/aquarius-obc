// Pressure/depth sensor
#include <MS5837.h>
#include <Wire.h>

// DS18S20, DS18B20, DS1822 Temperature
#include <OneWire.h>

// RGB sensor
#include <SFE_ISL29125.h>

// SD card
#include <SD.h>
#include <SD_t3.h>
#include <SPI.h>

// Internal temp/humidity
#include <dht.h>

#define N 255

///// USER CAN CHANGE VALUES BELOW /////

// define pins
const unsigned int DHT_PIN = 2; // DHT22 internal temperature and humidity sensor
const unsigned int DS_PIN = 10; // DS18 external temperature sensor

// set default filename structure
String FILE_BODY = "LOG";
String FILE_EXTENSION = ".TXT";

////////////////////////////////////////


// Create filename vars
unsigned int FILE_NUMBER_INT = 0;
String FILE_NUMBER_STRING = "000";

String testFilename = FILE_BODY + FILE_NUMBER_STRING + FILE_EXTENSION;

char FILENAME[N] = {};


// set communication rate
const unsigned int BAUD_RATE = 9600;
const unsigned int DELAY = 1000;

// create objects
File LOG;
dht DHT;
SFE_ISL29125 RGB;
MS5837 sensor;

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
  initRGB();

  // initialize pressure, depth, temperature, and altitude sensor
  initBlueRobotics();

  // Print CSV headers
  printHeaders();

  // will ya give it a SECOND?!
  delay(DELAY);
  seconds++;
}

void loop() {

  // INTERNAL TEMPERATURE AND HUMIDITY
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

  // EXTERNAL TEMPERATURE
  float externalDegreesCelsius = getExternalTemp();

  // DEPTH / PRESSURE
  // Update pressure and temperature readings
  sensor.read();
  // get vars
  float pressure = sensor.pressure(); // mbar
  float externalDegreesCelsius1 = sensor.temperature(); // deg C - Bar30
  float depth = sensor.depth(); // m
  float altitude = sensor.altitude(); // m above mean sea level
  
  
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
  LOG.print(",");
  LOG.print(externalDegreesCelsius);
  LOG.print(",");
  LOG.print(externalDegreesCelsius1);
  LOG.print(",");
  LOG.print(pressure);
  LOG.print(",");
  LOG.print(depth);
  LOG.print(",");
  LOG.print(altitude);

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
    Serial.print(",");
    Serial.print(externalDegreesCelsius);
    Serial.print(",");
    Serial.print(externalDegreesCelsius1);
    Serial.print(",");
    Serial.print(pressure);
    Serial.print(",");
    Serial.print(depth);
    Serial.print(",");
    Serial.print(altitude);
    Serial.println();
  }

  // delay and increment timer
  delay(DELAY);
  seconds++;  
}

void initSD() {
  Serial.print("Initializing SD card... ");
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("NO SD CARD");
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
  }
}

void initBlueRobotics() {
  Wire.begin();

  sensor.init();
  
  sensor.setModel(MS5837::MS5837_30BA);
  sensor.setFluidDensity(997); // kg/m^3 (freshwater, 1029 for seawater)
}

void printHeaders() {
    Serial.println("Time (s), Internal Temp (C), Internal Humidity (%), Red, Green, Blue, External Temp #1 - DS18B20 (C), External Temp #2 - Bar30 (C), Pressure (mbar), Depth (m), Altitude (m)");
}

float getExternalTemp(void) {
  OneWire  ds(DS_PIN);  // on pin 10 (a 4.7K resistor is necessary)

  
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return 0;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return 0;
  }
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return 0;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  return (float)raw / 16.0;
}
