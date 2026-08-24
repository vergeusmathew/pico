#ifndef WIFI_HPP
#define WIFI_HPP

#include "pico/stdlib.h"

// Expose the global internet calibration worker function to main loop structures
void Core1_Wifi_Worker_Thread();
bool IsMobileHotspotAvailable(const char* target_ssid);
void SyncRtcWithInternetTime();

#endif // WIFI_HPP

