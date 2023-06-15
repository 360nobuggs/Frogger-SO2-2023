#include "jogo.h"
#include <stdlib.h>
#include  <tchar.h>
#include <windows.h>

void inicia_jogo(Jogo* jogo, int numero_faixas) {
	//linha de partida
	srand(time(NULL));
	
	for (int i = 0; i < NUMERO_COL; i++)
	{
		jogo->mapa[0].linha[i] = ' ';
	}
	
	//linha de chegada
	for (int i = 0; i < NUMERO_COL ; i++)
	{
		jogo->mapa[numero_faixas+1].linha[i] = ' ';
	}
	//sapo colocado na linha inicial
	jogo->sapo[0].posicao.y = 0;
	jogo->sapo[0].pontuacao = 0;
	jogo->sapo[0].posicao.x = rand() % 20;
	jogo->sapo[0].posicao.tipo_obstaculo = 'S';
	jogo->mapa[0].linha[jogo->sapo[0].posicao.x] = jogo->sapo[0].posicao.tipo_obstaculo;

	//tipo de obstaculos 1.carros normais 2.camioes
	jogo->mapa[numero_faixas - 1].tipo_linha = 2;
	jogo->nivel = 0;
	//resto do mapa
	for (int i = 1; i <= numero_faixas ; i++)
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
					int a= rand() % 3;
					if (a < 2)
					{
						jogo->mapa[i].linha[j] = 'C';
						j++;
					}
					else
					{
						jogo->mapa[i].linha[j] = 'X';
					}
					carros++;
				}
				jogo->mapa[i].linha[j] = 'X';

			}
		}
	}
	
	
}
void mostra_mapa(Linha mapa[NUMERO_FAIXAS], int num_faixas)
{
	for (int i = 0; i <= num_faixas+1; i++)
	{
		_tprintf(TEXT("\n"));
		for (int j = 0; j < NUMERO_COL; j++)
		{
			_tprintf(TEXT("%c"),mapa[i].linha[j]);
		}
	}
}

