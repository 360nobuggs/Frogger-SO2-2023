#include "servidor.h"

DWORD WINAPI threadConsoleInterface(LPVOID lpParam) {
	TCHAR cmd[MAX_STR_SIZE];
	TCHAR* cmdToken = NULL;
	TCHAR* cmdNextToken = NULL;
	BOOL jogoCorre = TRUE;
	const TCHAR* seps = TEXT(" \n");
	ThreadDadosMemPartilhada* threadDados = (ThreadDadosMemPartilhada*)lpParam;
	_tprintf(TEXT("\nMenu:\nsair-termina jogo\nparar-suspende jogo\ncontinuar-continua jogo\nreiniciar-reinicia jogo\n"));
	while (TRUE) {
		_ftprintf(stdout, TEXT(">"));
		fflush(stdin);
		_fgetts(cmd, MAX_STR_SIZE, stdin);
		cmdToken = _tcstok_s(cmd, seps, &cmdNextToken);
		if (cmdToken != NULL)
		{
			if (_tcscmp(cmdToken, TEXT("sair")) == 0)
			{
				//EnterCriticalSection(threadDados->cs);
				threadDados->terminar = 1;
				SetEvent(threadDados->terminate_event);
				//ExitThread(0);
				//LeaveCriticalSection(threadDados->cs);
				break;
			}
			else if (_tcscmp(cmdToken, TEXT("parar")) == 0) {
				if (jogoCorre == FALSE) _tprintf(TEXT("O jogo j� est� em pausa\n"));
				else {
					//SuspendThread(threadDados->Thread_jogo);
					_tprintf(TEXT("O jogo parou\n"));
					jogoCorre = FALSE;
				}
				continue;
			}
			else if (_tcscmp(cmdToken, TEXT("continuar")) == 0) {
				if (jogoCorre == TRUE) _tprintf(TEXT("O jogo j� est� a correr\n"));
				else
				{
					//ResumeThread(threadDados->Thread_jogo);
					_tprintf(TEXT("O jogo continua\n"));
					jogoCorre = TRUE;
				}
				continue;
			}
			else if (_tcscmp(cmdToken, TEXT("reiniciar")) == 0) {

				EnterCriticalSection(threadDados->cs);
				inicia_jogo(threadDados->jogo, threadDados->jogo->dim_max);
				_tprintf(TEXT("O jogo reiniciou\n"));
				LeaveCriticalSection(threadDados->cs);

				continue;
			}
			else
			{
				_tprintf(TEXT("Comando nao reconhecido\n"));
			}
		}

	}
	return 0;
}

DWORD WINAPI threadTerminate(LPVOID lpParam)
{
	ThreadDadosMemPartilhada* tDados = (ThreadDadosMemPartilhada*)lpParam;
	if (WaitForSingleObject(tDados->terminate_event, INFINITE) == WAIT_OBJECT_0)
	{
		EnterCriticalSection(tDados->cs);
		for (int i = 0; i < 4; i++)
		{
			TerminateThread(tDados->Threads[i], 10);
		}
		_tprintf(TEXT("Servidor terminou por order sair.\n"));
		LeaveCriticalSection(tDados->cs);
		//ExitThread(0);
	}
	
}
DWORD WINAPI threadJogo(LPVOID lpParam)
{
	ThreadDadosMemPartilhada* tDados = (ThreadDadosMemPartilhada*)lpParam;
	while (tDados->terminar != 1)
	{
		Sleep(15000 - (tDados->jogo->v_inicial) * 100);
		EnterCriticalSection(tDados->cs);
		//_tprintf(TEXT("\n valor de : %d\n"), tDados->jogo->v_inicial);
		int a = tDados->jogo->dim_max;
		for (int i = 1; i < tDados->jogo->dim_max; i++)
		{
			move_fila(tDados->jogo, i);
		}
		mostra_mapa(tDados->jogo, tDados->jogo->dim_max);
		SetEvent(tDados->hEvent);
		LeaveCriticalSection(tDados->cs);
	}
	return 0;
}

