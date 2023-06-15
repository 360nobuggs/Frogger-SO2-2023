#pragma once
#include "servidor.h"

#ifdef UTILS_EXPORTS
#define DLL_IMP_API __declspec(dllexport)
#else
#define DLL_IMP_API __declspec(dllimport)
#endif

BOOL isUniqueInstance(HANDLE* semaphoreStart);
BOOL inicializa_mem(ThreadDadosMemPartilhada* tDadosMemPartilhada);
BOOL inicializa_mem_s(ThreadDadosMemPartilhada* tDadosMemPartilhada, TDados dados, HANDLE* jogo, HANDLE* hFileMap);
