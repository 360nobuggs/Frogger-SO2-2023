#include "servidor.h"

BOOL isUniqueInstance(HANDLE* semaphoreStart) {
	semaphoreStart = CreateSemaphore(NULL, 0, 1, TEXT("START_SERVIDOR_SEMAPHORE"));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(semaphoreStart);
		return FALSE;
	}
	return TRUE;
}
void clrscr()
{
	system("@cls||clear");
}
DWORD WINAPI threadTerminate(LPVOID lpParam) {
	ThreadDadosMemPartilhada* tDados = (ThreadDadosMemPartilhada*)lpParam;
	HANDLE hEventTer = OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("TERMINATE"));
	if (hEventTer == NULL)
	{
		_tprintf(TEXT("\n Erro a abrir evento."));
		return 1;
	}
	if (WaitForSingleObject(hEventTer, INFINITE) == WAIT_OBJECT_0)
	{
		TerminateThread(tDados->Threads[0], 10);
		TerminateThread(tDados->Threads[1], 10);
		_tprintf(TEXT("\n Monitor terminado por ordem do servidor."));
		ExitThread(0);
	}
	return 0;
}
DWORD WINAPI threadMapa(LPVOID lpParam) {
	ThreadDadosMemPartilhada* tDados = (ThreadDadosMemPartilhada*)lpParam;
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("EVENTO_DADOS"));
	if (hEvent == NULL)
	{
		_tprintf(TEXT("\n Erro a abrir evento."));
		return 1;
	}
	
	while (tDados->terminar != 1) //ATIVA QUANDO EVENTO DE MUDANCA
	{
		if (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)
		{
			//_tprintf(TEXT("%c"), _getch());
			clrscr();
			
			_tprintf(TEXT("\nMenu:\nsair-termina operador\npararLinha-suspende jogo\ninverterLinha y-inverte a direcão de linha y\ninserirBloco y x-insere um bloco em xy\n"));
			_tprintf(TEXT("\n Pontuação J1: %d"),tDados->jogo->pontuacao);
			for (int i = 0; i <= tDados->jogo->dim_max+1; i++)
			{
				_tprintf(TEXT("\n"));
				for (int j = 0; j < NUMERO_COL; j++)
				{
					_tprintf(TEXT("%c"), tDados->jogo->mapa[i].linha[j]);
				}
			}
		}
		ResetEvent(hEvent);
	}

}
DWORD WINAPI threadProdutor(LPVOID lpParam) {
	//ThreadDados* threadDados = (ThreadDados*) lpParam;
	ThreadDadosMemPartilhada* tDados = (ThreadDadosMemPartilhada*)lpParam;
	const TCHAR* seps = TEXT(" \n");
	while (tDados->terminar != 1)
	{
		CelulaBuffer cel;
		TCHAR* cmdToken = NULL;
		TCHAR* cmdNextToken = NULL;
		_tprintf(TEXT("\nMenu:\nsair-termina operador\npararLinha-suspende linha\ninverterLinha y-inverte a direcão de linha y\ninserirBloco y x-insere um bloco em xy\n"));
		while (TRUE) {
			_ftprintf(stdout, TEXT(">"));
			_fgetts(cel.tcMemPartilhada, MAX_STR_SIZE, stdin);
			cmdToken = _tcstok_s(cel.tcMemPartilhada, seps, &cmdNextToken);
			if (cmdToken == NULL) continue;
			if (_tcscmp(cmdToken, TEXT("sair")) == 0) {
				tDados->terminar = 1;
				break;
			}
			else if (_tcscmp(cmdToken, TEXT("inverterLinha")) == 0) {
				cmdToken = _tcstok_s(NULL, seps, &cmdNextToken);
				if (cmdToken == NULL || _tstoi(cmdToken) == 0) {
					_tprintf(TEXT("Segundo parametro incorreto...\inverterLinha [linha] \n"));
					continue;
				}

				cel.parametros[0] = _tstoi(cmdToken);
				break;
			}
			else if (_tcscmp(cmdToken, TEXT("pararLinha")) == 0) {
				cmdToken = _tcstok_s(NULL, seps, &cmdNextToken);
				if (cmdToken == NULL || _tstoi(cmdToken) == 0) {
					_tprintf(TEXT("Segundo parametro incorreto...\pararLinha [linha] \n"));
					continue;
				}

				cel.parametros[0] = _tstoi(cmdToken);
				break;
			}
			else if (_tcscmp(cmdToken, TEXT("inserirBloco")) == 0) {
				if ((cmdToken = _tcstok_s(NULL, seps, &cmdNextToken)) == NULL
					|| (cel.parametros[0] = _tstoi(cmdToken)) == 0) {
					_tprintf(TEXT("Segundo parametro incorreto...\ninserirBloco [numero_linha] [numero_coluna]\n"));
					continue;
				}
				if ((cmdToken = _tcstok_s(NULL, seps, &cmdNextToken)) == NULL
					|| (cel.parametros[1] = _tstoi(cmdToken)) == 0) {
					_tprintf(TEXT("Terceiro parametro incorreto...\ninserirBloco [numero_linha] [numero_coluna]\n"));
					continue;
				}
				_tprintf(TEXT("linha: %d || coluna: %d "), cel.parametros[0], cel.parametros[1]);
				break;
			}
			else
			{
				_tprintf(TEXT("Esse comando não existe\n"));
				continue;
			}

		}
		WaitForSingleObject(tDados->hSemEscrita, INFINITE);
		WaitForSingleObject(tDados->hMutex, INFINITE);
		CopyMemory(&tDados->memPar->buffer[tDados->memPar->posE], &cel, sizeof(CelulaBuffer));

		if (tDados->memPar->posE == 2) {
			tDados->memPar->posE = 0;
		}

		ReleaseMutex(tDados->hMutex);
		ReleaseSemaphore(tDados->hSemLeitura, 1, NULL);
	}

	return 0;
}




