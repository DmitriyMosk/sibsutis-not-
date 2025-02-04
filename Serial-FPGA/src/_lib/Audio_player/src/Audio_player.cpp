#include "Audio_player.hpp"
#include "file_open_close.h"

Audio_player::Audio_player(){
	cmd_stop_thread = false;  //переменная завершения цикла
	thread_stopped = false;   //подтверждение завершения цикла

	if (FAILED(DirectSoundCreate(NULL, &m_lpDS, NULL))) {
		printf("DirectSoundCreate ERROR!\n");
		exit(-99);
	}
	//Set Cooperative Level
	HWND hWnd = GetForegroundWindow();
	if (hWnd == NULL)
	{
		hWnd = GetDesktopWindow();
	}

	if (FAILED(m_lpDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY))) {
		printf("SetCooperativeLevel ERROR!\n");
		return;
	}

	//Create Primary Buffer 
	
	ZeroMemory(&dsbd, sizeof(dsbd));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = NULL;

	LPDIRECTSOUNDBUFFER lpDSB = NULL;
	if (FAILED(m_lpDS->CreateSoundBuffer(&dsbd, &lpDSB, NULL))) {
		printf("CreateSoundBuffer Primary Buffer ERROR!\n");
		return;
	}

	memset(&wfx, 0, sizeof(WAVEFORMATEX));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = 8000; //иногда стоить сделать чуть быстрее записи
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample/8);
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * (wfx.wBitsPerSample / 8);
	wfx.cbSize = 0;

	//Set Primary Buffer Format
	HRESULT hr = 0;
	if (FAILED(lpDSB->SetFormat(&wfx))) {
		printf("Set Primary Buffer Format ERROR!\n");
		return;
	}

	//Create Second Sound Buffer
	bufferDurationInTime = 0.2; // Оптим 0.2 длительность буфера звуковоспроизведения в секундах (задержка звука = половине длит буф)
	dsbd.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS;
	dsbd.dwBufferBytes = bufferDurationInTime * wfx.nAvgBytesPerSec; //x Seconds Buffer
	dsbd.lpwfxFormat = &wfx;

	if (FAILED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSB, NULL))) {
		printf("Create Second Sound Buffer Failed!");
		return;
	}
	printf("Audio_player: init success!\n");
}


Audio_player::~Audio_player()
{
  
}

void Audio_player::start(AtomicQueue< char > *b){
    sndBuffer = b;                                      //это не буфер, а переменная хранящая указатель на буфер
	cmd_stop_thread = false;
	std::thread body_thread(&Audio_player::run, this);  //запускаем поток выполняющий функцию run по указателю, этого экземпляра класса this
    body_thread.detach();                               //отключились от потока
}



void Audio_player::run()
{
	bool turn_on = false; //принудительно выключили плейер

	DWORD writePosition = 0; //Смещение с которого мы будем записывать байты (от начала буфера)
	//DWORD desiredWriteSamples = dsbd.dwBufferBytes / 4; //Половина буфера в семлах //В ЭТОМ И БЫЛ КОСЯК

	DWORD writeCursor = 0; //за ним можно безопасно писать данные (не используем)
	DWORD playCursor = 0; //Курсор на семлах которые сейчас воспроизводятся

	char* pAudio1 = NULL; //Адрес переменной, которая получает указатель на заблокированную часть буфера.
	DWORD dwBytesAudio1 = 0; //Адрес переменной, которая получает количество байтов в блоке в ppvAudioPtr1
	
	HRESULT hr = 0; //Хранит код ошибки (для дебага в отладчике)

	bool flag = 0; //КАКУЮ ПОЛОВИНУ БУФЕРА БУДЕМ ПИСАТЬ (0 - вторая, 1 - первая)

	//char sample = 0; //один звуковой отчет
	char sample_byte;

	//FILE* recfile2;
	//file_open_write(recfile2, "recFile2");
	//while (1) {
	//	while (sndBuffer->Pull(sample_byte)) {
	//		fwrite(&sample_byte, 1, 1, recfile2);
	//	}
	//}

	printf("Audio_player: is start!\n");
	while(!cmd_stop_thread){
		if (sndBuffer->Size() < dsbd.dwBufferBytes / 2) {//кол-во байт в очереди меньше половины буфера //И тут был косяк
			std::this_thread::sleep_for(std::chrono::milliseconds(50)); //тогда спим 50мс, ждем когда в очередь набьется достаточно семплов
			//printf("Audio_player: sndBuff is empty!\n");
			m_lpDSB->Stop();
		}
		else m_lpDSB->Play(0, 0, DSBPLAY_LOOPING); //начинает цикличное воспроизведение данных из буфера

		m_lpDSB->GetCurrentPosition(&playCursor, &writeCursor);
		if (flag == 0) {
			if (playCursor <= dsbd.dwBufferBytes / 2) {
				writePosition = dsbd.dwBufferBytes / 2;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds((long long)(bufferDurationInTime / 4 * 1000)));
				continue;
			}
		}
		else {
			if (playCursor > dsbd.dwBufferBytes / 2) {
				writePosition = 0;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds((long long)(bufferDurationInTime / 4 * 1000)));
				continue;
			}
		}

		hr = m_lpDSB->Lock(writePosition, dsbd.dwBufferBytes / 2, (LPVOID*)&pAudio1, &dwBytesAudio1, 0, 0, 0);

		for (int i = 0; i < dwBytesAudio1; i++) {
			sndBuffer->Pull(sample_byte);
			//fwrite(&sample_byte, 1, 1, recfile2);
			pAudio1[i] = sample_byte;
		}

		hr = m_lpDSB->Unlock(pAudio1, dwBytesAudio1, NULL, NULL);
		flag = !flag;
		std::this_thread::sleep_for(std::chrono::milliseconds((long long)(bufferDurationInTime / 4 * 1000)));
		//printf("debug stop audio player!\n");
		//break;
		}
    fprintf(stderr, "Audio_player: cmd_stop_thread: %d\n", cmd_stop_thread);  //пришла команда стоп, вышли из цикла
    thread_stopped = true;  //если цикл завершен, даем обратную связь
}

void Audio_player::stop()
{
    cmd_stop_thread = true;           //даем команду остановить цикл

	m_lpDSB->Stop(); //останавливает воспроизведение

    size_t temp_count = 0;
    while (thread_stopped == false)   //контролируем останов
    {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        temp_count++;
        if (temp_count == 200)
        {
            fprintf(stderr, "player: ERROR: thread_stopped: %d\n", thread_stopped);
            exit(1);
        }
    }
}

void Audio_player::close(){
	stop();
	//TODO: Корректно завершить работу с API (а как?)
}