DWORD WINAPI threadConsumidor(LPVOID lpParam) {
	ThreadDadosMemPartilhada* tDados = (ThreadDadosMemPartilhada*)lpParam;
	CelulaBuffer cel;
	TCHAR* comando_parar = TEXT("pararLinha");
	while (tDados->terminar != 1)
	{

		WaitForSingleObject(tDados->hSemLeitura, INFINITE);

		WaitForSingleObject(tDados->hMutex, INFINITE);

		CopyMemory(&cel, &tDados->memPar->buffer[tDados->memPar->posL], sizeof(CelulaBuffer));

		ReleaseMutex(tDados->hMutex);

		ReleaseSemaphore(tDados->hSemEscrita, 1, NULL);

		_tprintf(TEXT("Observador: %s %d %d\n"), cel.tcMemPartilhada, cel.parametros[0], cel.parametros[1]);

		EnterCriticalSection(tDados->cs);
		_tprintf(TEXT("Observador: %s\n"), cel.tcMemPartilhada);
		if (_tcsicmp(cel.tcMemPartilhada, comando_parar) == 0) {
			SuspendThread(threadJogo);
			Sleep(1000 * cel.parametros[0]);
			ResumeThread(threadJogo);
			_tprintf(TEXT("Carros parados por %d segundos. \n"),cel.parametros[0]);
		}
		else if (_tcsicmp(cel.tcMemPartilhada, TEXT("inserirBloco")) == 0) {
			if (cel.parametros[0] > 0 && cel.parametros[0] < tDados->jogo->dim_max && cel.parametros[1]>0 && cel.parametros[1] < NUMERO_COL)
			{
				insere_barreira(tDados->jogo, cel.parametros[0], cel.parametros[1]);
				_tprintf(TEXT("Bloco inserido \n"));
				SetEvent(tDados->hEvent);
			}
			else
			{
				_tprintf(TEXT("Valores recebidos invalidos para adiconar bloco. \n"));
			}
		}
		else if (_tcsicmp(cel.tcMemPartilhada, TEXT("inverterLinha")) == 0) {
			if (cel.parametros[0] > 0 || cel.parametros[0] < tDados->jogo->dim_max)
			{
				inverte_direcao(tDados->jogo, cel.parametros[0]);
				_tprintf(TEXT("Linha invertida \n"));
				SetEvent(tDados->hEvent);
			}
			else
			{
				_tprintf(TEXT("Valor recebido invalido para inverter. \n"));
			}
			
		}
		else if (_tcsicmp(cel.tcMemPartilhada, TEXT("sair")) == 0) {
			_tprintf(TEXT("Monitor desconectado \n"));
		}
		LeaveCriticalSection(tDados->cs);

	}
	return 0;
}

BOOL isUniqueInstance(HANDLE* semaphoreStart) {
	semaphoreStart = CreateSemaphore(NULL, 0, 1, TEXT("START_SERVIDOR_SEMAPHORE"));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(semaphoreStart);
		return FALSE;
	}
	return TRUE;
}

