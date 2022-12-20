#include <WiFi.h>
#include <FirebaseESP32.h>
#include "DHT.h"
#include <DFRobot_MAX30102.h>
#include <LiquidCrystal_I2C.h>
#include "ThingSpeak.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "xxxxxxxxxxxxxxxx"
#define WIFI_PASSWORD "xxxxxxxxxxxxx"

// Insert Firebase project API Key
#define API_KEY "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "xxxxxxxxxxxxxxxxxxxxxx"
#define USER_PASSWORD "xxxxxxxxxxxxxxxxxxxxx"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

#define LM35_Sensor1 35

DFRobot_MAX30102 particleSensor;

LiquidCrystal_I2C lcd(0x3F,16,2);

WiFiClient  client;

unsigned long myChannelNumber = xxxxxxxxxxxxxxxxxxx;
const char * myWriteAPIKey = "xxxxxxxxxxxxxxxxx";

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Variables to save database paths
String databasePath;
String bodytempPath;
String heartratePath;
String bloodoxygenPath;
String humPath;
String tempPath;

float t;
float h;
float f;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;



// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(5000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Write float values to the database
void sendFloat(String path, float value){
  if (Firebase.RTDB.setFloat(&fbdo, path.c_str(), value)){
    Serial.print("Writing value: ");
    Serial.print (value);
    Serial.print(" on the following path: ");
    Serial.println(path);
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  }
  else {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

void setup(){
  Serial.begin(115200);

  // Initialize BME280 sensor
 
  initWiFi();

  lcd.init();         // initialize the lcd

  lcd.backlight();    // open the backlight

  ThingSpeak.begin(client);

   while (!particleSensor.begin()) {
    lcd.print("MAX30102 was not found");
    Serial.println("MAX30102 was not found");
    delay(100);
  }

    particleSensor.sensorConfiguration(/*ledBrightness=*/50, /*sampleAverage=*/SAMPLEAVG_4, \
                        /*ledMode=*/MODE_MULTILED, /*sampleRate=*/SAMPLERATE_100, \
                        /*pulseWidth=*/PULSEWIDTH_411, /*adcRange=*/ADCRANGE_16384);


  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid;

  // Update database path for sensor readings
  bodytempPath = databasePath + "/tc";
  heartratePath = databasePath + "/hr";
  bloodoxygenPath = databasePath + "/bo";
  tempPath = databasePath + "/t"; // --> UsersData/<user_uid>/temperature
  humPath = databasePath + "/h"; // --> UsersData/<user_uid>/humidity
}

void loop()
{
 
if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    // Get the voltage reading from the LM35
  int reading = analogRead(LM35_Sensor1);

  // Convert that reading into voltage
  float voltage = reading * (2.0 / 1024.0);

  // Convert the voltage into the temperature in Celsius
  float tc = voltage * 25;

  int32_t bo; //SPO2
  int8_t bov; //Flag to display if SPO2 calculation is valid
  int32_t hr; //Heart-rate
  int8_t hrv; //Flag to display if heart-rate calculation is valid 


   t = dht.readTemperature();
   
   h = dht.readHumidity();

   f = dht.readTemperature(true);

   // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  
  int hf = dht.computeHeatIndex(f, h);
 
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.println(F("Wait about four seconds"));
  particleSensor.heartrateAndOxygenSaturation(/**SPO2=*/&bo, /**SPO2Valid=*/&bov, /**heartRate=*/&hr, /**heartRateValid=*/&hrv);
  //Print result 
  Serial.print(F("heartRate="));
  Serial.print(hr, DEC);
  Serial.print(F("|"));  
  
  Serial.print(F("|"));
  delay(1000);
  
  Serial.print(F("BloodOxygen="));
  Serial.print(bo, DEC);
  Serial.print(F("|"));
  
  Serial.println(F("|"));
  delay(1000);

    // Print the temperature in Celsius
  Serial.print("Body Temperature: ");
  Serial.print(tc);
  Serial.println("c |");
  delay(1000);

  Serial.print(F("Room Humidity: "));
  Serial.print(h);
  Serial.println("% |");  
  delay(1000);
  Serial.print(F("Room Temperature: "));
  Serial.print(t);
  Serial.println(F("F |"));
  delay(1000);
  
  Serial.print(F(" Heat index: "));
  Serial.print(hic);
  Serial.print(F("C "));
  
  delay(1000);
  
  if (isnan(tc) || isnan(hr) || isnan(bo) || isnan(h) || isnan(t)) {
    lcd.setCursor(0, 0);
    lcd.print("Failed");
  } else {
    lcd.setCursor(1, 1);  // display position
    lcd.print("BodyTemp ");
    lcd.print(tc);     // display the temperature
    lcd.print("c");
    delay(2000);
    lcd.clear();

    lcd.setCursor(2, 1);  // display position
    lcd.print("HR ");
    lcd.print(hr);      // display the humidity
    lcd.print("BPM");
    delay(2000);
    lcd.clear();
    
    lcd.setCursor(1, 1);  // display position
    lcd.print("SPO2 ");
    lcd.print(bo);      // display the humidity
    lcd.print("%");
    delay(2000);
    lcd.clear();

    lcd.setCursor(2, 1);  // display position
    lcd.print("RoomHum ");
    lcd.print(h);      // display the humidity
    lcd.print("%");
    delay(2000);
    lcd.clear();

    lcd.setCursor(1, 1);  // display position
    lcd.print("RoomTemp ");
    lcd.print(t);      // display the humidity
    lcd.print("f");
    delay(2000);
    lcd.clear();

  }

  delay(1000);

 
    sendFloat(bodytempPath, tc);
    sendFloat(heartratePath, hr);
    sendFloat(bloodoxygenPath, bo);
    sendFloat(humPath, h);
    sendFloat(tempPath, t);

    delay(1000);

    // set the fields with the values
    ThingSpeak.setField(1, tc);
    ThingSpeak.setField(2, hr);
    ThingSpeak.setField(3, bo);
    ThingSpeak.setField(4, h);
    ThingSpeak.setField(5, t);
    
    
    
    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    lastTime = millis();
    
  }
}