#pragma once
#include "jogo.h"

#define TAM 200
#define MAX_STR_SIZE 128
#define N 2
#define VELOCIDADE_INICIAL 3
#define DIM_MENSAGEM 20
#define REGISTRYPATH "Software\\Frogger"
#define PIPE_NAME TEXT("\\\\.\\pipe\\sapo")
#define PIPE_NAME_R TEXT("\\\\.\\pipe\\servidor")
#define CONFIGPAIRNAME1 "VEL_INI"
#define CONFIGPAIRNAME2 "DIM"

#define UP 0
#define RIGHT 1
#define LEFT 2
#define DOWN 3
#define PONTUACAO 4
#define SAIR 5
#define TEMPO 6
#define ESPERA_JOGADOR 7 
#define JOGADOR_CHEGOU 8
#define REGRESSO 9


typedef struct {
	int velocidade;
	int dim_max;
	HANDLE hmutex;
	CRITICAL_SECTION* cs;
	HANDLE hEvent;
}TDados;

typedef struct {
	TCHAR tcMemPartilhada[256];
	int parametros[2];
}CelulaBuffer;

typedef struct {
	int nProdutores;
	int nConsumidores;
	int posE;
	int posL;
	CelulaBuffer buffer[8];
}BufferCircular;

typedef struct {
	BufferCircular* memPar;
	HANDLE hSemEscrita;
	HANDLE hSemLeitura;
	HANDLE hMutex;
	int terminar;
	int id;
	Jogo* jogo;
	CRITICAL_SECTION* cs;
	HANDLE *Threads[4];
	HANDLE *hEvent;
	HANDLE* terminate_event;
}ThreadDadosMemPartilhada;

typedef struct {
	Jogo jogo;
	TCHAR comando[TAM];
	int id;//0 para sapos, 1 para servidor
	int cmd;
	HANDLE* mutex_escrita;
}Mensagem_Sapo;

typedef struct {
	HANDLE hPipe; //instancia do named pipe � representada por handle 
	OVERLAPPED overlap;//PARA OPERA��ES ASSINCRONAS
	BOOL activo;
}PipeDados;

typedef struct {
	PipeDados hPipes[N];
	PipeDados hPipesR[N];
	HANDLE hEvents[N];
	HANDLE hMutex;
	int terminar;
	int n_sapo;
}ThreadMensagemDados;

typedef struct {
	ThreadMensagemDados* td;
	ThreadDadosMemPartilhada* memoria;
}Men_Atualiza;