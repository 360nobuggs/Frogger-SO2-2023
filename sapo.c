#include "servidor.h"
#include "resource.h"

DWORD thread;
HDC memDC = NULL;
HANDLE hMutexPaint;
HBITMAP hBitmapDB;
#define PIPE_NAME TEXT("\\\\.\\pipe\\sapo")
#define PIPE_NAME_R TEXT("\\\\.\\pipe\\servidor")
#define DIM_TEXT 20
DWORD WINAPI leitorMensagens(LPVOID lpParam);
/* ===================================================== */
/* Programa base (esqueleto) para aplicações Windows     */
/* ===================================================== */
// Cria uma janela de nome "Janela Principal" e pinta fundo de branco
// Modelo para programas Windows:
//  Composto por 2 funções: 
//	WinMain()     = Ponto de entrada dos programas windows
//			1) Define, cria e mostra a janela
//			2) Loop de recepção de mensagens provenientes do Windows
//     TrataEventos()= Processamentos da janela (pode ter outro nome)
//			1) É chamada pelo Windows (callback) 
//			2) Executa código em função da mensagem recebida

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TrataEventosModo(HWND, UINT, WPARAM, LPARAM);

// Nome da classe da janela (para programas de uma só janela, normalmente este nome é 
// igual ao do próprio programa) "szprogName" é usado mais abaixo na definição das 
// propriedades do objecto janela
HWND hWnd; // hWnd é o handler da janela, gerado mais abaixo por CreateWindow()
HWND janelaglobal;
TCHAR szProgName[] = TEXT("Frogger");
int windowMode = 0;

HANDLE hPipeEscrita;
HANDLE *mutex_escrita;
HDC hdc;
HDC memdc;

HBITMAP hbitEmpty;
HBITMAP hbitWall;
HBITMAP hbitFrog;
HBITMAP hbitCar;
HBITMAP hbitBus;
HBITMAP hbitGrass;
RECT rect;
HBRUSH hbrush;
INT_PTR InitialDialog;
HINSTANCE hThisInst;
Jogo jogo;
typedef struct {
	int pos_x;
	int pos_y;
}Pos_Sapo;
typedef struct {

	TCHAR nome[DIM_TEXT];
	TCHAR pontuacao[DIM_TEXT];
	TCHAR tempo[DIM_TEXT];
	Jogo jogo;
	Pos_Sapo sapo;
	Pos_Sapo pos_adjacentes[3];//0 up 1 right 2 left 3 down
}Dados;

Dados dados;
// ============================================================================
// FUNÇÃO DE INÍCIO DO PROGRAMA: WinMain()
// ============================================================================
// Em Windows, o programa começa sempre a sua execução na função WinMain()que desempenha
// o papel da função main() do C em modo consola WINAPI indica o "tipo da função" (WINAPI
// para todas as declaradas nos headers do Windows e CALLBACK para as funções de
// processamento da janela)
// Parâmetros:
//   hInst: Gerado pelo Windows, é o handle (número) da instância deste programa 
//   hPrevInst: Gerado pelo Windows, é sempre NULL para o NT (era usado no Windows 3.1)
//   lpCmdLine: Gerado pelo Windows, é um ponteiro para uma string terminada por 0
//              destinada a conter parâmetros para o programa 
//   nCmdShow:  Parâmetro que especifica o modo de exibição da janela (usado em  
//        	   ShowWindow()

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;		// hWnd é o handler da janela, gerado mais abaixo por CreateWindow()
	MSG lpMsg;		// MSG é uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp;	// WNDCLASSEX é uma estrutura cujos membros servem para 
	// definir as características da classe da janela
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

