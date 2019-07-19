#include <SD.h>
//#include <SD_t3.h>

#include <dht.h>
#include <SPI.h>

#define N 255


// set communication rate
const unsigned int BAUD_RATE = 9600;
const unsigned int DELAY = 1000;

// define pins
const unsigned int DHT_PIN = 2;

// create File object
File LOG;
dht DHT;

// init timer
unsigned int seconds = 0;

// get default filename
const char* FILE_BODY = "LOG";
const char* EXTENSION = ".TXT";
const char* DEFAULT_FILENAME = strcat(FILE_BODY, EXTENSION);

// initialize real filename
char* FILENAME = DEFAULT_FILENAME;

void setup() {
  delay(5000);
  
  // begin serial communication
  Serial.begin(BAUD_RATE);

  // give serial time to initialize
  delay(5000);

  // get filename
  FILENAME = getFilename();

  // initialize SD card
  initSD();

  // ensure SD can be written to
  checkSD();

  // will ya give it a SECOND?!
  delay(DELAY);
  seconds++;
}

void loop() {
  // update DHT22 object
  int check = DHT.read22(DHT_PIN);

  // get temperature in degrees Celsius
  float internalDegreesCelsius = DHT.temperature;

  // get internal humidity percentage
  float internalHumidityPercentage = DHT.humidity;
  
  // open file for writing
  LOG = SD.open(FILENAME, FILE_WRITE);

  // write data to file
  LOG.print(seconds);
  LOG.print(",");
  LOG.print(internalDegreesCelsius);
  LOG.print(",");
  LOG.print(internalHumidityPercentage);

  LOG.println();

  // close file
  LOG.close();

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

    // write headers
    for (int i = 0; i < 25; i++)
      LOG.print("=");
    LOG.println();
    
    // close the file
    LOG.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error
    Serial.print("Error opening ");
    Serial.println(FILENAME);
  }
}

char* getFilename() {
  // begin checking at 0
  int logNumber = 0;

  // init filename
  char filename[255] = {};
  filename[255] = strcat(FILE_BODY, EXTENSION);
  
  // open root
  File root = SD.open("/");
  // iterate over each item
  while (true) {
    // create filename
    if (logNumber != 0)
      filename[255] = strcat(FILE_BODY, strcat(logNumber, EXTENSION));
    File entry = root.openNextFile();
    if (!entry) {
      // reached last file - close files and return filename
      entry.close();
      root.close();
      return filename;
    }
    if (strcmp(entry.name(), filename)) {
      // names are the same, try again
      logNumber++;
    }
  }
}
