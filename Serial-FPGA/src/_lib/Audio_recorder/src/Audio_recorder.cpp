#include "Audio_recorder.hpp"
#include<iostream>
#include "file_open_close.h"

using namespace std::chrono_literals;

Audio_recorder::Audio_recorder(){}
Audio_recorder::~Audio_recorder(){}

void Audio_recorder::init(AtomicQueue< char > *b){
	if (inited) {
		fprintf(stderr, "recorder: Error! reinitialization! Exit...\n");
		exit(1);
	}
	sndBuffer = b;      //это не буфер, а переменная хранящая указатель на буфер
	DirectSoundCaptureCreate8(NULL, &g_pDSCapture, NULL); //создает и инициализирует объект, поддерживающий интерфейс IDirectSoundCapture8
	//параметры
	//lpcGUID	//	Адрес GUID, который идентифицирует устройство звукозаписи.Значение этого параметра должно быть одним из идентификаторов GUID, возвращаемых DirectSoundCaptureEnumerate, или NULL для устройства по умолчанию, или одним из следующих значений.
	//	lplpDSC	//	Адрес переменной для получения указателя интерфейса IDirectSoundCapture8 .
	//	pUnkOuter	//	Адрес интерфейса IUnknown управляющего объекта для агрегации COM.Должно быть NULL, так как агрегация не поддерживается.
	
	//НАСТРОЕЧКИ ЗАХВАТА закладываемые на этапе компиляции//////////////////////////////////////////////
	wfx.wFormatTag = WAVE_FORMAT_PCM; //Данные PCM (импульсно-кодовая модуляция) в целочисленном формате со знаком, little endian
	wfx.nChannels = 1; //Кол-во каналов
	wfx.nSamplesPerSec = 8000; //Частота дискретизации
	wfx.wBitsPerSample = 8; //Кол-во бит на семпл
	wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8); //Выравнивание, не трогать, расчитывается автоматически
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; //не трогать
	wfx.cbSize = 0; //Размер в байтах дополнительной информации о формате, добавленной в конец структуры WAVEFORMATEX 
	bufferDurationInTime = 0.2; //Сколько секунд записи помещает буфер (задежка передачи в атомик  >= половине размера буфера во времени)
	////////////////////////////////////////////////////////////////////////////////////////////////////

	ZeroMemory(&dsbd, sizeof(DSCBUFFERDESC)); //memset макрос, "на всякий зануляем"
	dsbd.dwSize = sizeof(DSCBUFFERDESC); //Размер структуры в байтах. Этот член должен быть инициализирован перед использованием структуры.
	
	dsbd.dwBufferBytes = bufferDurationInTime * wfx.nAvgBytesPerSec; //Размер создаваемого буфера захвата в байтах. 
	dsbd.lpwfxFormat = &wfx; //Указатель на структуру WAVEFORMATEX , содержащую формат для захвата данных.

	
	g_pDSCapture->CreateCaptureBuffer(&dsbd, &pDSCB, NULL); //создает буфер захвата
	//Параметры
	//	lpDSCBufferDesc
	//	Указатель на структуру DSCBUFFERDESC, содержащую значения для создаваемого буфера захвата.
	//	lplpDirectSoundCaptureBuffer
	//	Адрес указателя интерфейса IDirectSoundCaptureBuffer в случае успеха.
	//	pUnkOuter
	//	Управление IUnknown агрегата.Его значение должно быть NULL.
	pDSCB->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&g_pDSCaptureBuffer); //https://docs.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-queryinterface(refiid_void)
	pDSCB->Release(); //https://docs.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-release
	inited = true; //Инициализация завершена
}

void Audio_recorder::start(){
	isInited();
	if (thread_stopped == false) {
		fprintf(stderr, "recorder: ERROR! the stream is already running! Exit...\n");
		exit(1);
	}
	cmd_stop_thread = false;
	g_pDSCaptureBuffer->Start(DSCBSTART_LOOPING); //начинает захват данных в буфер циклично https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418181(v=vs.85)
	//запускаем поток выполняющий функцию run по указателю, этого экземпляра класса this
	std::thread body_thread(&Audio_recorder::run, this);
	body_thread.detach();//отключились от потока
	thread_stopped = false;
	std::cout << "recorder: start()\n";
}

