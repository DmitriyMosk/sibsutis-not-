#pragma once

#include <stdio.h> // Для C

#include <thread>

class Mode_switcher
{
public:
    Mode_switcher();
    ~Mode_switcher();

    bool get_mode();
    void start();
    void stop(); //TOOD: доделать выключение
private:

    void run();

    bool run_flag = false;
    bool mode = false;
};
