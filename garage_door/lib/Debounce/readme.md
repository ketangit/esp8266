## Readme

Debounce is a simple Arduino compatible library for debouncing of inputs (typically for mechanical switches).

#### Install
Download and rename the folder Debounce and put it in your arduinosketchfolder/libraries/ folder.

####Usage:

```c
#include <Debounce.h>

Debounce debounce = Debounce();

void setup() {
    int pin = A0;
    debounce.addInput(pin, INPUT_PULLUP, callback);
    //use INPUT_PULLUP if you want internal pullup
    //or INPUT if you don't want it
    Serial.begin(115200);
}

void loop() {
    debounce.update();
}

void callback(bool state, uint8_t pin) {
    //do something when input pin is toggled
    Serial.println(state);
}

```



Advanced Usage:

```c
debounce.addInput(pin, INPUT_PULLUP, callback,
    DEBOUNCE_SETTING_SKIP_FALLING_EDGE |
    DEBOUNCE_SETTINGS_INVERT |
    DEBOUNCE_SETTINGS_FAST_CALLBACK );
```

More documentation found in source files.