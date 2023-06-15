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
				if (jogoCorre == FALSE) _tprintf(TEXT("O jogo já está em pausa\n"));
				else {
					SuspendThread(threadDados->Threads[2]);
					_tprintf(TEXT("O jogo parou\n"));
					jogoCorre = FALSE;
				}
				continue;
			}
			else if (_tcscmp(cmdToken, TEXT("continuar")) == 0) {
				if (jogoCorre == TRUE) _tprintf(TEXT("O jogo já está a correr\n"));
				else
				{
					ResumeThread(threadDados->Threads[2]);
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


DWORD WINAPI ThreadAtualizaSapo(LPVOID param) //envia mensagens
{
	Mensagem_Sapo mensagem;
	DWORD n;
	int i;
	Men_Atualiza* dados = (Men_Atualiza*)param;
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("EVENTO_DADOS"));
	if (hEvent == NULL)
	{
		_tprintf(TEXT("\n Erro a abrir evento."));
		return 1;
	}
	do {
		if (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)
		{
			for (i = 0; i < N; i++) //ENVIA A MESMA MENSAGEM PARA TODOS OS SAPOS ATIVOS
			{
				WaitForSingleObject(dados->td->hMutex, INFINITE);
				if (dados->td->hPipes[i].activo == 1)
				{
					//_tprintf(TEXT("[ERRO] %c\n", dados->jogo->mapa[3].linha[4]));
					mensagem.id = 1;
					//mensagem.mutex_escrita = &dados->td->hMutex;
					mensagem.jogo = *dados->memoria->jogo;
					
					if (!WriteFile(dados->td->hPipes[i].hPipe, &mensagem, sizeof(mensagem), &n, NULL)) {
						_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
						dados->td->hPipes[i].activo = 0;
					}
					else {
						_tprintf(TEXT("[ThreadDados] Enviei %d bytes ao sapo %d... (WriteFile)\n"), n, i);
					}
				}
				ReleaseMutex(dados->td->hMutex);
			}
			ResetEvent(hEvent);
		}

	} while (dados->td->terminar!=1);
	//ASSINALAR PARA TERMINAR 
	for (i = 0; i < N; i++)
		SetEvent(dados->td->hEvents[i]);
	return 0;
}
int intrepretaComandosSapo(Men_Atualiza *men, Mensagem_Sapo mensagem, int id_sapo)
{
	if (mensagem.cmd == LEFT || mensagem.cmd == RIGHT || mensagem.cmd == UP || mensagem.cmd == DOWN)
	{
		EnterCriticalSection(men->memoria->cs);
		move_sapo(mensagem.cmd, men->memoria->jogo, id_sapo);
		LeaveCriticalSection(men->memoria->cs);
		SetEvent(men->memoria->hEvent);
	}
	else if (mensagem.cmd==SAIR)
	{
		return 1;//sapo desconectado
	}
	return 0;
}

DWORD WINAPI ThreadRecebeSapo(LPVOID param) //envia mensagens
{
	Mensagem_Sapo mensagem;
	DWORD n;
	Men_Atualiza* dados = (Men_Atualiza*)param;
	int num = dados->td->n_sapo;
	//para cada sapo um pipe de leitura
	
	do {
		if (!ReadFile(dados->td->hPipesR[num].hPipe, &mensagem, sizeof(mensagem), &n, NULL)) {
			_tprintf(TEXT("[ERRO] A receber mensagem! (ReadFile)\n"));
			return 0; //FAILSAFE
		}
		else {
				_tprintf(TEXT("[ThreadDados] Mensagem %d recebida do sapo %d\n"), mensagem.cmd, num);
				if (intrepretaComandosSapo(dados, mensagem, num) == 1)
				{
					_tprintf(TEXT("[ThreadDados] Sapo numero %d desconectado \n"), num);
					dados->td->hPipes[num].activo = 0;//desativa pipe de envio
					dados->td->hPipesR[num].activo = 0;
					dados->td->n_sapo = dados->td->n_sapo - 1;
					return 1;
				}
		}
	} while (dados->td->terminar != 1);
	return 0;
}
DWORD WINAPI leitorMensagens(LPVOID lpParam) {
	HANDLE hPipe,hPipeR, hThread, hEventTemp,hThreadRecepcao1,hThreadRecepcao2;
	ThreadDadosMemPartilhada* threadDados = (ThreadDadosMemPartilhada*)lpParam;
	ThreadMensagemDados dados;
	int i, numClientes = 0;
	DWORD offset, nBytes, n;
	Mensagem_Sapo mensagem;
	HANDLE mutex_escrita_sapo;
	dados.terminar = 0;
	dados.n_sapo = -1;
	dados.hMutex = CreateMutex(NULL, FALSE, NULL);
	if (dados.hMutex == NULL)
	{
		_tprintf(TEXT("[ThreadDados] Erro a criar mutex"));
		exit(-1);
	}
	mutex_escrita_sapo=CreateMutex(NULL, FALSE, NULL);
	if (mutex_escrita_sapo == NULL)
	{
		_tprintf(TEXT("[ThreadDados] Erro a criar mutex"));
		exit(-1);
	}
	for (int i = 0; i < N; i++) {
		//cria uma nova instancia para cada leitor
		//vai aceitar novos clientes em ciclo
		_tprintf(TEXT("[ThreadDados] Criar uma copia do pipe '%s' ... (CreateNamedPipe)\n"), PIPE_NAME);
		//N CLIENTES
		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT |
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, N,
			sizeof(Mensagem_Sapo), sizeof(Mensagem_Sapo), 1000, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ThreadDados] [ERRO] Criar Named Pipe! (CreateNamedPipe)"));
			exit(-1);
		}
		//PIPE DE RECEPCAO DE MENSAGENS
		
		hPipeR = CreateNamedPipe(PIPE_NAME_R, PIPE_ACCESS_DUPLEX, PIPE_WAIT |
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, N,
			sizeof(Mensagem_Sapo), sizeof(Mensagem_Sapo), 1000, NULL);
		if (hPipeR == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ThreadDados] [ERRO] Criar Named Pipe R! (CreateNamedPipe)"));
			exit(-1);
		}

		//evento overlapped
		hEventTemp = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hEventTemp == NULL)
		{
			_tprintf(TEXT("[ThreadDados] Erro a criar Evento"));
			exit(-1);
		}
		dados.hPipes[i].hPipe = hPipe;
		dados.hPipes[i].activo = FALSE; //NAO ESTA ATIVO NAO TEM CLIENTE ASSOCIADO
		ZeroMemory(&dados.hPipes[i].overlap, sizeof(dados.hPipes[i].overlap));
		dados.hPipes[i].overlap.hEvent = hEventTemp;
		dados.hEvents[i] = hEventTemp;
		dados.hPipesR[i].hPipe = hPipeR;
		_tprintf(TEXT("[ThreadDados] Esperar ligacao de um Sapo... (ConnectNamedPipe)\n"));
		ConnectNamedPipe(hPipe, &dados.hPipes[i].overlap);// retorno ainda nao da para saber se tem sucesso
		if (GetLastError == ERROR_IO_PENDING)
		{
			_tprintf(TEXT("[ThreadDados] Problema a conectar pipe.\n"));
		}

	}

	Men_Atualiza men;
	men.td = &dados;
	men.memoria = threadDados;
	hThread = CreateThread(NULL, 0, ThreadAtualizaSapo, &men, 0, NULL);
	if (hThread == NULL)
	{
		_tprintf(TEXT("Erro a criar thread"));
		exit(-1);
	}

	while (!dados.terminar && (numClientes < N))
	{
		offset = WaitForMultipleObjects(N, dados.hEvents, FALSE, INFINITE);
		i = offset - WAIT_OBJECT_0; //indice do array do evento que desbloqueou
		if (i >= 0 && i < N)
		{
			dados.n_sapo++;//numero de sapos conectados
			_tprintf(TEXT("[ThreadDados] Sapo numero %d conectado \n"), i);
			
			if (dados.n_sapo == 0)
			{
				hThreadRecepcao1 = CreateThread(NULL, 0, ThreadRecebeSapo, &men, 0, NULL);
				if (hThread == NULL)
				{
					_tprintf(TEXT("Erro a criar thread"));
					exit(-1);
				}
			}
			else
			{
				hThreadRecepcao2 = CreateThread(NULL, 0, ThreadRecebeSapo, &men, 0, NULL);
				if (hThread == NULL)
				{
					_tprintf(TEXT("Erro a criar thread"));
					exit(-1);
				}
			}
			

			//ENVIO DE INFORMACAO INICIAL
			mensagem.id = 1;
			mensagem.jogo = *threadDados->jogo;
			mensagem.mutex_escrita = mutex_escrita_sapo;
			WaitForSingleObject(dados.hMutex, INFINITE);
			if (!WriteFile(dados.hPipes[i].hPipe, &mensagem, sizeof(mensagem), &n, NULL)) {
				_tprintf(TEXT("[ThreadDados][ERRO] Escrever no pipe! (WriteFile)\n"));
			}
			else {
				_tprintf(TEXT("[ThreadDados] Enviei %d bytes ao sapo %d... (WriteFile)\n"), n, i);
			}
			ReleaseMutex(dados.hMutex);


			if (GetOverlappedResult(dados.hPipes[i].hPipe, &dados.hPipes[i].overlap, &nBytes, FALSE))
			{
				//evento de reset manual
				ResetEvent(dados.hEvents[i]);
				WaitForSingleObject(dados.hMutex, INFINITE);
				dados.hPipes[i].activo = TRUE;//esta instancia esta ativa
				ReleaseMutex(dados.hMutex);
				numClientes++;
			}
		}
		dados.terminar = threadDados->terminar;
	}
	WaitForSingleObject(hThread, INFINITE);
	

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
		Sleep(10000 - (tDados->jogo->v_inicial) * 100);
		EnterCriticalSection(tDados->cs);
		//_tprintf(TEXT("\n valor de : %d\n"), tDados->jogo->v_inicial);
		int a = tDados->jogo->dim_max;
		for (int i = 1; i <= tDados->jogo->dim_max; i++)
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
			SuspendThread(tDados->Threads[2]);
			Sleep(1000 * cel.parametros[0]);
			ResumeThread(tDados->Threads[2]);
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
	   // VERIFICAR SE JÁ EXISTE ALGUMA INSTÂNCIA DO SERVIDOR A CORRER
	if (isUniqueInstance(&semaphoreStart) == FALSE) {
		_tprintf(TEXT("[!] Já existe uma instância do servidor a correr"));
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
	//verificação de argumentos Registry // servidor ,velocidade inical, faixas
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
	//GERAR MAPA E POSIÇÕES INICIAIS
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
	if (threadTerminar == NULL) {
		_tprintf(TEXT("[!] Erro ao criar a thread.\n"));
		UnmapViewOfFile(tDadosMemPartilhada.jogo);
		UnmapViewOfFile(tDadosMemPartilhada.memPar);
		CloseHandle(hFileMap);
		CloseHandle(hjogo);
		return -1;
	}
	HANDLE threadMensagem = CreateThread(NULL, 0, leitorMensagens, &tDadosMemPartilhada, 0, NULL);
	if (leitorMensagens == NULL) {
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
	WaitForSingleObject(threadMensagem, INFINITE);
	//WaitForSingleObject(threadTerminar, INFINITE);

	UnmapViewOfFile(tDadosMemPartilhada.jogo);
	UnmapViewOfFile(tDadosMemPartilhada.memPar);
	CloseHandle(hFileMap);
	CloseHandle(hjogo);
	CloseHandle(threadComandos);
	CloseHandle(recebeComandos);
	CloseHandle(threadMovimento);
	CloseHandle(threadMensagem);
	RegCloseKey(chave);
	
	
	return 0;
}