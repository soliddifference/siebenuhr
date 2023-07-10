#ifndef _7U_APCONTROLLER_H
#define _7U_APCONTROLLER_H

#include "SiebenUhr.h"
#include "AsyncWiFiManagerParameterExt.h"
#include <ESPAsyncWiFiManager.h>

namespace siebenuhr {

class APController {
public:
    APController();
    ~APController() = default;

    static APController* getInstance();

    bool begin();

    void getNetworkInfo();
    inline int getSelectedTimeZone() { return _nSelectedTimeZoneID; };

private:
    bool setupAPCaptivePortal();
    String buildTimezoneCheckboxOption(int default_tz);

    void resetWifiSettingsAndReboot(AsyncWiFiManager* pWiFiManager);

    static APController* _pInstance;
    String _sDropDownTimeZoneHTML;
    AsyncWiFiManagerParameter *_pCustomMQTTServer;
    AsyncWiFiManagerParameter *_pCustomMQTTUser;
    AsyncWiFiManagerParameter *_pCustomMQTTPassword;
    AsyncWiFiManagerParameterExt *_pCustomTZDropdown;
    AsyncWiFiManagerParameter *_pCustomTZHidden;

    int _nSelectedTimeZoneID;

    char _sIdentifier[100];
};

}

#endif