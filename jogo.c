#include "jogo.h"
#include <stdlib.h>
#include  <tchar.h>
#include <windows.h>

void inicia_jogo(Jogo* jogo, int numero_faixas) {
	//linha de partida
	srand(time(NULL));
	
	

	//char a[NUMERO_COL] = {' ',' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', };
	//Linha aux = { .linha = a, .n_carros = NUMERO_MAX_CARROS,.tipo_linha = 1,.velocidade_linha = 1 };
	/*for (int i = 0; i < NUMERO_FAIXAS; i++)
	{		
		mapa_a[0] = aux;
	}*/

	
	
	for (int i = 0; i < NUMERO_COL; i++)
	{
		jogo->mapa[0].linha[i] = ' ';
	}
	
	//linha de chegada
	for (int i = 0; i < NUMERO_COL ; i++)
	{
		jogo->mapa[numero_faixas].linha[i] = ' ';
	}
	//sapo colocado na linha inicial
	jogo->sapo.y = 0;
	jogo->sapo.x = rand() % 20;
	jogo->sapo.tipo_obstaculo = 'S';
	jogo->mapa[0].linha[jogo->sapo.x] = jogo->sapo.tipo_obstaculo;

	//tipo de obstaculos 1.carros normais 2.camioes
	jogo->mapa[numero_faixas - 1].tipo_linha = 2;
	//resto do mapa
	for (int i = 1; i < numero_faixas ; i++)
	{
		jogo->mapa[i].direcao = 0;
		if (jogo->mapa[i].tipo_linha == 2)
		{
			int carros = 1;
			for (int j = 0; j < NUMERO_COL; j++)
			{
				if (NUMERO_MAX_CARROS/2 > carros)
				{
					jogo->mapa[i].linha[j] = 'L';
					j++;
					jogo->mapa[i].linha[j] = 'L';
					j++;
					carros++;
				}
				jogo->mapa[i].linha[j] = 'X';

			}

		}
		else
		{
			int carros = 1;
			for (int j = 0; j < NUMERO_COL; j++)
			{
				if (NUMERO_MAX_CARROS > carros)
				{
					jogo->mapa[i].linha[j] = 'C';
					j++;
					carros++;
				}
				jogo->mapa[i].linha[j] = 'X';

			}
		}
	}
	
	
}
void mostra_mapa(Linha mapa[NUMERO_FAIXAS], int num_faixas)
{
	for (int i = 0; i <= num_faixas; i++)
	{
		_tprintf(TEXT("\n"));
		for (int j = 0; j < NUMERO_COL; j++)
		{
			_tprintf(TEXT("%c"),mapa[i].linha[j]);
		}
	}
}
void vitoria()
{

}
void timeout_sapo()
{

}
void move_sapo(char direcao, Jogo *jogo)
{

}
void move_fila(Jogo *jogo, int linha_o)
{
	if (jogo->mapa[linha_o].direcao == 0)//direita
	{
		char aux[NUMERO_COL];
		char a = jogo->mapa[linha_o].linha[NUMERO_COL - 1];
		for (int i = 1; i < NUMERO_COL; i++)
		{
			aux[NUMERO_COL-i] = jogo->mapa[linha_o].linha[NUMERO_COL-i-1];
			jogo->mapa[linha_o].linha[NUMERO_COL - i] = aux[NUMERO_COL - i];
		}
		jogo->mapa[linha_o].linha[0] = a;
	}
	else
	{
		for (int i = 0; i < NUMERO_COL; i++)//esquerda
		{
			char aux[NUMERO_COL];
			char a = jogo->mapa[linha_o].linha[0];
			for (int i = 0; i < NUMERO_COL; i++)
			{
				aux[i] = jogo->mapa[linha_o].linha[i + 1];
				jogo->mapa[linha_o].linha[i] = aux[i];
			}
			jogo->mapa[linha_o].linha[NUMERO_COL - 1] = a;
		}
	}
	
}
void inverte_direcao(Jogo *jogo, int linha_o, int direcao)
{
	jogo->mapa[linha_o].direcao = direcao;
}
void insere_barreira(Jogo* jogo, int linha_o, int coluna_o)
{
	if (jogo->mapa[linha_o].linha[coluna_o] == ' ')
	{
		jogo->mapa[linha_o].linha[coluna_o] = 'T';
	}
}