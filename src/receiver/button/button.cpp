#include "button.h"


button* button::instance = nullptr;

void button::begin(){
    bindingButton.attachDoubleClick(doubleClick);
    bindingButton.attachLongPressStop(longPress);
    pinMode(_pin,INPUT_PULLUP);
}

void button::doubleClick(){
    if (instance)
    {
        instance->onDoubleClick();
    }
}
void button::onDoubleClick(){
    mode = true;
}

void button::longPress(){
    if (instance)
    {
        instance->onLongPress();
    }
}

void button::onLongPress(){
    removeData = true;
}

void button::tick(){
    bindingButton.tick();
}

