#include <BLEUtils.h>

using namespace std;

namespace M5Constants
{
    BLEUUID SERVICE_UUID = BLEUUID((uint16_t)0x181A);
    BLEUUID TEMP_UUID = BLEUUID((uint16_t)0x2A6E);
    BLEUUID HUMID_UUID = BLEUUID((uint16_t)0x2A6F);
    BLEUUID DESC_UUID = BLEUUID((uint16_t)0x2901);

    string DEVICE_NAME = "M5-Sensor";
    string HUMID_SERVICE_TAG = "HUMID DATA";
    string TEMP_SERVICE_TAG = "TEMP DATA";
}