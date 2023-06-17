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
#define TEMPO_INICIO 100

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
	Posicao posicao;
	int pontuacao;
	char nome_jogador[20];
}Sapo;

typedef struct {
	Linha mapa[NUMERO_FAIXAS];
	int nivel;
	Sapo sapo[1];//dois sapos
	int v_inicial;
	int dim_max;
	int tempo;
	int vitoria;
}Jogo;

void inicia_jogo(Jogo* jogo, int numero_faixas);
void mostra_mapa(Linha mapa[NUMERO_FAIXAS], int num_faixas);
void move_fila(Jogo* jogo, int linha_o);
void inverte_direcao(Jogo* jogo, int linha_o);
void insere_barreira(Jogo* jogo, int linha_o, int coluna_o);
int  move_sapo(int direcao, Jogo* jogo, int id_sapo);
int novo_nivel(Jogo* jogo);
void reset_sapo(Jogo* jogo, int num_sapo);
