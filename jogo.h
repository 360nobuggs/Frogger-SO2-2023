#pragma once
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <synchapi.h>
#include<conio.h>

#define BUFFER_SIZE 10
#define NAME_LENGTH 40
#define NUMERO_COL 20
#define NUMERO_FAIXAS 10
#define NUMERO_MAX_CARROS 8

typedef struct {
	char  tipo_obstaculo ;
	int x;
	int y;
}Posicao;

typedef struct {
	char linha[NUMERO_COL];
	int velocidade_linha;
	int n_carros;
	int tipo_linha;
	int direcao; //0 direita 1 esquerda
}Linha;

typedef struct {
	Linha mapa[NUMERO_FAIXAS];
	Posicao sapo;
	int pontuacao;
	int v_inicial;
	int dim_max;
}Jogo;

void inicia_jogo(Jogo* jogo, int numero_faixas);
void mostra_mapa(Linha mapa[NUMERO_FAIXAS], int num_faixas);
void move_fila(Jogo* jogo, int linha_o);
void inverte_direcao(Jogo* jogo, int linha_o, int direcao);
void insere_barreira(Jogo* jogo, int linha_o, int coluna_o);
