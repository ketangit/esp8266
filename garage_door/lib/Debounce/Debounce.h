/*
 Copyright (c) 2013, Arne Bech
 All rights reserved.
 See license file for more info.
*/

#ifndef ____Debounce__
#define ____Debounce__

#ifndef DEBOUNCE_MAX_CAPACITY
/* The default capacity e.g. how many inputs we can monitor at the most. */
#define DEBOUNCE_MAX_CAPACITY 5
#endif

#ifndef DEBOUNCE_DEFAULT_DELAY
/* Default bounce delay in ms */
#define DEBOUNCE_DEFAULT_DELAY 10
#endif


/* 
 Debounce settings - flag words
 
 These are can be used in the addInput settings param and can
 be combined e.g.
 debounce.addInput(....., DEBOUNCE_SETTING_SKIP_RISING_EDGE | DEBOUNCE_SETTINGS_INVERT);
 
*/
/* normal setting, is used when calling addInput with no setting parameter. e.g. 
 no need to use this directly */
#define DEBOUNCE_SETTING_NORMAL 0x00

/* don't not fire callback on rising edge e.g. no callbacks sent when the
 input goes from LOW to HIGH */
#define DEBOUNCE_SETTING_SKIP_RISING_EDGE 0x01

/* don't fire callback on falling edge e.g. no callbacks sent when the 
 input goes from HIGH to LOW */
#define DEBOUNCE_SETTING_SKIP_FALLING_EDGE 0x02

/* invert result state, e.g. useful when using pull-up resistors since
 the button off-state is will be 1 by default then */
#define DEBOUNCE_SETTINGS_INVERT 0x04

/* fire callback on first edge. this makes it really fast, but will
 also fire on false positives (if any) such as noise on the line */
#define DEBOUNCE_SETTINGS_FAST_CALLBACK 0x08


//#include <iostream>
#include "Arduino.h"

class Debounce {
    
public:
    Debounce();
    void setBounceDelay(uint16_t);
    uint8_t addInput(uint8_t pin, uint8_t mode, void(*func)(bool, uint8_t));
    uint8_t addInput(uint8_t pin, uint8_t mode, void(*func)(bool, uint8_t), uint8_t settings);
    bool update();
    
private:
    typedef struct config_struct {
        uint8_t pin;
        void(*func)(bool, uint8_t);
        uint8_t state;
        uint8_t transientState;
        unsigned long lastChangeTime;
        uint8_t settings;
        
    } switch_info_t;
    switch_info_t switches[DEBOUNCE_MAX_CAPACITY];
    uint8_t configuredSwitchesNum;
    uint16_t bounceDelay;
    
};

#endif