#ifndef _7U_WIFIMANAGERPARAMEXT_H
#define _7U_WIFIMANAGERPARAMEXT_H

#include <ESPAsyncWiFiManager.h>

namespace siebenuhr {

class AsyncWiFiManagerParameterExt : public AsyncWiFiManagerParameter {
    using AsyncWiFiManagerParameter::AsyncWiFiManagerParameter;

public:
    void setDefaultValue(const char *defaultValue, unsigned int length) {
        this->_length = length;
        this->_value = new char[length + 1];

        for (unsigned int i = 0; i < length; i++) {
            this->_value[i] = 0;
        }

        if (defaultValue != NULL) {
            strncpy(this->_value, defaultValue, length);
        }        
    };

    void setCustomHTML(const char *custom) {
        this->_customHTML = custom;
    };
};

}

#endif