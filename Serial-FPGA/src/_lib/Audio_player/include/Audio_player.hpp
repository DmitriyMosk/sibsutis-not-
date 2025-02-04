#pragma once
//#include "stdafx.h"

#define    INITGUID //<== эта дрянь потратила 3 часа моей жизни !!!!НЕ ТРОГАТЬ ИНАЧЕ ВСЕ СДОХНЕТ!!!!
#define    DIRECTSOUND_VERSION        0x0800

#include <dsound.h>

#include <thread>

#include "AtomicQueue.hpp"

class Audio_player
{
public:
    Audio_player();
    ~Audio_player();
    
    void start(AtomicQueue< char > *sndBuffer);
    void stop();
    void close();

private:
    void run();
    
    DSBUFFERDESC dsbd;
    WAVEFORMATEX wfx;
    LPDIRECTSOUND m_lpDS;
    LPDIRECTSOUNDBUFFER m_lpDSB;

    float bufferDurationInTime = 0;

    AtomicQueue< char > *sndBuffer;
    bool cmd_stop_thread = false;
    bool thread_stopped = false;
};