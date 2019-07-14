/*
 Copyright (c) 2013, Arne Bech
 All rights reserved.
 See license file for more info.
 */

#include "Debounce.h"


Debounce::Debounce() {
    this->configuredSwitchesNum = 0;
    this->bounceDelay = DEBOUNCE_DEFAULT_DELAY;
}

/* 
    Sets the delay for which we consider the input
    unusable/unstable after a rising/falling edge
*/
void Debounce::setBounceDelay(uint16_t delay) {
    this->bounceDelay = delay;
}


/*
    Adds an input to the debouncer. 
    pin - pin number of input
    mode - INPUT or INPUT_PULLUP
    func - function that gets called on state change
*/
uint8_t Debounce::addInput(uint8_t pin, uint8_t mode, void(*func)(bool, uint8_t)) {
    return this->addInput(pin, mode, func, DEBOUNCE_SETTING_NORMAL);
}

/*
    Same as above, with the addition of the settings param. See header for more info on settings.
    debounce.addInput(....., DEBOUNCE_SETTING_SKIP_RISING_EDGE | DEBOUNCE_SETTINGS_INVERT);

*/
uint8_t Debounce::addInput(uint8_t pin, uint8_t mode, void(*func)(bool, uint8_t), uint8_t settings) {
    
    if (this->configuredSwitchesNum >= DEBOUNCE_MAX_CAPACITY) {
        return -1;
    }
    
    pinMode(pin, mode);
    
    switch_info_t *info = &this->switches[this->configuredSwitchesNum];
    
    info->pin = pin;
    info->func = func;
    info->state = digitalRead(pin);
    info->transientState = info->state;
    info->lastChangeTime = 0;
    info->settings = settings;
    
    this->configuredSwitchesNum++;
    
    return this->configuredSwitchesNum;
    
}

/*
    Checks status of inputs and call any callbacks
    when neeed. Must be called frequenctly, typically
    from loop() function in order to properly work.
*/
bool Debounce::update() {
    
    uint8_t measuredState;
    uint8_t reportedState;
    switch_info_t *info;
    unsigned long currentTime = millis();
    unsigned long deltaTime;
        
    for (uint8_t i = 0; i < this->configuredSwitchesNum; i++) {

        info = &this->switches[i];
        measuredState = digitalRead(info->pin);
        
        if (!(info->settings & DEBOUNCE_SETTINGS_FAST_CALLBACK) && measuredState != info->transientState) {
            info->transientState = measuredState;
            info->lastChangeTime = currentTime;
        }
        
        if (measuredState != info->state) {
            
            deltaTime = currentTime - info->lastChangeTime;
            
            if (deltaTime > this->bounceDelay) {
                info->state = measuredState;
                
                if (info->settings & DEBOUNCE_SETTINGS_INVERT) {
                    reportedState = !measuredState;
                } else {
                    reportedState = measuredState;
                }
                
                if (reportedState) {
                    //rising edge
                    if (! (info->settings & DEBOUNCE_SETTING_SKIP_RISING_EDGE) ) {
                        info->func(reportedState, info->pin);
                    }
                } else {
                    //falling edge
                    if (! (info->settings & DEBOUNCE_SETTING_SKIP_FALLING_EDGE) ) {
                        info->func(reportedState, info->pin);
                    }
                }
                
            }
            
        }
        
        if (measuredState != info->transientState) {
            info->transientState = measuredState;
            info->lastChangeTime = currentTime;
        }
        
    }
    return 1;
}