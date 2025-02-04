#include "Mode_switcher.hpp"

Mode_switcher::Mode_switcher()
{
}

Mode_switcher::~Mode_switcher()
{
}

bool Mode_switcher::get_mode()
{
    return mode;
}

void Mode_switcher::start()
{   
    fflush(stdin); //чистим поток ввода (чтобы например, если до метода start мы запрашивали ввод цифры, у нас не произошло нажатия enter после start)
    while (getchar() != '\n'); //костыль для отчистки буфера ввода, если fflush не сработал (у меня он ни разу не работал)
    if (run_flag == 1) { 
        printf("Mode_switcher is already working!\n");
    }
    else {//Если не запущен, то запускаем
        run_flag = 1;
        std::thread body_thread(&Mode_switcher::run, this);
        body_thread.detach();
    }
}

void Mode_switcher::stop(){
    run_flag = 0;
}

void Mode_switcher::run()
{
    while(run_flag)
    {
        getchar();
        mode = !mode;
    }
}
