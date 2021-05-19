#include <constants.h>
#include <M5Stack.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

BLEServer *pServer = nullptr;
BLEService *pService = nullptr;
bool deviceConnected = false;

using namespace M5Constants;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    M5.Lcd.println("connect");
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    M5.Lcd.println("disconnect");
    deviceConnected = false;
    pServer->startAdvertising();
  }
};

class TempCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *tempCharacteristic)
  {
    int8_t tempOut = random(100) - 40;
    tempCharacteristic->setValue((uint8_t *)&tempOut, 1);
    tempCharacteristic->notify();
  }

  void onNotify(BLECharacteristic *pCharacteristic)
  {
    M5.Lcd.println("Read Temp");
  }
};

class HumidCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *humidCharacteristic)
  {
    int8_t humidOut = random(100);
    humidCharacteristic->setValue((uint8_t *)&humidOut, 1);
    humidCharacteristic->notify();
  }

  void onNotify(BLECharacteristic *pCharacteristic)
  {
    M5.Lcd.println("Read Humidity");
  }
};

void setup()
{
  // Data transmission rate in bps
  Serial.begin(115200);
  M5.begin();
  M5.Power.setWakeupButton(BUTTON_A_PIN);
  m5.Speaker.mute();
  M5.Lcd.clear();
  M5.Lcd.setBrightness(30);
  M5.Lcd.println("BLE start.");

  // Initalize the BLE Dervice
  // Create a Server with a
  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  pService = pServer->createService(SERVICE_UUID);

  BLEAdvertisementData *pData = new BLEAdvertisementData();
  pData->setCompleteServices(SERVICE_UUID);

  if (M5.BtnA.isPressed())
  {
    // TEMP CHARACTERISTIC
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
  else
  {
    // HUMID CHARACTERISTIC
    BLECharacteristic *humidCharacteristic = pService->createCharacteristic(HUMID_UUID,
                                                                            BLECharacteristic::PROPERTY_READ |
                                                                                BLECharacteristic::PROPERTY_NOTIFY);
    humidCharacteristic->setCallbacks(new HumidCallbacks());
    // HUMID Descriptor
    BLEDescriptor *humidDescriptor = new BLEDescriptor(DESC_UUID);
    humidDescriptor->setValue("Humidity 0 to 100\%");
    humidCharacteristic->addDescriptor(humidDescriptor);

    
    // Set data type
    pData->setServiceData(HUMID_UUID, HUMID_SERVICE_TAG);

    M5.Lcd.println("Humidity Sensor");
  }

    pService->start();
    pServer->getAdvertising()->setAdvertisementData(*pData);
    pServer->startAdvertising();
}

void loop()
{
  M5.update();
  if (M5.BtnB.isPressed())
  {
    M5.Power.reset();
  }
}