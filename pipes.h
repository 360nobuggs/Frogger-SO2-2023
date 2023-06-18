#include "servidor.h"
DWORD WINAPI ThreadAtualizaSapo(LPVOID param);
int intrepretaComandosSapo(Men_Atualiza* men, Mensagem_Sapo mensagem, int id_sapo);
DWORD WINAPI threadRecebeMensagens(LPVOID param);
DWORD WINAPI threadRecebeSapos(LPVOID lpParam);
