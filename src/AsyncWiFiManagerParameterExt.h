#ifndef _7U_WIFIMANAGERPARAMEXT_H
#define _7U_WIFIMANAGERPARAMEXT_H

#include <ESPAsyncWiFiManager.h>

namespace siebenuhr {

class AsyncWiFiManagerParameterExt : public AsyncWiFiManagerParameter {
    using AsyncWiFiManagerParameter::AsyncWiFiManagerParameter;

public:
    void setCustomHTML(const char *custom) {
        this->_customHTML = custom;
    };
};

}

#endif