int _tmain(int argc, LPTSTR argv[]) {

	TDados dados;
	HKEY chave = NULL;
	TCHAR chave_completa[TAM];
	DWORD res;
	DWORD dwPairValue = 0;
	DWORD dwBufferSize = sizeof(DWORD);
	ThreadDadosMemPartilhada tDadosMemPartilhada;
	HANDLE hFileMap, hjogo;
	HANDLE semaphoreStart;
	CRITICAL_SECTION csBuffer;
	HANDLE hEventsDados;
	HANDLE hTerminate;

#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif
	//verifica se exite outro servidor a correr, ao verificar a exitencia do mutex criadp
	   // VERIFICAR SE J� EXISTE ALGUMA INST�NCIA DO SERVIDOR A CORRER
	if (isUniqueInstance(&semaphoreStart) == FALSE) {
		_tprintf(TEXT("[!] J� existe uma inst�ncia do servidor a correr"));
		return -1;
	}
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
;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT(REGISTRYPATH),
		0, KEY_ALL_ACCESS, &chave)==ERROR_SUCCESS)
	{
		_tprintf(TEXT("Chave existe."));
	}
	else {
		//cria chave
		_stprintf_s(chave_completa, TAM, TEXT(REGISTRYPATH), TEXT("Frogger"));
		if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_completa, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &chave, &res) != ERROR_SUCCESS)
		{
			_tprintf(TEXT("Erro a criar ou abrir a chave (%d)."), GetLastError());
			return 1;
		}
	}
	
	DWORD regData;
	//verifica��o de argumentos Registry // servidor ,velocidade inical, faixas
	if(argc > 2) {
		if (_tstoi(argv[1]) > 10 || _tstoi(argv[1]) < 1 || _tstoi(argv[2]) < 1 || _tstoi(argv[2]) > 10)
		{
			_tprintf(TEXT("Valores na gama errada, velocidade inicial [1-10], numero de faixas [1-10]"));
			return 1;
		}
		else
		{

			regData = _tstoi(argv[1]);
			if (!(RegSetValueEx(chave, TEXT(CONFIGPAIRNAME2), 0, REG_DWORD, (BYTE*)&regData, sizeof(regData)) == ERROR_SUCCESS))
			{
				_tprintf(TEXT("Erro a registar par chave"));
				return 1;
			}
			dados.dim_max = _tstoi(argv[1]);
			

			regData = _tstoi(argv[2]);
			if (!(RegSetValueEx(chave, TEXT(CONFIGPAIRNAME1), 0, REG_DWORD, (BYTE*)&regData, sizeof(regData)) == ERROR_SUCCESS))
			{
				_tprintf(TEXT("Erro a registar par chave"));
				return 1;
			}
			dados.velocidade= _tstoi(argv[2]);
			
		}

	}
	else if (argc > 1) //_tstoi
	{
		if (_tstoi(argv[1]) > 10 || _tstoi(argv[1]) < 1)
		{
			_tprintf(TEXT("Valores na gama errada, velocidade inicial [1-10], numero de faixas [1-10]"));
			return 1;
		}
		else
		{
			//usa apenas primeiro argumento
			regData = _tstoi(argv[1]);
			if (!(RegSetValueEx(chave, TEXT(CONFIGPAIRNAME2), 0, REG_DWORD, (BYTE*)&regData, sizeof(regData)) == ERROR_SUCCESS))
			{
				_tprintf(TEXT("Erro a registar par chave"));
				return 1;
			}
			dados.dim_max = _tstoi(argv[1]);


			if (RegQueryValueEx(chave, TEXT(CONFIGPAIRNAME1), NULL, NULL, (LPBYTE)&dwPairValue, &dwBufferSize) == ERROR_SUCCESS)
				dados.velocidade = dwPairValue;
			else
			{
				DWORD regData = VELOCIDADE_INICIAL;
				if (!(RegSetValueEx(chave, TEXT(CONFIGPAIRNAME1), 0, REG_DWORD, (BYTE*)&regData, sizeof(regData)) == ERROR_SUCCESS))
				{
					_tprintf(TEXT("Erro a registar par chave"));
					return 1;
				}
				dados.velocidade = VELOCIDADE_INICIAL;
			}
		}
	}
	else
	{
		if (RegQueryValueEx(chave, TEXT(CONFIGPAIRNAME2), NULL, NULL, (LPBYTE)&dwPairValue, &dwBufferSize) == ERROR_SUCCESS)
			dados.dim_max = dwPairValue;
		else
		{
			DWORD regData = NUMERO_FAIXAS;
			if (!(RegSetValueEx(chave, TEXT(CONFIGPAIRNAME2), 0, REG_DWORD, (BYTE*)&regData, sizeof(regData)) == ERROR_SUCCESS))
			{
				_tprintf(TEXT("Erro a registar par chave"));
				return 1;
			}
			dados.dim_max = NUMERO_FAIXAS;
		}


		if (RegQueryValueEx(chave, TEXT(CONFIGPAIRNAME1), NULL, NULL, (LPBYTE)&dwPairValue, &dwBufferSize) == ERROR_SUCCESS)
			dados.velocidade = dwPairValue;
		else
		{
			DWORD regData = VELOCIDADE_INICIAL;
			if (!(RegSetValueEx(chave, TEXT(CONFIGPAIRNAME1), 0, REG_DWORD, (BYTE*)&regData, sizeof(regData)) == ERROR_SUCCESS))
			{
				_tprintf(TEXT("Erro a registar par chave"));
				return 1;
			}
			dados.velocidade = VELOCIDADE_INICIAL;
		}
	}
	//memoria partilhada
	(void)InitializeCriticalSectionAndSpinCount(&csBuffer, 500);
	tDadosMemPartilhada.cs = &csBuffer;
	tDadosMemPartilhada.hSemEscrita = CreateSemaphore(NULL, 2, 2, TEXT("SO2_SEMAFORO_ESCRITA"));
	tDadosMemPartilhada.hSemLeitura = CreateSemaphore(NULL, 0, 2, TEXT("SO2_SEMAFORO_LEITURA"));
	tDadosMemPartilhada.hMutex = CreateMutex(NULL, FALSE, TEXT("SO2_MUTEX_CONSUMIDOR"));
	if (tDadosMemPartilhada.hSemEscrita == NULL || tDadosMemPartilhada.hSemLeitura == NULL || tDadosMemPartilhada.hMutex == NULL) {
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
	tDadosMemPartilhada.memPar = (BufferCircular*)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (tDadosMemPartilhada.memPar == NULL) {
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
	tDadosMemPartilhada.jogo = (Jogo*)MapViewOfFile(hjogo, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (tDadosMemPartilhada.jogo == NULL) {
		_tprintf(TEXT("[!] Ocorreu um erro ao mapear uma vista do ficheiro"));
		CloseHandle(hFileMap);
		return -1;
	}

	tDadosMemPartilhada.memPar->nConsumidores = 0;
	tDadosMemPartilhada.memPar->nProdutores = 0;
	tDadosMemPartilhada.memPar->posE = 0;
	tDadosMemPartilhada.memPar->posL = 0;
	tDadosMemPartilhada.terminar = 0;
	tDadosMemPartilhada.jogo->dim_max = dados.dim_max;
	tDadosMemPartilhada.hEvent = hEventsDados;
	tDadosMemPartilhada.terminate_event = hTerminate;
	tDadosMemPartilhada.jogo->v_inicial = dados.velocidade;

	WaitForSingleObject(tDadosMemPartilhada.hMutex, INFINITE);
	tDadosMemPartilhada.memPar->nConsumidores++;
	ReleaseMutex(tDadosMemPartilhada.hMutex);
	//GERAR MAPA E POSI��ES INICIAIS
	inicia_jogo(tDadosMemPartilhada.jogo, dados.dim_max);
	mostra_mapa(tDadosMemPartilhada.jogo->mapa, dados.dim_max);

	//Lancamento de threads
	
	HANDLE recebeComandos = CreateThread(NULL, 0, threadConsumidor, &tDadosMemPartilhada, 0, NULL);
	if (recebeComandos == NULL) {
		_tprintf(TEXT("[!] Erro ao criar a thread.\n"));
		UnmapViewOfFile(tDadosMemPartilhada.jogo);
		UnmapViewOfFile(tDadosMemPartilhada.memPar);
		CloseHandle(hFileMap);
		CloseHandle(hjogo);
		return -1;
	}
	
	HANDLE threadMovimento = CreateThread(NULL, 0, threadJogo, &tDadosMemPartilhada, 0, NULL);
	if (threadMovimento == NULL) {
		_tprintf(TEXT("[!] Erro ao criar a thread.\n"));
		UnmapViewOfFile(tDadosMemPartilhada.jogo);
		UnmapViewOfFile(tDadosMemPartilhada.memPar);
		CloseHandle(hFileMap);
		CloseHandle(hjogo);
		return -1;
	}
	HANDLE threadComandos = CreateThread(NULL, 0, threadConsoleInterface, &tDadosMemPartilhada, 0, NULL);
	if (threadComandos == NULL) {
		_tprintf(TEXT("[!] Erro ao criar a thread.\n"));
		UnmapViewOfFile(tDadosMemPartilhada.jogo);
		UnmapViewOfFile(tDadosMemPartilhada.memPar);
		CloseHandle(hFileMap);
		CloseHandle(hjogo);
		return -1;
	}
	HANDLE threadTerminar = CreateThread(NULL, 0, threadTerminate, &tDadosMemPartilhada, 0, NULL);
	if (threadComandos == NULL) {
		_tprintf(TEXT("[!] Erro ao criar a thread.\n"));
		UnmapViewOfFile(tDadosMemPartilhada.jogo);
		UnmapViewOfFile(tDadosMemPartilhada.memPar);
		CloseHandle(hFileMap);
		CloseHandle(hjogo);
		return -1;
	}
	tDadosMemPartilhada.Threads[0] = threadComandos;
	tDadosMemPartilhada.Threads[1] = recebeComandos;
	tDadosMemPartilhada.Threads[2] = threadMovimento;


	WaitForSingleObject(threadComandos, INFINITE);
	WaitForSingleObject(recebeComandos, INFINITE);
	WaitForSingleObject(threadMovimento, INFINITE);
	//WaitForSingleObject(threadTerminar, INFINITE);

	UnmapViewOfFile(tDadosMemPartilhada.jogo);
	UnmapViewOfFile(tDadosMemPartilhada.memPar);
	CloseHandle(hFileMap);
	CloseHandle(hjogo);
	CloseHandle(threadComandos);
	CloseHandle(recebeComandos);
	CloseHandle(threadMovimento);
	RegCloseKey(chave);
	
	
	return 0;
}