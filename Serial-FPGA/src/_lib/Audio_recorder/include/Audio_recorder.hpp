#pragma once

#define	INITGUID //<== эта дрянь потратила 3 часа моей жизни !!!!НЕ ТРОГАТЬ ИНАЧЕ ВСЕ СДОХНЕТ!!!!
#define	DIRECTSOUND_VERSION		0x0800

#include <dsound.h>
#include <thread>
#include <chrono>
#include "AtomicQueue.hpp"

class Audio_recorder
{
public:
    Audio_recorder();   //не используется
    ~Audio_recorder();  //не используется

    void init(AtomicQueue< char > *sndBuffer);//Вызывать перед start(), НАСТРОЙКИ ВНУТРИ метода закладываются на этапе компиляции
    //void close(); //Остановка записи (если она ведется) + заверешение работы с api ---- !не требуется!

    void start(); //Запуск записи семплов в атомик очередь
    void stop(); //Остановка записи 

private:
    void run(); //Метод который работает в отдельном потоке и записывает семплы в атомик очередь 
    void isInited();
    bool inited = false; //был ли вызван метод init()
 
    LPDIRECTSOUNDCAPTURE8			g_pDSCapture; //указатель на интерфейс IDirectSoundCapture https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418154(v=vs.85)
    LPDIRECTSOUNDCAPTUREBUFFER8		g_pDSCaptureBuffer;
    WAVEFORMATEX	wfx; //структура определяет формат звуковых данных https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex
    LPDIRECTSOUNDCAPTUREBUFFER	pDSCB; // буфер захвата 
    DSCBUFFERDESC	dsbd; //Структура DSCBUFFERDESC описывает буфер захвата.  https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee416823(v=vs.85)
                         //Он используется методом IDirectSoundCapture8::CreateCaptureBuffer и функцией DirectSoundFullDuplexCreate8 .
    float bufferDurationInTime;
    AtomicQueue< char > *sndBuffer;  //это не буфер, а переменная хранящая указатель на буфер

    bool cmd_stop_thread = true;
    bool thread_stopped = true;
};

