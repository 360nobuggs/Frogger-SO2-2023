#include "pipes.h";

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
					WaitForSingleObject(dados->memoria->hMutex, INFINITE);
					mensagem.jogo = *dados->memoria->jogo;
					ReleaseMutex(dados->memoria->hMutex);

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

	} while (dados->td->terminar != 1);
	//ASSINALAR PARA TERMINAR 
	for (i = 0; i < N; i++)
		SetEvent(dados->td->hEvents[i]);
	return 0;
}
int intrepretaComandosSapo(Men_Atualiza* men, Mensagem_Sapo mensagem, int id_sapo)
{
	if (mensagem.cmd == LEFT || mensagem.cmd == RIGHT || mensagem.cmd == UP || mensagem.cmd == DOWN)
	{
		WaitForSingleObject(men->memoria->hMutex,INFINITE);
		move_sapo(mensagem.cmd, men->memoria->jogo, id_sapo);
		ReleaseMutex(men->memoria->hMutex);
		SetEvent(men->memoria->hEvent);
	}
	else if (mensagem.cmd == SAIR)
	{
		return 1;//sapo desconectado
	}
	else if (mensagem.cmd == REGRESSO)
	{
		WaitForSingleObject(men->memoria->hMutex, INFINITE);
		reset_sapo(men->memoria->jogo, men->td->n_sapo);
		ReleaseMutex(men->memoria->hMutex);
	}
	return 0;
}

DWORD WINAPI threadRecebeMensagens(LPVOID param) //recebe mensagens
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
				WaitForSingleObject(dados->td->hMutex,INFINITE);
				dados->td->hPipes[num].activo = 0;//desativa pipe de envio
				dados->td->hPipesR[num].activo = 0;
				dados->td->n_sapo = dados->td->n_sapo - 1;
				ReleaseMutex(dados->td->hMutex);
				return 1;
			}
		}
	} while (dados->td->terminar != 1);
	return 0;
}
DWORD WINAPI threadRecebeSapos(LPVOID lpParam) { //recebe os sapos
	HANDLE hPipe, hPipeR, hThread, hEventTemp, hThreadRecepcao1, hThreadRecepcao2;
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
	mutex_escrita_sapo = CreateMutex(NULL, FALSE, NULL);
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
		if (GetLastError() == ERROR_IO_PENDING)
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
				hThreadRecepcao1 = CreateThread(NULL, 0, threadRecebeMensagens, &men, 0, NULL);
				if (hThread == NULL)
				{
					_tprintf(TEXT("Erro a criar thread"));
					exit(-1);
				}
			}
			else
			{
				hThreadRecepcao2 = CreateThread(NULL, 0, threadRecebeMensagens, &men, 0, NULL);
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
