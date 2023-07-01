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

    bool begin();
    void getNetworkInfo();

private:
    bool setupAPCaptivePortal();
    String buildTimezoneCheckboxOption(int default_tz);

    void resetWifiSettingsAndReboot(AsyncWiFiManager* pWiFiManager);

    static APController* _pInstance;

    char _sIdentifier[100];
    bool _bUIConfigured;
};

}

#endif