int  _tmain(int argc, TCHAR* argv[]) {
	HANDLE hSemaphoreServidorUnique;
	HANDLE hThreadProdutor, hThreadMapa, hThreadTerminate;
	HANDLE hFileMap, hFileJogo;
	ThreadDadosMemPartilhada tDadosMemPartilhada;
	TDados threadDados;
	DWORD dwThreadId;
	TCHAR cmd[256];
	CRITICAL_SECTION cs;
#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif // UNICODE
	//verifica se exite outro servidor a correr, ao verificar a exitencia do mutex criadp
	if (isUniqueInstance(&hSemaphoreServidorUnique) == TRUE) {
		_tprintf(TEXT("\nNão existe um servidor a correr.\n"));
		return -1;
	}



	tDadosMemPartilhada.hSemEscrita = CreateSemaphore(NULL, 2, 2, TEXT("SO2_SEMAFORO_ESCRITA"));
	tDadosMemPartilhada.hSemLeitura = CreateSemaphore(NULL, 0, 2, TEXT("SO2_SEMAFORO_LEITURA"));

	tDadosMemPartilhada.hMutex = CreateMutex(NULL, FALSE, TEXT("SO2_MUTEX_PRODUTOR"));
	if (tDadosMemPartilhada.hSemEscrita == NULL || tDadosMemPartilhada.hSemLeitura == NULL || tDadosMemPartilhada.hMutex == NULL) {
		_tprintf(TEXT("Erro no semaforo ou no mutex"));
		return -1;
	}

	hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("SO2_MEM_PARTILHADA"));
	if (hFileMap == NULL) return -1;
	tDadosMemPartilhada.memPar = (BufferCircular*)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (tDadosMemPartilhada.memPar == NULL)
	{
		_tprintf(TEXT("erro no mapviewoffile"));
		return -1;
	}


	//filemaping jogo
	hFileJogo = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("SO2_MEM_JOGO"));
	if (hFileJogo == NULL) return -1;

	tDadosMemPartilhada.jogo = (Jogo*)MapViewOfFile(hFileJogo, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (tDadosMemPartilhada.memPar == NULL)
	{
		_tprintf(TEXT("erro no mapviewoffile"));
		return -1;
	}

	WaitForSingleObject(tDadosMemPartilhada.hMutex, INFINITE);
	tDadosMemPartilhada.memPar->nProdutores++;
	ReleaseMutex(tDadosMemPartilhada.hMutex);
	hThreadProdutor = CreateThread(
		NULL,
		0,
		threadProdutor,
		&tDadosMemPartilhada,
		0,
		NULL);
	if (hThreadProdutor == NULL) {
		_tprintf(TEXT("Erro na thread consumidor"));
		return -1;
	}
	hThreadMapa = CreateThread(
		NULL,
		0,
		threadMapa,
		&tDadosMemPartilhada,
		0,
		NULL);
	if (hThreadMapa == NULL) {
		_tprintf(TEXT("Erro na thread mapa"));
		return -1;
	}
	hThreadTerminate = CreateThread(
		NULL,
		0,
		threadTerminate,
		&tDadosMemPartilhada,
		0,
		NULL);
	if (hThreadTerminate == NULL) {
		_tprintf(TEXT("Erro na thread mapa"));
		return -1;
	}
	tDadosMemPartilhada.Threads[0] = hThreadMapa;
	tDadosMemPartilhada.Threads[1] = hThreadProdutor;
	


	WaitForSingleObject(hThreadProdutor, INFINITE);
	WaitForSingleObject(hThreadMapa, INFINITE);
	//WaitForSingleObject(hThreadTerminate, INFINITE);
	tDadosMemPartilhada.terminar = 1;

	CloseHandle(hThreadProdutor);
	CloseHandle(hThreadMapa);
	CloseHandle(hThreadTerminate);
	return 0;
}