void Audio_recorder::run()
{
	char* pAudio1 = NULL; //Адрес переменной, которая получает указатель на первую заблокированную часть буфера.
	DWORD AudioBytes1 = NULL; //Адрес переменной, которая получает количество байтов в блоке в ppvAudioPtr1

	DWORD readPosition=0; //Смещение с которого мы будем считывать байты (от начала буфера)
	DWORD desiredReadBytes = dsbd.dwBufferBytes / 2; //Сколько байт хотим считать (половина буфера)

	DWORD readСursor=0; //Курсор, ПЕРЕД КОТОРЫМ МОЖНО БЕЗОПАСНО ЧИТАТЬ БАЙТЫ
	DWORD captureСursor=0; // курсор захвата (У НАС НЕ ИСПОЛЬЗУЕТСЯ ВООБЩЕ)
	
	bool flag = 0; //КАКУЮ ПОЛОВИНУ БУФЕРА БУДЕМ ЧИТАТЬ (0 - первая, 1 - вторая)

	//FILE* debug_file;
	//file_open_write(debug_file,"recFile");
	//char sample;

	//Сначала идет курсора захвата, потом курсор чтения, мы можем безопасно брать семплы до адреса курсора чтения
    while (!cmd_stop_thread) //пока нет команды стоп
    {
		g_pDSCaptureBuffer->GetCurrentPosition(&captureСursor, &readСursor);
		if (flag == 0) {
			if (readСursor >= dsbd.dwBufferBytes/2) {
				readPosition = 0;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds((long long)(bufferDurationInTime/4 * 1000)));
				continue;
			} 
		}
		else {
			if (readСursor < dsbd.dwBufferBytes / 2) {
				readPosition = dsbd.dwBufferBytes / 2;

			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds((long long)(bufferDurationInTime/4 * 1000)));
				continue;
			}
		}

		g_pDSCaptureBuffer->Lock(readPosition, desiredReadBytes,
			(LPVOID*)&pAudio1, &AudioBytes1,
			NULL, NULL,
			0); https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418179(v=vs.85)
				//блокирует часть буфера.Блокировка буфера возвращает указатели на буфер, 
				//позволяя приложению считывать или записывать аудиоданные в память.
				//также считывает кусок буффера с p, размером s
	
		for (unsigned int i = 0; i < AudioBytes1; i++)
		{
			//sample = pAudio1[i];
			//fwrite(&sample, 1, 1, debug_file);
			sndBuffer->Push(pAudio1[i]); //пишем байты из внутреннего буфера в конец очереди по одному
			//sndBuffer->Push(pAudio1[i + 1]); //пишем байты из внутреннего буфера в конец очереди по одному
		}

		g_pDSCaptureBuffer->Unlock(pAudio1, AudioBytes1, NULL, NULL); //https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418185(v=vs.85)
		std::this_thread::sleep_for(std::chrono::milliseconds((long long)(bufferDurationInTime / 4 * 1000)));
		flag = !flag;
    }

    //пришла команда стоп, вышли из цикла
    fprintf(stderr, "recorder: cmd_stop_thread: %d\n", cmd_stop_thread);
    //если цикл завершен, даем обратную связь
    thread_stopped = true;
}

void Audio_recorder::stop(){
	isInited();
	if (thread_stopped == true) {
		fprintf(stderr, "recorder: ERROR! repeat stop()! Exit...\n");
		exit(0);
	}
	cmd_stop_thread = true; //даем команду  остановить цикл
	g_pDSCaptureBuffer->Stop(); //останавливает буфер, чтобы он больше не собирал данные.Если буфер не захватывается, метод не действует.
	
	size_t temp_count = 0;
	while (thread_stopped == false)	{
		// контролируем останов, с засыпанием между котролями на 10 мс
		std::this_thread::sleep_for(5ms);
		temp_count++;
		if (temp_count == 200){
			//если не дождались, пишем
			fprintf(stderr, "recorder: ERROR: thread_stopped: %d\n", thread_stopped);
			// и корректно выходим
			exit(1);
		}
	}
}

//void Audio_recorder::close(){
//	isInited();
//	if (thread_stopped == false) stop();
//
//
//	inited = false;
//
//	// Утечек памяти не происходит, какое-то особенное закрытие - не требуется
//}

void Audio_recorder::isInited() {
	if (!inited) {
		fprintf(stderr, "Error! Audio_recorder is not initialized! Exit...\n");
		exit(1);
	}
}