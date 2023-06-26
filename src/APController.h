#ifndef _7U_APCONTROLLER_H
#define _7U_APCONTROLLER_H

#include "SiebenUhr.h"

namespace siebenuhr {

class APController {
public:
    APController();
    ~APController() = default;

    static APController* getInstance();

    void begin();

private:
    void setupWifi();
    void resetWifiSettingsAndReboot();

    static APController* _pInstance;

    char _sIdentifier[100];
    String _cTimezone;
};

}

#endif