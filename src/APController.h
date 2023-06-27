#ifndef _7U_APCONTROLLER_H
#define _7U_APCONTROLLER_H

#include "SiebenUhr.h"
#include <ESPAsyncWiFiManager.h>

namespace siebenuhr {

class APController {
public:
    APController();
    ~APController() = default;

    static APController* getInstance();

    void begin(AsyncWiFiManager* pWiFiManager);

    void getNetworkInfo();

private:
    void setupWifi(AsyncWiFiManager* pWiFiManager);
    void resetWifiSettingsAndReboot(AsyncWiFiManager* pWiFiManager);

    static APController* _pInstance;

    char _sIdentifier[100];
    String _cTimezone;
};

}

#endif