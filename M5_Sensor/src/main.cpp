#include <constants.h>
#include <M5Stack.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "time.h"

bool typeChoosen = false;
bool dutyChoosen = false;
BLEServer *pServer;
BLEService *pService;
BLEAdvertisementData *pData;
bool dutyCycleOn = false;
enum BLESensorState
{
  standby,
  advertising,
  connected
};
BLESensorState curState;
time_t timestamp = 0;

using namespace M5Constants;

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
    curState = connected;
  };

  /**
   * Prints "Disconnected" to the M5Stack. Then, with the global
   * server, starts advertising it's services again.
   */
  void onDisconnect(BLEServer *pServer)
  {
    M5.Lcd.println("Disconnected");
    curState = standby;
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

void updateState(BLESensorState newState)
{
  curState = newState;
  time(&timestamp);
}

void stateLoop()
{
  switch (curState)
  {
  case connected:
    break;
  case standby:
    updateState(advertising);
    pServer->getAdvertising()->start();
    break;
  case advertising:
    if (dutyCycleOn) // If we are duty cycling
    {
      // Calculate the difference in time since last update
      time_t curTime = 0;
      time(&curTime);
      double diff = difftime(curTime, timestamp);
      if (diff >= 5) // If the difference is 5 seconds or more...
      {
        updateState(standby);           // Then we need to sleep for...
        int sleepTime = 5 - (diff - 5); // The 5 second intervals inclusive.
        M5.Power.lightSleep(SLEEP_SEC(sleepTime));
      }
    }
    break; // If we are not duty cycling, then just ignore this state.
  }
}

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