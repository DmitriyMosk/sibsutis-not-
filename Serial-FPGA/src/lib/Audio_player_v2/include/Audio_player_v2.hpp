#pragma once

#define    INITGUID //<== эта дрянь потратила 3 часа моей жизни !!!!НЕ ТРОГАТЬ ИНАЧЕ ВСЕ СДОХНЕТ!!!!
#define    DIRECTSOUND_VERSION        0x0800

#include <cstdio>

#include <dsound.h>
#include <thread>
#include "AtomicQueue.hpp"

template <class T>
class Audio_player
{
public:
    Audio_player(AtomicQueue< T >* sndBuffer, const int bitsPerSample, const int sampleRate);
    ~Audio_player(); // не используется
    
    void start();
    void stop();
    
    int getSampleRate();    // получить частоту дискретизации
    int getBitsPerSample(); // получить сколько бит содержится в одном семпле

    //void close(); //TODO:void close() не работает, не готово

private:
    void run();
    
    DSBUFFERDESC m_dsbd;
    WAVEFORMATEX m_wfx;
    LPDIRECTSOUND m_lpDS;
    LPDIRECTSOUNDBUFFER m_lpDSB;

    float m_bufferDurationInTime;

    AtomicQueue< T > * m_sndBuffer;
    bool m_cmd_stop_thread = false;
    bool m_thread_stopped = false;
};

//заполнение шаблона
typedef unsigned char byte;
template class Audio_player<char>;  //явно создаем экземпляр шаблона класса
template class Audio_player<short>; //явно создаем экземпляр шаблона класса
template class Audio_player<byte>;  //явно создаем экземпляр шаблона класса
template class Audio_player<int>;   //явно создаем экземпляр шаблона класса