void timeout_sapo()
{

}
int novo_nivel(Jogo *jogo)
{
	if (jogo->nivel == 5)
	{
		return 1;
	}
	srand(time(NULL));
	if (jogo->v_inicial < 30)
	{
		jogo->v_inicial = jogo->v_inicial + 5;
	}
	for (int i = 0; i < NUMERO_COL; i++)
	{
		jogo->mapa[0].linha[i] = ' ';
	}
	//linha de chegada
	for (int i = 0; i < NUMERO_COL; i++)
	{
		jogo->mapa[jogo->dim_max + 1].linha[i] = ' ';
	}
	//sapo colocado na linha inicial
	jogo->sapo[0].posicao.y = 0;
	jogo->sapo[0].posicao.x = rand() % 20;
	jogo->sapo[0].posicao.tipo_obstaculo = 'S';
	jogo->mapa[0].linha[jogo->sapo[0].posicao.x] = jogo->sapo[0].posicao.tipo_obstaculo;

	//tipo de obstaculos 1.carros normais 2.camioes
	jogo->mapa[jogo->dim_max - 1].tipo_linha = 2;
	//resto do mapa
	for (int i = 1; i <= jogo->dim_max; i++)
	{
		jogo->mapa[i].direcao = 0;
		if (jogo->mapa[i].tipo_linha == 2)
		{
			int carros = 1;
			for (int j = 0; j < NUMERO_COL; j++)
			{
				if (NUMERO_MAX_CARROS+ jogo->nivel / 2 > carros)
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
				if (NUMERO_MAX_CARROS+ jogo->nivel > carros)
				{
					int a = rand() % 3;
					if (a < 2)
					{
						jogo->mapa[i].linha[j] = 'C';
						j++;
					}
					else
					{
						jogo->mapa[i].linha[j] = 'X';
					}
					carros++;
				}
				jogo->mapa[i].linha[j] = 'X';

			}
		}
	}
	return 0;
}
int  move_sapo(int direcao, Jogo *jogo, int id_sapo)
{
	switch (direcao)
	{
	case 0://up
		if (jogo->sapo[id_sapo].posicao.y !=0)
		{
			if (jogo->mapa[jogo->sapo[id_sapo].posicao.y - 1].linha[jogo->sapo[id_sapo].posicao.x] == 'X'|| jogo->mapa[jogo->sapo[id_sapo].posicao.y - 1].linha[jogo->sapo[id_sapo].posicao.x] == ' ')//so se pode mover para posicoes vazias
			{
				//limpa posicoes
				jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = 'X';
				jogo->sapo[id_sapo].posicao.y--;//-- para cima
				//escreve novo
				jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = 'S';
			}
		}
		break;
	case 1://right
		if (jogo->sapo[id_sapo].posicao.x!= NUMERO_COL-1)
		{
			if (jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x+1] == 'X' || jogo->mapa[jogo->sapo[id_sapo].posicao.y ].linha[jogo->sapo[id_sapo].posicao.x+1] == ' ')//so se pode mover para posicoes vazias
			{
				//limpa posicoes
				if (jogo->sapo[id_sapo].posicao.y == 0)
				{
					jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = ' ';
				}
				else
				{
					jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = 'X';
				}
				jogo->sapo[id_sapo].posicao.x++;//-- para cima
				//escreve novo
				jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = 'S';
			}
		}
		break;
	case 2://left
		if (jogo->sapo[id_sapo].posicao.x-1 != -1)
		{
			if (jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x-1] == 'X' || jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x-1] == ' ')//so se pode mover para posicoes vazias
			{
				//limpa posicoes
				if (jogo->sapo[id_sapo].posicao.y == 0)
				{
					jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = ' ';
				}
				else
				{
					jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = 'X';
				}
				jogo->sapo[id_sapo].posicao.x--;//-- para cima
				//escreve novo
				jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = 'S';
			}
		}
		break;
	case 3://down
		if (jogo->sapo[id_sapo].posicao.y == jogo->dim_max) //vitoria
		{
			jogo->nivel++;
			jogo->sapo[id_sapo].pontuacao++;
			if (novo_nivel(jogo))
			{
				//VITORIA
			}
		}
		else
		{
			if (jogo->mapa[jogo->sapo[id_sapo].posicao.y+1].linha[jogo->sapo[id_sapo].posicao.x] == 'X')
			{
				//limpa posicoes
				if (jogo->sapo[id_sapo].posicao.y == 0)
				{
					jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = ' ';
				}
				else
				{
					jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = 'X';
				}
				jogo->sapo[id_sapo].posicao.y++;//-- para cima
				//escreve novo
				jogo->mapa[jogo->sapo[id_sapo].posicao.y].linha[jogo->sapo[id_sapo].posicao.x] = 'S';
			}
		}
		break;
	}
	return 0;
}
void reset_sapo(Jogo* jogo, int num_sapo)
{
	jogo->mapa[jogo->sapo[num_sapo].posicao.y].linha[jogo->sapo[num_sapo].posicao.x] = 'X';
	jogo->sapo[num_sapo].posicao.y = 0;
	int n = rand() % 20;
	while (jogo->mapa[0].linha[n] == 'S')
	{
		n = rand() % 20;
	}
	jogo->sapo[num_sapo].posicao.x = n;
	jogo->mapa[jogo->sapo[num_sapo].posicao.y].linha[jogo->sapo[num_sapo].posicao.x] = 'S';
}
void move_fila(Jogo *jogo, int linha_o)
{
	if (jogo->mapa[linha_o].direcao == 0)//direita
	{
		char aux[NUMERO_COL];
		char a = jogo->mapa[linha_o].linha[NUMERO_COL - 1];
		for (int i = 1; i < NUMERO_COL; i++)
		{
			if (jogo->mapa[linha_o].linha[NUMERO_COL - i-1] == 'S')
			{
				if (jogo->mapa[linha_o].linha[NUMERO_COL - i-2] == 'C' || jogo->mapa[linha_o].linha[NUMERO_COL - i-2] == 'L')
				{
					if ((jogo->sapo[0].posicao.x == NUMERO_COL - i -1) && (jogo->sapo[0].posicao.y == linha_o))
					{
						reset_sapo(jogo, 0);
					}
					else {
						reset_sapo(jogo, 1);
					}
				}
				aux[NUMERO_COL - i] = 'X';
				jogo->mapa[linha_o].linha[NUMERO_COL - i] = aux[NUMERO_COL - i];
				
			}
			else
			{
				aux[NUMERO_COL - i] = jogo->mapa[linha_o].linha[NUMERO_COL - i - 1];
				jogo->mapa[linha_o].linha[NUMERO_COL - i] = aux[NUMERO_COL - i];
			}
			
		}
		jogo->mapa[linha_o].linha[0] = a;
		if (jogo->sapo[0].posicao.y == linha_o)
		{
			jogo->mapa[linha_o].linha[jogo->sapo[0].posicao.x] = 'S';
		}
		if (jogo->sapo[1].posicao.y == linha_o)
		{
			jogo->mapa[linha_o].linha[jogo->sapo[0].posicao.x] = 'S';
		}
	}
	else
	{
		char aux[NUMERO_COL];
		char a = jogo->mapa[linha_o].linha[0];
		for (int i = 0; i < NUMERO_COL; i++)
		{
			if (jogo->mapa[linha_o].linha[i +1] == 'S')
			{
				if (jogo->mapa[linha_o].linha[i] == 'C' || jogo->mapa[linha_o].linha[i] == 'L')
				{
					if ((jogo->sapo[0].posicao.x == NUMERO_COL - i - 1) && (jogo->sapo[0].posicao.y == linha_o))
					{
						reset_sapo(jogo, 0);
					}
					else {
						reset_sapo(jogo, 1);
					}
				}
			}
			aux[i] = jogo->mapa[linha_o].linha[i + 1];
			jogo->mapa[linha_o].linha[i] = aux[i];
		}
		jogo->mapa[linha_o].linha[NUMERO_COL - 1] = a;
		
	}
	
}
void inverte_direcao(Jogo *jogo, int linha_o)
{
	int d = 0;
	if (jogo->mapa[linha_o].direcao == 0)
		d = 1;
	jogo->mapa[linha_o].direcao = d;
}
void insere_barreira(Jogo* jogo, int linha_o, int coluna_o)
{
	if (jogo->mapa[linha_o].linha[coluna_o] == 'X')
	{
		jogo->mapa[linha_o].linha[coluna_o] = 'T';
	}
	else
	{
		_tprintf(TEXT("\n Bloco nao pode ser colocado.\n"));
	}
}