#include <constants.h>
#include <M5Stack.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "time.h"

enum BLESensorState
{
  standby,
  advertising,
  connected
};

// BLE Server Global Data
BLEServer *pServer;
BLEService *pService;
BLEAdvertisementData *pData;

// Duty Cycling Settings
bool dutyCycleOn = false;
BLESensorState curState;
time_t timestamp = 0;
const int ADVERT_TIME_SEC = 1;
const int SLEEP_TIME_SEC = 5;

using namespace M5Constants;

//-----------------------------------------------------------------------------
// Looping functionality
//-----------------------------------------------------------------------------

/**
 * Updating the state should also update the
 * timestamp, as such, this method ensures both are done
 * on state update.
 */
void updateState(BLESensorState newState)
{
  curState = newState;
  time(&timestamp);
}

/**
 * Depending on the state of the Sensor,
 * determine the next actions to be taken
 * during this time.
 */
void stateLoop()
{
  switch (curState)
  {
  case connected:
    break;
  case standby:
    pServer->getAdvertising()->start();
    updateState(advertising);
    break;
  case advertising:
    if (dutyCycleOn) // If we are duty cycling
    {
      // Calculate the difference in time since last update
      time_t curTime = 0;
      time(&curTime);
      int diff = difftime(curTime, timestamp);
      if (diff > ADVERT_TIME_SEC) // If the difference is ADVERT_TIME_SEC seconds or more...
      {
        updateState(standby);
        M5.Power.lightSleep(SLEEP_SEC(SLEEP_TIME_SEC)); // Sleep at the start of every standby.
      }
    } // If we are not duty cycling, then just ignore this state.
  }
}

//-----------------------------------------------------------------------------
// Callback functionality
//-----------------------------------------------------------------------------

/**
 * ServerCallbacks. Overwrites functionality for BLEServerCallbacks.
 */
class ServerCallbacks : public BLEServerCallbacks
{
  /**
   * Prints "Connected" to the M5Stack.
   */
  void onConnect(BLEServer *pServer)
  {
    M5.Lcd.println("Connected");
    updateState(connected);
  };

  /**
   * Prints "Disconnected" to the M5Stack. Then, with the global
   * server, starts advertising it's services again.
   */
  void onDisconnect(BLEServer *pServer)
  {
    M5.Lcd.println("Disconnected");
    updateState(standby);
  }
};

/**
 * Temperature Callbacks. Overwrites some functionality for BLECharacteristicCallbacks.
 */
class TempCallbacks : public BLECharacteristicCallbacks
{
  /**
   * Produce a new random number between -40 and 60 degrees celcius.
   * Set the characteristics value to said random number.
   * Notify that the number has been set.
   */
  void onRead(BLECharacteristic *tempCharacteristic)
  {
    int8_t tempOut = random(100) - 40;
    tempCharacteristic->setValue((uint8_t *)&tempOut, 1);
    tempCharacteristic->notify();
  }

  /**
   * Prints "Read Temp" to the M5Stack.
   */
  void onNotify(BLECharacteristic *pCharacteristic)
  {
    M5.Lcd.println("Read Temp");
  }
};

/**
 * Humidity Callbacks. Overwrites some functionality for BLECharacteristicCallbacks.
 */
class HumidCallbacks : public BLECharacteristicCallbacks
{
  /**
   * Produce a new random number between 0 and 100 percent.
   * Set the characteristics value to said random number.
   * Notify that the number has been set.
   */
  void onRead(BLECharacteristic *humidCharacteristic)
  {
    int8_t humidOut = random(100);
    humidCharacteristic->setValue((uint8_t *)&humidOut, 1);
    humidCharacteristic->notify();
  }

  /**
   * Print "Read Humidity" to the M5Stack
   */
  void onNotify(BLECharacteristic *pCharacteristic)
  {
    M5.Lcd.println("Read Humidity");
  }
};

//-----------------------------------------------------------------------------
// Configurations
//-----------------------------------------------------------------------------

/**
 * Setup the M5Stack
 */
void m5Config()
{
  Serial.begin(115200); // Data transmission rate in bps
  M5.begin();
  m5.Speaker.mute();
  M5.Lcd.clear();
  M5.Lcd.println("BLE start.");
}

/**
 * Configure the global services
 */
void serverConfig()
{
  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  pService = pServer->createService(SERVICE_UUID);

  pData = new BLEAdvertisementData();
  pData->setCompleteServices(SERVICE_UUID);
  curState = standby;
}

/**
 * Configure Temperature Characteristic
 */
void setupTemperature()
{
  // Temp CHARACTERISTIC
  BLECharacteristic *tempCharacteristic = pService->createCharacteristic(TEMP_UUID,
                                                                         BLECharacteristic::PROPERTY_READ |
                                                                             BLECharacteristic::PROPERTY_NOTIFY);
  tempCharacteristic->setCallbacks(new TempCallbacks());

  // Temp Descriptor
  BLEDescriptor *tempDescriptor = new BLEDescriptor(DESC_UUID);
  tempDescriptor->setValue("Temperature -40 to 60°C");
  tempCharacteristic->addDescriptor(tempDescriptor);

  // Set data type
  pData->setServiceData(TEMP_UUID, TEMP_SERVICE_TAG);

  M5.Lcd.println("Temperature Sensor");
}

/**
 * Configure Humidity Characteristic
 */
void setupHumitidy()
{
  // Humid CHARACTERISTIC
  BLECharacteristic *humidCharacteristic = pService->createCharacteristic(HUMID_UUID,
                                                                          BLECharacteristic::PROPERTY_READ |
                                                                              BLECharacteristic::PROPERTY_NOTIFY);
  humidCharacteristic->setCallbacks(new HumidCallbacks());

  // Humid Descriptor
  BLEDescriptor *humidDescriptor = new BLEDescriptor(DESC_UUID);
  humidDescriptor->setValue("Humidity 0 to 100\%");
  humidCharacteristic->addDescriptor(humidDescriptor);

  // Set data type
  pData->setServiceData(HUMID_UUID, HUMID_SERVICE_TAG);

  M5.Lcd.println("Humidity Sensor");
}

/**
 * Called during the setup process,
 * if certain buttons are pressed, alter the
 * state in which this device will run.
 *
 * ButtonA: If pressed, Temperature sensor, else, Humidity Sensor
 * ButtonC: If pressed, Duty Cycling On, else, Duty Cycling Off
 */
void multipleChoice()
{
  if (M5.BtnA.isPressed())
    setupTemperature();
  else
    setupHumitidy();

  if (M5.BtnC.isPressed())
  {
    M5.Lcd.println("Duty Cycle: On");
    dutyCycleOn = true;
  }
  else
  {
    M5.Lcd.println("Duty Cycle: Off");
    dutyCycleOn = false;
  }
}

//-----------------------------------------------------------------------------
// Base Setup and Loop functionality
//-----------------------------------------------------------------------------

/**
 * Setup:
 * Config the M5Stack.
 * Config the server.
 * Setup the environment (temp/humid).
 * Config and Start Advertising.
 */
void setup()
{
  m5Config();
  serverConfig();
  multipleChoice();
  pService->start();
  pServer->getAdvertising()->setAdvertisementData(*pData);
}

void loop()
{
  M5.update();
  stateLoop();
  if (M5.BtnB.isPressed())
  {
    M5.Power.reset();
  }
}