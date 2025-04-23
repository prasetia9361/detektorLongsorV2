#pragma once

#include "OneButton.h"
// #include "config.h"

class button
{
private:
    bool mode = false;
    bool removeData = false;
    int _pin;
    OneButton bindingButton;
    static button* instance;
    static void doubleClick();
    static void longPress();
public:
    button(int pin):_pin(pin),bindingButton(_pin, true){
        // mode = false;
        // removeData = false;
        instance = this;
    }
    void begin();
    void onDoubleClick();
    void onLongPress();
    bool getMode(){return mode;}
    bool setMode(bool value){
        mode = value;
        return mode;
    }

    bool getRemove(){return removeData;}
    bool setRemove(bool value){
        removeData = value;
        return removeData;
    }
    
    void tick();
};