// ============================================================================
// 1. Definição das características da janela "wcApp" 
//    (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
// ============================================================================
	wcApp.cbSize = sizeof(WNDCLASSEX);      // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst;		         // Instância da janela actualmente exibida 
	// ("hInst" é parâmetro de WinMain e vem 
		  // inicializada daí)
	wcApp.lpszClassName = szProgName;       // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = TrataEventos;       // Endereço da função de processamento da janela
	// ("TrataEventos" foi declarada no início e
	// encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;  // Estilo da janela: Fazer o redraw se for
	// modificada horizontal ou verticalmente

	wcApp.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDB_FROG));   // "hIcon" = handler do ícon normal
	// "NULL" = Icon definido no Windows
	// "IDI_AP..." Ícone "aplicação"
	wcApp.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDB_FROG)); // "hIconSm" = handler do ícon pequeno
	// "NULL" = Icon definido no Windows
	// "IDI_INF..." Ícon de informação
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);	// "hCursor" = handler do cursor (rato) 
	// "NULL" = Forma definida no Windows
	// "IDC_ARROW" Aspecto "seta" 
	wcApp.lpszMenuName = MAKEINTRESOURCE(IDR_MENU2);			// Classe do menu que a janela pode ter
	// (NULL = não tem menu)
	wcApp.cbClsExtra = 0;				// Livre, para uso particular
	wcApp.cbWndExtra = 0;				// Livre, para uso particular
	wcApp.hbrBackground = CreateSolidBrush(RGB(144, 238, 144));
		//(HBRUSH)GetStockObject(WHITE_BRUSH);
	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por
	// "GetStockObject".Neste caso o fundo será branco

	// ============================================================================
	// 2. Registar a classe "wcApp" no Windows
	// ============================================================================
	if (!RegisterClassEx(&wcApp))
		return(0);
	// ============================================================================
	// 3. Criar a janela
	thread = GetCurrentThreadId();
	// ============================================================================
	hMutexPaint = CreateMutex(NULL, FALSE, NULL);
	hWnd = CreateWindow(
		szProgName,			// Nome da janela (programa) definido acima
		TEXT("Frogger"),// Texto que figura na barra do título
		WS_OVERLAPPEDWINDOW,	// Estilo da janela (WS_OVERLAPPED= normal)
		CW_USEDEFAULT,		// Posição x pixels (default=à direita da última)
		CW_USEDEFAULT,		// Posição y pixels (default=abaixo da última)
		800,		// Largura da janela (em pixels)
		500,		// Altura da janela (em pixels)
		(HWND)HWND_DESKTOP,	// handle da janela pai (se se criar uma a partir de
		// outra) ou HWND_DESKTOP se a janela for a primeira, 
		// criada a partir do "desktop"
		(HMENU)NULL,			// handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst,		// handle da instância do programa actual ("hInst" é 
		// passado num dos parâmetros de WinMain()
		0);				// Não há parâmetros adicionais para a janela
	// ============================================================================
	// 4. Mostrar a janela
	// ============================================================================
	ShowWindow(hWnd, nCmdShow);	// "hWnd"= handler da janela, devolvido por 
	// "CreateWindow"; "nCmdShow"= modo de exibição (p.e. 
	// normal/modal); é passado como parâmetro de WinMain()
	UpdateWindow(hWnd);		// Refrescar a janela (Windows envia à janela uma 
	// mensagem para pintar, mostrar dados, (refrescar)… 
	// 
// ============================================================================
// 5. Loop de Mensagens
// ============================================================================
// O Windows envia mensagens às janelas (programas). Estas mensagens ficam numa fila de
// espera até que GetMessage(...) possa ler "a mensagem seguinte"	
// Parâmetros de "getMessage":
// 1)"&lpMsg"=Endereço de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no  
//   início de WinMain()):
//			HWND hwnd		handler da janela a que se destina a mensagem
//			UINT message		Identificador da mensagem
//			WPARAM wParam		Parâmetro, p.e. código da tecla premida
//			LPARAM lParam		Parâmetro, p.e. se ALT também estava premida
//			DWORD time		Hora a que a mensagem foi enviada pelo Windows
//			POINT pt		Localização do mouse (x, y) 
// 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
//   receber as mensagens para todas as
// janelas pertencentes à thread actual)
// 3)Código limite inferior das mensagens que se pretendem receber
// 4)Código limite superior das mensagens que se pretendem receber

// NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
// 	  terminando então o loop de recepção de mensagens, e o programa 


	HANDLE threadMensagem = CreateThread(NULL, 0, leitorMensagens, NULL, 0, NULL);
	if (leitorMensagens == NULL) {
		_tprintf(TEXT("[!] Erro ao criar a thread.\n"));
		return -1;
	}

	//InitialDialog = DialogBox(hThisInst, (LPCWSTR)IDD_STARTDIALOG, hWnd, (DLGPROC)StartWindow);





	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);	// Pré-processamento da mensagem (p.e. obter código 
		// ASCII da tecla premida)
		DispatchMessage(&lpMsg);	// Enviar a mensagem traduzida de volta ao Windows, que
		// aguarda até que a possa reenviar à função de 
		// tratamento da janela, CALLBACK TrataEventos (abaixo)
	}

	// ============================================================================
	// 6. Fim do programa
	// ============================================================================


	
	return((int)lpMsg.wParam);	// Retorna sempre o parâmetro wParam da estrutura lpMsg
}
void enviaMensagem(int comando)
{
	Mensagem_Sapo mensagem;
	DWORD n;
	//int buffer_size = _tcslen(mensagem.comando) * sizeof(TCHAR);
	//_tcscpy_s(mensagem.comando, buffer_size, frase);
	mensagem.cmd = comando;
	mensagem.id = 0;
	

	if (!WriteFile(hPipeEscrita, &mensagem, sizeof(mensagem), &n, NULL)) {
		_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
		CloseHandle(hPipeEscrita);
		PostQuitMessage(0);
	}
}
// ============================================================================
// FUNÇÃO DE PROCESSAMENTO DA JANELA
// Esta função pode ter um nome qualquer: Apenas é necesário que na inicialização da
// estrutura "wcApp", feita no início de // WinMain(), se identifique essa função. Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pré-processadas
// no loop "while" da função WinMain()
// Parâmetros:
//		hWnd	O handler da janela, obtido no CreateWindow()
//		messg	Ponteiro para a estrutura mensagem (ver estrutura em 5. Loop...
//		wParam	O parâmetro wParam da estrutura messg (a mensagem)
//		lParam	O parâmetro lParam desta mesma estrutura
//
// NOTA:Estes parâmetros estão aqui acessíveis o que simplifica o acesso aos seus valores
//
// A função EndProc é sempre do tipo "switch..." com "cases" que descriminam a mensagem
// recebida e a tratar.
// Estas mensagens são identificadas por constantes (p.e. 
// WM_DESTROY, WM_CHAR, WM_KEYDOWN, WM_PAINT...) definidas em windows.h
// ============================================================================
LRESULT CALLBACK TrataEventosModo(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	TCHAR username[DIM_TEXT];

	switch (messg)
	{
	case WM_COMMAND:

		// o LOWORD(wParam) traz o ID onde foi carregado 
		// se carregou no OK
		if (LOWORD(wParam) == IDJOGO_INDIVIDUAL)
		{
			// GetDlgItemText() vai à dialogbox e vai buscar o input do user
			GetDlgItemText(hWnd, IDC_EDIT_NOME_J, username, DIM_TEXT);
			MessageBox(hWnd, username, TEXT("Username"), MB_OK | MB_ICONINFORMATION);
			wcscpy_s(dados.nome, DIM_TEXT, username);
			EndDialog(hWnd, 0);


		}
		// se carregou no CANCEL
		else if (LOWORD(wParam) == IDJOGO_COMPETITIVO)
		{
			EndDialog(hWnd, 0);
			return TRUE;
		}

		break;

	case WM_CLOSE:
		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}
void bitmap(int left, int right, int top, int bot, HBITMAP hbit) {
	hdc = GetDC(hWnd);
	HDC auxmemdc = CreateCompatibleDC(hdc);
	SelectObject(auxmemdc, hbit);
	ReleaseDC(hWnd, hdc);
	BitBlt(memDC, left, top, right, bot, auxmemdc, 0, 0, SRCCOPY);
	DeleteDC(auxmemdc);
}
LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	int xPos, yPos;
	BOOL tracking = FALSE;
	static int mudar = 0;
	switch (messg) {
		HDC hdc;
		HANDLE listBox = NULL;

	case WM_CREATE:
		DialogBox(NULL, MAKEINTRESOURCE(IDD_MENU_NOME), hWnd, TrataEventosModo);
		hdc = GetDC(hWnd);
		memDC = CreateCompatibleDC(hdc);
		hbitEmpty = CreateCompatibleBitmap(hdc, 800, 650);
		SelectObject(memDC, hbitEmpty);
		hbitEmpty = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ROAD));

		hbitWall = CreateCompatibleBitmap(hdc, 800, 650);
		SelectObject(memDC, hbitWall);
		hbitWall = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_WALL));


		hbitFrog = CreateCompatibleBitmap(hdc, 800, 650);
		SelectObject(memDC, hbitFrog);
		hbitFrog = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_FROG));

		hbitCar = CreateCompatibleBitmap(hdc, 800, 650);
		SelectObject(memDC, hbitCar);
		hbitCar = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CAR));

		hbitBus = CreateCompatibleBitmap(hdc, 800, 650);
		SelectObject(memDC, hbitBus);
		hbitBus = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUS));

		hbitGrass = CreateCompatibleBitmap(hdc, 900, 650);
		SelectObject(memDC, hbitGrass);
		hbitGrass = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_GRASS));
		
		hbrush = CreateSolidBrush(RGB(144, 238, 144));
		SelectObject(memDC, hbrush);
		PatBlt(memDC, 0, 0, 800, 650, PATCOPY);

		ReleaseDC(hWnd, hdc);
		
		break;
	case WM_LBUTTONDOWN:
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		//cima
		if ((dados.sapo.pos_x < xPos && xPos < dados.sapo.pos_x + 30) && (dados.sapo.pos_y -50 < yPos && yPos < dados.sapo.pos_y - 10))
		{
			enviaMensagem(UP);
		}
		if ((dados.sapo.pos_x < xPos && xPos < dados.sapo.pos_x + 30) && (dados.sapo.pos_y + 20 < yPos && yPos < dados.sapo.pos_y + 50))
		{
			enviaMensagem(DOWN);
		}
		if ((dados.sapo.pos_x +20 < xPos && xPos < dados.sapo.pos_x + 50) && (dados.sapo.pos_y < yPos && yPos < dados.sapo.pos_y + 30))
		{
			enviaMensagem(RIGHT);
		}
		if ((dados.sapo.pos_x -50< xPos && xPos < dados.sapo.pos_x -10) && (dados.sapo.pos_y  < yPos && yPos < dados.sapo.pos_y + 30))
		{
			enviaMensagem(LEFT);
		}
		
		break;
	case WM_RBUTTONDOWN:
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		if ((dados.sapo.pos_x < xPos && xPos < dados.sapo.pos_x + 30) && (dados.sapo.pos_y < yPos && yPos < dados.sapo.pos_y + 30))
		{
			/*
			TCHAR mensagem_c[TAM];
			int a = 0;
			a = swprintf_s(mensagem_c, TAM, L"%s", L"Sapo passou:  ");
			a += swprintf_s(mensagem_c + a, TAM - a, L"%s", dados.pontuacao);
			a += swprintf_s(mensagem_c + a, TAM - a, L"%s", L" vezes.");
			MessageBox(hWnd, mensagem_c, TEXT("Sapo"), MB_OK);*/
			enviaMensagem(REGRESSO);
		}
		break;
	case WM_MOUSEMOVE:
		if (!tracking) {
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.hwndTrack = hWnd;
			tme.dwHoverTime = 100;
			tracking = TrackMouseEvent(&tme);
		}
		break;
	case WM_MOUSEHOVER:
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		if ((dados.sapo.pos_x < xPos && xPos < dados.sapo.pos_x + 30) && (dados.sapo.pos_y < yPos && yPos < dados.sapo.pos_y + 30))
		{
			
			TCHAR mensagem_c[TAM];
			int a = 0;
			a = swprintf_s(mensagem_c, TAM, L"%s", L"Sapo passou:  ");
			a += swprintf_s(mensagem_c + a, TAM - a, L"%s", dados.pontuacao);
			a += swprintf_s(mensagem_c + a, TAM - a, L"%s", L" vezes.");
			MessageBox(hWnd, mensagem_c, TEXT("Sapo"), MB_OK);
		}
		tracking = FALSE; // tracking is cancelled
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hWnd;
		tracking = TrackMouseEvent(&tme);

		break;
	case WM_MOUSELEAVE:
		tracking = FALSE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_MUDAR_ITEMS:
			if (mudar == 0)
			{
				hbitEmpty = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ROAD2));
				hbitWall = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_WALL2));
				hbitFrog = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_FROG2));
				hbitCar = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CAR2));
				hbitBus = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUS2));
				hbitGrass = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_GRASS2));
				mudar = 1;
			}
			else
			{
				hbitEmpty = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ROAD));
				hbitWall = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_WALL));
				hbitFrog = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_FROG));
				hbitCar = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CAR));
				hbitBus = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUS));
				hbitGrass = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_GRASS));
				mudar = 0;
			}
			break;

		}
		break;

	case WM_KEYDOWN:
	{
		switch (wParam) {
		case VK_LEFT:
			enviaMensagem(LEFT);
			break;

		case VK_RIGHT:
			enviaMensagem(RIGHT);
			break;

		case VK_UP:
			enviaMensagem(UP);
			break;

		case VK_DOWN:
			enviaMensagem(DOWN);
			break;
		case VK_SPACE:
			//data.type = SHOT;
			break;
		case VK_ESCAPE:
			MessageBox(hWnd, TEXT("ESC"), TEXT("..."), MB_OK);
			break;
		default:
			if (wParam == TEXT('C'))
				//data.type = JOIN_GAME;
				break;
		}
		//sendCommand(data);
		break;

	}

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &rect);
		if (memDC == NULL) {
			memDC = CreateCompatibleDC(hdc);
			hBitmapDB = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			SelectObject(memDC, hBitmapDB);
			DeleteObject(hBitmapDB);
		}
		if (dados.nome != NULL)
			TextOut(memDC, 600, 10, dados.nome, _tcslen(dados.nome));
		TextOut(memDC, 600, 30, TEXT("Pontuação:"), strlen("Pontuação:"));
		if(dados.pontuacao!=NULL)
			TextOut(memDC, 690, 30, dados.pontuacao, _tcslen(dados.pontuacao));
		TextOut(memDC, 600, 50, TEXT("Tempo:"), strlen("Tempo:"));
		if (dados.tempo != NULL)
			TextOut(memDC, 690, 50, dados.tempo, _tcslen(dados.tempo));
		BitBlt(hdc, 0, 0, 800, 500, memDC, 0, 0, SRCCOPY);

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:	// Destruir a janela e terminar o programa 
		//enviaMensagem(NULL, SAIR);
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		enviaMensagem(SAIR);
		PostQuitMessage(0);
		break;
	default:
		// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar")
		// não é efectuado nenhum processamento, apenas se segue o "default" do Windows
		return(DefWindowProc(hWnd, messg, wParam, lParam));
		break;  // break tecnicamente desnecessário por causa do return
	}
	return(0);
}



