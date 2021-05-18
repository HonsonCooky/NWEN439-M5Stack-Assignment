#include <M5Stack.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
bool deviceConnected = false;

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
    M5.Power.reset();
  }
};

class TempCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *tempCharacteristic)
  {
    int8_t tempOut = random(100) - 40;
    tempCharacteristic->setValue((uint8_t *)&tempOut, 2);
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
    humidCharacteristic->setValue((uint8_t *)&humidOut, 2);
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
  BLEDevice::init("M5-Sensor");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x181A));

  if (M5.BtnA.isPressed())
  {

    // TEMP CHARACTERISTIC
    BLECharacteristic *tempCharacteristic = pService->createCharacteristic(BLEUUID((uint16_t)0x2A6E),
                                                                           BLECharacteristic::PROPERTY_READ |
                                                                               BLECharacteristic::PROPERTY_NOTIFY);
    tempCharacteristic->setCallbacks(new TempCallbacks());
    // Temp Descriptor
    BLEDescriptor *tempDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    tempDescriptor->setValue("Temperature -40 to 60°C");
    tempCharacteristic->addDescriptor(tempDescriptor);
  }
  else
  {
    // HUMIDITY CHARACTERISTIC
    BLECharacteristic *humidCharacteristic = pService->createCharacteristic(BLEUUID((uint16_t)0x2A6F),
                                                                            BLECharacteristic::PROPERTY_READ |
                                                                                BLECharacteristic::PROPERTY_NOTIFY);
    humidCharacteristic->setCallbacks(new HumidCallbacks());
    // Humidity Descriptor
    BLEDescriptor *humidDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    humidDescriptor->setValue("Humidity 0 to 100%");
    humidCharacteristic->addDescriptor(humidDescriptor);
  }
  pService->start();
  BLEDevice::getAdvertising()->addServiceUUID(BLEUUID((uint16_t)0x181A));
  pServer->getAdvertising()->start();
}

void loop()
{
  M5.update();
}