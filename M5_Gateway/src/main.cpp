/**
 * Base code for gateway node development in NWEN439
 * You will need to modify this code to implement the necessary
 * features specified in the handout.
 * 
 * Important: The compiled code for this module is quite
 * big so you will need to change the partition scheme to 
 * "No OTA (Large App)". To do this, click "Tools" on 
 * Arduino Studio, then select "No OTA (Large App)" from 
 * Partition Scheme.
 */
#include <constants.h>
#include <M5Stack.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "time.h"

using namespace M5Constants;

/**
 * SSID and password of the WIFI network
 */

const char *WIFI_SSID = "The Ira Street Hooligans";
const char *WIFI_PASSWORD = "D1CKD33P1NCR4ZY";

/**
 * Time synchronization related stuff
 */
const char *ntpServer1 = "oceania.pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = (13) * 60 * 60;
const int daylightOffset_sec = 3600;

/**
 * Unix timestamp
 * Once gateway is time synced, you can use this timestamp
 */
RTC_DATA_ATTR time_t timestamp = 0;

/**
 * BLE scanner objects
 */
BLEScan *scanner;
BLEClient *client;
BLEAdvertisedDevice *aDevice;
string searchingFor;

/**
 * IP address, UDP and Coap instances
 * Must be global since they are used by callbacks
 */
IPAddress wifi_ip;
WiFiUDP udp;
Coap coap(udp);

//--------------------------------------------------------------------------
// SCANNING FOR SERVICES
//--------------------------------------------------------------------------

/**
 * Scan for BLE servers and find all services that advertises the service we are looking for.
 */
class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(SERVICE_UUID))
    {
      string data = advertisedDevice.getServiceData();
      if (data.compare(searchingFor) == 0)
      {
        aDevice = new BLEAdvertisedDevice(advertisedDevice);
        BLEDevice::getScan()->stop();
      }
    }
  }
};

//--------------------------------------------------------------------------
// REPLYING TO COAP
//--------------------------------------------------------------------------

/** 
 * readAttr:
 * - Given a boolean and service to read from
 * - if the service is null, then we haven't found a service we need
 * - if we are looking for temp, but no characteristic is available
 *      then we haven't found the service
 * - if we are looking for temp and the characteristic is found then
 *      read the uint8_t from the service, convert it to a const char*
 *      and return the value
 * - do the same as temp for humidity
 */
static const char *readAttr(bool isTemp, BLERemoteService *from)
{
  if (from != nullptr)
  {
    if (isTemp)
    {
      if (from->getCharacteristic(TEMP_UUID) != nullptr)
        return strdup(String((int8_t)from->getCharacteristic(TEMP_UUID)->readUInt8()).c_str());
      else
        return "No Temperature Reading";
    }
    else
    {
      if (from->getCharacteristic(HUMID_UUID) != nullptr)
        return strdup(String((int8_t)from->getCharacteristic(HUMID_UUID)->readUInt8()).c_str());
      else
        return "No Humidity Reading";
    }
  }
  else
    return "No Sensor Found";
}

/**
 * getRes:
 * - Scan for devices
 * - Connect to device
 * - Get the result we are looking for
 * - Return the result
 */
static const char *getRes(bool isTemp)
{
  // Start the scanner
  scanner->start(5, false);
  M5.Lcd.println("Scan done!");
  scanner->clearResults(); // delete results fromBLEScan buffer to release memory

  if (client == nullptr || aDevice == nullptr)
  {
    return "No Devices Found";
  }

  // Connect to the device
  M5.Lcd.println("Connecting...");
  client->connect(aDevice);

  // Get Service
  M5.Lcd.println("Getting Service...");
  BLERemoteService *service = client->getService(SERVICE_UUID);
  const char *res = readAttr(isTemp, service);

  // Disconnect from device and reset device
  client->disconnect();
  aDevice = nullptr;

  // Return the result
  return res;
}

/**
 * Temperature CoAP Callback:
 * - Call getRes (see above) for result
 * - Send the result to the CoAP server
 */
void callback_temp(CoapPacket &packet, IPAddress ip, int port)
{
  // Print status to know we recieved the call
  M5.Lcd.println("CoAP /temp query received");
  searchingFor = TEMP_SERVICE_TAG;
  const char *res = getRes(true);

  // Send response
  int ret = coap.sendResponse(ip, port, packet.messageid, res, strlen(res),
                              COAP_CONTENT, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);

  // Print status of sent response
  if (ret)
    M5.Lcd.println("Sent response");
  else
    M5.Lcd.println("Failed to send response");
}

/**
 * Humidity CoAP Callback:
 * - Call getRes (see above) for result
 * - Send the result to the CoAP server
 */
void callback_humidity(CoapPacket &packet, IPAddress ip, int port)
{
  // Print status to know we recieved the call
  M5.Lcd.println("CoAP /humidity query received");
  searchingFor = HUMID_SERVICE_TAG;
  const char *res = getRes(false);

  // Send response
  int ret = coap.sendResponse(ip, port, packet.messageid, res, strlen(res),
                              COAP_CONTENT, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);

  // Print status of sent response
  if (ret)
    M5.Lcd.println("Sent response");
  else
    M5.Lcd.println("Failed to send response");
}

/**
 * Unknown CoAP Callback
 */
void callback_unknown(CoapPacket &packet, IPAddress ip, int port)
{
  // Print status to know we recieved the call
  const char *res = "Unknown Command";

  // Send response
  int ret = coap.sendResponse(ip, port, packet.messageid, res, strlen(res),
                              COAP_CONTENT, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);

  // Print status of sent response
  if (ret)
    M5.Lcd.println("Sent response");
  else
    M5.Lcd.println("Failed to send response");
}

//--------------------------------------------------------------------------
// SETUP AND LOOP
//--------------------------------------------------------------------------

/**
 * Setup:
 * - Start the M5Stack
 * - Initialize the BLE device, and BLE scanner
 * - Create a client to connect to BLE devices
 * - Initalize WIFI
 * - Synchronize time
 * - Initalize CoAP 
 */
void setup()
{
  // Initialize the M5Stack object
  M5.begin();
  M5.Power.begin();
  M5.Lcd.clear();
  M5.Lcd.setBrightness(30);

  // Initialize BLE
  BLEDevice::init("M5-Gateway");

  // Create BLE scanner
  scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  scanner->setActiveScan(true); //active scan uses more power, but get results faster
  scanner->setInterval(100);
  scanner->setWindow(99); // less or equal setInterval value

  // Initalize the client to read
  client = BLEDevice::createClient();

  // Initialize WIFI
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
    delay(1000);
  M5.Lcd.print("Connected to WIFI with IP address ");
  wifi_ip = WiFi.localIP();
  M5.Lcd.println(wifi_ip);

  // Do time sync. Make sure gateway can connect to
  // Internet (i.e., your WiFi AP must be connected to Internet)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  delay(2000);
  time(&timestamp);
  M5.Lcd.println("Current time is " + String(timestamp));

  // CoAP server implementation
  coap.server(callback_temp, "temp");
  coap.server(callback_humidity, "humidity");
  coap.response(callback_unknown);
  coap.start();
}

/**
 * Looping:
 * Wait 1 second
 * run the CoAP loop
 */
void loop()
{
  delay(1000);
  coap.loop();
}