void RefreshMap(Jogo jogo) {

	hdc = GetDC(hWnd);
	WaitForSingleObject(hMutexPaint, INFINITE);
	int x = 10, y = 10;
	for (int l = 0; l <= jogo.dim_max; l++) {
		for (int c = 0; c < NUMERO_COL; c++) {
			switch (jogo.mapa[l].linha[c]) {
			case 'X':
				bitmap(x, x + 30, y, y + 30, hbitEmpty);
				break;
			case 'C':
				bitmap(x, x + 30, y, y + 30, hbitCar);
				break;
			case 'S':
				bitmap(x, x + 30, y, y + 30, hbitFrog);
				dados.sapo.pos_x = x;
				dados.sapo.pos_y = y;
				break;
			case 'L':
				bitmap(x, x + 30, y, y + 30, hbitBus);
				break;
			case 'T':
				bitmap(x, x + 30, y, y + 30, hbitWall);
				break;
			case ' ':
				bitmap(x, x + 30, y, y + 50, hbitGrass);
			}
			x += 20;
		}
		x = 10;
		y += 20;
	}
	ReleaseDC(hWnd, memDC);
	InvalidateRect(hWnd, NULL, TRUE);
	ReleaseMutex(hMutexPaint, INFINITE);
}
	DWORD WINAPI leitorMensagens(LPVOID lpParam) {
		HANDLE hPipe;
		Mensagem_Sapo mensagem;
		int i = 0;
		DWORD n;

		_tprintf(TEXT("[SAPO] Esperar pelo pipe '%s' (WaitNamedPipe)\n"),
			PIPE_NAME);
		if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_USE_DEFAULT_WAIT)) {
			_tprintf(TEXT("[SAPO][ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
			exit(-1);
		}
		_tprintf(TEXT("[SAPO] Ligação ao pipe do servidor... (CreateFile)\n"));
		hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (hPipe == NULL) {
			_tprintf(TEXT("[SAPO][ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
			exit(-1);
		}
		_tprintf(TEXT("[LEITOR] Liguei-me...\n"));

		//ligou se 
		
		hPipeEscrita = CreateFile(PIPE_NAME_R, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (hPipeEscrita == NULL) {
			_tprintf(TEXT("[SAPO][ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME_R);
			exit(-1);
		}
		int fim = 0;
		

		while (fim!=5) {//MUDAR INSTANCIA DE FIM
			if (!ReadFile(hPipe, &mensagem, sizeof(mensagem), &n, NULL))
			{
				fim = 5;
				//MessageBox(hWnd, TEXT("TESTE"), TEXT("FIM"), MB_OK);
			}
				
			_tprintf(TEXT("[SAPO] Recebi %d bytes: '%s'... (ReadFile)\n"), n, mensagem.comando);
			

	
			//ATUALIZAR MAPA
			dados.jogo = jogo;
			swprintf_s(dados.pontuacao, DIM_TEXT, L"%d", mensagem.jogo.sapo[0].pontuacao);
			if(mensagem.jogo.tempo>0)
				swprintf_s(dados.tempo, DIM_TEXT, L"%d", mensagem.jogo.tempo);
			RefreshMap(mensagem.jogo);
			if (mensagem.jogo.vitoria == 1)
			{
				TCHAR mensagem_v[TAM];
				int a = 0;
				a= swprintf_s(mensagem_v, TAM, L"%s", L"Parabens vitoria com ");
				a+= swprintf_s(mensagem_v+a, TAM-a, L"%s", dados.pontuacao);
				a+=swprintf_s(mensagem_v+a, TAM-a, L"%s", L" pontos.");
				enviaMensagem(SAIR);
				MessageBox(hWnd, mensagem_v, TEXT("Vitoria"), MB_OK);
				PostThreadMessage(thread, WM_QUIT, 0, 0);
			}
			//InvalidateRect(janelaglobal, NULL, 0);
			
		}
		CloseHandle(hPipe);
		MessageBox(hWnd, TEXT("Servidor Desconectado"), TEXT("Servidor Desconectado"), MB_OK);
		//PostQuitMessage(0);
		PostThreadMessage(thread, WM_QUIT,0,0);
		return 0;
	}
