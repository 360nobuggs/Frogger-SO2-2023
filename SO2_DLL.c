#include "SO2_DLL.h"

#define MUTEX_VERIFY_SERVER_OPEN _T("MutexServidorExiste")

BOOL isUniqueInstance(HANDLE* semaphoreStart) {
	semaphoreStart = CreateSemaphore(NULL, 0, 1, TEXT("START_SERVIDOR_SEMAPHORE"));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(semaphoreStart);
		return FALSE;
	}
	return TRUE;
}
BOOL inicializa_mem_s(ThreadDadosMemPartilhada* tDadosMemPartilhada, TDados dados, HANDLE* hjogo, HANDLE* hFileMap)
{
	HKEY chave = NULL;
	
	DWORD res;
	DWORD dwPairValue = 0;
	DWORD dwBufferSize = sizeof(DWORD);
	HANDLE semaphoreStart;
	CRITICAL_SECTION csBuffer;
	HANDLE hEventsDados;
	HANDLE hTerminate;
	//evento de mudanca de dados

	hEventsDados = CreateEvent(NULL, TRUE, FALSE, TEXT("EVENTO_DADOS"));
	if (hEventsDados == NULL)
	{
		_tprintf(TEXT("\nErro a criar evento dados\n "));
		return 1;
	}
	hTerminate = CreateEvent(NULL, TRUE, FALSE, TEXT("TERMINATE"));
	if (hTerminate == NULL)
	{
		_tprintf(TEXT("\nErro a criar evento terminate \n "));
		return 1;
	}
	//verifica exitencia de valores no registry,se nao existem cria


	(void)InitializeCriticalSectionAndSpinCount(&csBuffer, 500);
	tDadosMemPartilhada->cs = &csBuffer;
	tDadosMemPartilhada->hSemEscrita = CreateSemaphore(NULL, 2, 2, TEXT("SO2_SEMAFORO_ESCRITA"));
	tDadosMemPartilhada->hSemLeitura = CreateSemaphore(NULL, 0, 2, TEXT("SO2_SEMAFORO_LEITURA"));
	tDadosMemPartilhada->hMutex = CreateMutex(NULL, FALSE, TEXT("SO2_MUTEX_CONSUMIDOR"));
	if (tDadosMemPartilhada->hSemEscrita == NULL || tDadosMemPartilhada->hSemLeitura == NULL || tDadosMemPartilhada->hMutex == NULL) {
		_tprintf(TEXT("Erro a criar os semaforos"));
		return -1;
	}
	hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(BufferCircular),
		TEXT("SO2_MEM_PARTILHADA")
	);
	if (hFileMap == NULL) {
		_tprintf(TEXT("Erro no CreateFileMapping"));
		return -1;
	}
	tDadosMemPartilhada->memPar = (BufferCircular*)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (tDadosMemPartilhada->memPar == NULL) {
		_tprintf(TEXT("Erro no MapViewOfFile"));
		return -1;
	}
	hjogo = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(Jogo),
		TEXT("SO2_MEM_JOGO")
	);
	if (hjogo == NULL) {
		_tprintf(TEXT("Erro no CreateFileMapping"));
		return -1;
	}
	tDadosMemPartilhada->jogo = (Jogo*)MapViewOfFile(hjogo, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (tDadosMemPartilhada->jogo == NULL) {
		_tprintf(TEXT("[!] Ocorreu um erro ao mapear uma vista do ficheiro"));
		CloseHandle(hFileMap);
		return -1;
	}

	tDadosMemPartilhada->memPar->nConsumidores = 0;
	tDadosMemPartilhada->memPar->nProdutores = 0;
	tDadosMemPartilhada->memPar->posE = 0;
	tDadosMemPartilhada->memPar->posL = 0;
	tDadosMemPartilhada->terminar = 0;
	tDadosMemPartilhada->jogo->dim_max = dados.dim_max;
	tDadosMemPartilhada->hEvent = hEventsDados;
	tDadosMemPartilhada->terminate_event = hTerminate;
	tDadosMemPartilhada->jogo->v_inicial = dados.velocidade;

	WaitForSingleObject(tDadosMemPartilhada->hMutex, INFINITE);
	tDadosMemPartilhada->memPar->nConsumidores++;
	ReleaseMutex(tDadosMemPartilhada->hMutex);
}
BOOL inicializa_mem(ThreadDadosMemPartilhada* tDadosMemPartilhada)
{
	HANDLE hFileMap, hFileJogo;
	TDados threadDados;
	
	
	

	tDadosMemPartilhada->hSemEscrita = CreateSemaphore(NULL, 2, 2, TEXT("SO2_SEMAFORO_ESCRITA"));
	tDadosMemPartilhada->hSemLeitura = CreateSemaphore(NULL, 0, 2, TEXT("SO2_SEMAFORO_LEITURA"));

	tDadosMemPartilhada->hMutex = CreateMutex(NULL, FALSE, TEXT("SO2_MUTEX_PRODUTOR"));
	if (tDadosMemPartilhada->hSemEscrita == NULL || tDadosMemPartilhada->hSemLeitura == NULL || tDadosMemPartilhada->hMutex == NULL) {
		_tprintf(TEXT("Erro no semaforo ou no mutex"));
		tDadosMemPartilhada->id = -1;
	}

	hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("SO2_MEM_PARTILHADA"));
	if (hFileMap == NULL) tDadosMemPartilhada->id = -1;
	tDadosMemPartilhada->memPar = (BufferCircular*)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (tDadosMemPartilhada->memPar == NULL)
	{
		_tprintf(TEXT("erro no mapviewoffile"));
		tDadosMemPartilhada->id = -1;
	}


	//filemaping jogo
	hFileJogo = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("SO2_MEM_JOGO"));
	if (hFileJogo == NULL) tDadosMemPartilhada->id = -1;

	tDadosMemPartilhada->jogo = (Jogo*)MapViewOfFile(hFileJogo, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (tDadosMemPartilhada->memPar == NULL)
	{
		_tprintf(TEXT("erro no mapviewoffile"));
		tDadosMemPartilhada->id = -1;
	}

	WaitForSingleObject(tDadosMemPartilhada->hMutex, INFINITE);
	tDadosMemPartilhada->memPar->nProdutores++;
	ReleaseMutex(tDadosMemPartilhada->hMutex);

}