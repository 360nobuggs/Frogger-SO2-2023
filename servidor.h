#pragma once
#include "jogo.h"

#define TAM 200
#define MAX_STR_SIZE 128
#define VELOCIDADE_INICIAL 3
#define REGISTRYPATH "Software\\Frogger"
#define CONFIGPAIRNAME1 "VEL_INI"
#define CONFIGPAIRNAME2 "DIM"

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
	CelulaBuffer buffer[2];
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
	HANDLE *Thread_jogo;
	HANDLE *hEvent;
}ThreadDadosMemPartilhada;

