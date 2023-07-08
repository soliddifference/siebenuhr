#include "HATextExt.h"
#ifndef EX_ARDUINOHA_TEXT

#include <HAMqtt.h>
#include <utils/HASerializer.h>

const char HAComponentText[] PROGMEM = {"text"};

HATextExt::HATextExt(const char* uniqueId) :
    HABaseDeviceType(AHATOFSTR(HAComponentText), uniqueId),
    _class(nullptr),
    _icon(nullptr),
    _retain(false),
    _optimistic(false),
    _currentState(),
    _commandTextCallback(nullptr)
{

}

bool HATextExt::setState(const String state, const bool force)
{
    if (!force && state == _currentState) {
        return true;
    }

    if (publishState(state)) {
        _currentState = state;
        return true;
    }

    return false;
}

void HATextExt::buildSerializer()
{
    if (_serializer || !uniqueId()) {
        return;
    }

    _serializer = new HASerializer(this, 15); // 15 - max properties nb

    _serializer->set(AHATOFSTR(HANameProperty), _name);
    _serializer->set(AHATOFSTR(HAUniqueIdProperty), _uniqueId);
    _serializer->set(AHATOFSTR(HADeviceClassProperty), _class);
    _serializer->set(AHATOFSTR(HAIconProperty), _icon);
    //_serializer->set(AHATOFSTR(HAUnitOfMeasurementProperty), _unitOfMeasurement);
    // _serializer->set(
    //     AHATOFSTR(HAModeProperty),
    //     getModeProperty(),
    //     HASerializer::ProgmemPropertyValue
    // );
    // _serializer->set(
    //     AHATOFSTR(HACommandTemplateProperty),
    //     getCommandTemplate(),
    //     HASerializer::ProgmemPropertyValue
    // );

    // if (_minValue.isSet()) {
    //     _serializer->set(
    //         AHATOFSTR(HAMinProperty),
    //         &_minValue,
    //         HASerializer::NumberPropertyType
    //     );
    // }

    // if (_maxValue.isSet()) {
    //     _serializer->set(
    //         AHATOFSTR(HAMaxProperty),
    //         &_maxValue,
    //         HASerializer::NumberPropertyType
    //     );
    // }

    // if (_step.isSet()) {
    //     _serializer->set(
    //         AHATOFSTR(HAStepProperty),
    //         &_step,
    //         HASerializer::NumberPropertyType
    //     );
    // }

    if (_retain) {
        _serializer->set(
            AHATOFSTR(HARetainProperty),
            &_retain,
            HASerializer::BoolPropertyType
        );
    }

    if (_optimistic) {
        _serializer->set(
            AHATOFSTR(HAOptimisticProperty),
            &_optimistic,
            HASerializer::BoolPropertyType
        );
    }

    _serializer->set(HASerializer::WithDevice);
    _serializer->set(HASerializer::WithAvailability);
    _serializer->topic(AHATOFSTR(HAStateTopic));
    _serializer->topic(AHATOFSTR(HACommandTopic));
}

void HATextExt::onMqttConnected()
{
    if (!uniqueId()) {
        return;
    }

    publishConfig();
    publishAvailability();

    if (!_retain) {
        publishState(_currentState);
    }

    subscribeTopic(uniqueId(), AHATOFSTR(HACommandTopic));
}

void HATextExt::onMqttMessage(
    const char* topic,
    const uint8_t* payload,
    const uint16_t length
)
{
    if (HASerializer::compareDataTopics(
        topic,
        uniqueId(),
        AHATOFSTR(HACommandTopic)
    )) {
        handleCommand(payload, length);
    }
}

bool HATextExt::publishState(const String state)
{
    if (state.length() == 0) {
        return publishOnDataTopic(
            AHATOFSTR(HAStateTopic),
            AHATOFSTR(HAStateNone),
            true
        );
    }

    const uint8_t size = state.length();
    if (size == 0) {
         return false;
    }
    const char * str = state.c_str();

    return publishOnDataTopic(
        AHATOFSTR(HAStateTopic),
        str,
        true
    );
}


void HATextExt::handleCommand(const uint8_t* cmd, const uint16_t length)
{
    if (!_commandTextCallback) {
        return;
    }

    if (memcmp_P(cmd, HAStateNone, length) == 0) {
        _commandTextCallback(String(), this);
        
    } else {
        String text = String((char *)cmd);
        text = text.substring(0, (int)length);
        if (text.length() > 0) {
            _commandTextCallback(text, this);
        }
    }
}


const __FlashStringHelper* HATextExt::getModeProperty() const
{
    switch (_mode) {
    case ModeBox:
        return AHATOFSTR(HAModeBox);

    case ModeSlider:
        return AHATOFSTR(HAModeSlider);

    default:
        return nullptr;
    }
}

#endif
