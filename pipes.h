#include "servidor.h"
DWORD WINAPI ThreadAtualizaSapo(LPVOID param);
int intrepretaComandosSapo(Men_Atualiza* men, Mensagem_Sapo mensagem, int id_sapo);
DWORD WINAPI ThreadRecebeSapo(LPVOID param);
DWORD WINAPI leitorMensagens(LPVOID lpParam);
