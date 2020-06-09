// ConnectFourOnline.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/*
	Author: Alfredo Sepulveda Van Hoorde
	https://github.com/alfredo-svh/ConnectFourOnline

	License
	~~~~~~~
	Copyright (C) 2020  Alfredo Sepulveda Van Hoorde

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.


	This program comes with ABSOLUTELY NO WARRANTY.
	This is free software, and you are welcome to redistribute it
	under certain conditions; See license for details.
	Original work located at:
	https://github.com/alfredo-svh/ConnectFourOnline
	GNU GPLv3
	https://github.com/alfredo-svh/ConnectFourOnline/blob/master/LICENSE.md
	From alfredo-svh
	~~~~~~~~~~~~~~~
	Hi there! This is program and its entire code is free software. You can take it, use it,
	distribute it, change it. 
	You acknowledge	that I am not responsible for anything bad that happens as a result of
	your actions. However this code is protected by GNU GPLv3, see the license in the
	github repo. This means you must attribute me if you use it. You can view this
	license here: https://github.com/alfredo-svh/ConnectFourOnline/blob/master/LICENSE.md

	Background
	~~~~~~~~~~
	I originally made this game without any online components, but decided it was not
	fun to play against myself.

	Controls are 1,2,3,4,5,6,7 to select the column where you wish to place your token.
	You can press the Esc key to stop the game and end the connection at any time.
	To win, you need to align 4 tokens horizontally, vertically or diagonally.
	DO NOT try to resize the window. It will bring chaos. (Perhaps you can fix this?)

	To compile, create a new Windows Console application on Visual Studio.
	Make sure to define WIN32_LEAN_AND_MEAN inside pch.h


	Future Modifications
	~~~~~~~~~~~~~~~~~~~~
	1) Refactor some of the code
	2) Making screen resize work
	3) Introduce "play again?" loop

	Author
	~~~~~~
	Alfredo Sepulveda Van Hoorde
	Github: https://github.com/alfredo-svh/
	Email: asepulvedavanhoorde@alumni.scu.edu

	Last Updated: June 8th, 2020
*/

#include "pch.h"
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <Windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 1
#define DEFAULT_PORT "4433"
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

using namespace std;

int nFieldWidth = 17;
int nFieldHeight = 15;
unsigned char *pField = nullptr;

int nScreenWidth = 80; //80
int nScreenHeight = 30; //30

bool bServer;
bool bTurn = 0;
WORD wColor[2];
vector<vector<int>> vGrid = { {-1,-1,-1,-1,-1,-1, -1}, {-1,-1,-1,-1,-1,-1, -1}, {-1,-1,-1,-1,-1,-1, -1}, {-1,-1,-1,-1,-1,-1, -1}, {-1,-1,-1,-1,-1,-1, -1}, {-1,-1,-1,-1,-1,-1, -1} };

HANDLE hConsole;

WSADATA wsaData;

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;
SOCKET ConnectSocket = INVALID_SOCKET;

int iResult;
int iSendResult;
char recvbuf[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

struct addrinfo *result = NULL, hints;


bool checkForWin(int color) {
	int cur = 0;

	//horizontal
	for (int py = 0; py < 6; py++) {
		cur = 0;
		for (int px = 0; px < 7; px++) {
			if (vGrid[py][px] == color) {
				cur++;
				if (cur == 4) {
					return true;
				}
			}
			else {
				cur = 0;
			}
		}
	}

	//vertical
	for (int px = 0; px < 7; px++) {
		cur = 0;
		for (int py = 0; py < 6; py++) {
			if (vGrid[py][px] == color) {
				cur++;
				if (cur == 4) {
					return true;
				}
			}
			else {
				cur = 0;
			}
		}
	}

	//diagonalBL2TR
	for (int py = 0; py < 3; py++) {
		cur = 0;
		for (int px = 0, y = py; px < 7 && y < 6; px++, y++) {
			if (vGrid[y][px] == color) {
				cur++;
				if (cur == 4) {
					return true;
				}
			}
			else {
				cur = 0;
			}
		}
	}
	for (int px = 1; px < 4; px++) {
		cur = 0;
		for (int x = px, py = 0; x < 7 && py < 6; py++, x++) {
			if (vGrid[py][x] == color) {
				cur++;
				if (cur == 4) {
					return true;
				}
			}
			else {
				cur = 0;
			}
		}
	}

	//digaonalBR2TL
	for (int py = 0; py < 3; py++) {
		cur = 0;
		for (int y = py, px = 6; y < 6 && px >= 0; px--, y++) {
			if (vGrid[y][px] == color) {
				cur++;
				if (cur == 4) {
					return true;
				}
			}
			else {
				cur = 0;
			}
		}
	}
	for (int px = 5; px >= 3; px--) {
		cur = 0;
		for (int py = 0, x = px; x >= 0 && py < 6; py++, x--) {
			if (vGrid[py][x] == color) {
				cur++;
				if (cur == 4) {
					return true;
				}
			}
			else {
				cur = 0;
			}
		}
	}

	return false;
}


bool insertToken(int column) {
	for (int i = 0; i < 6; i++) {
		if (vGrid[i][column] < 0) {
			vGrid[i][column] = bTurn;
			return true;
		}
	}
	return false;
}


void playGame() {
	/* Create playing field */

	pField = new unsigned char[nFieldWidth * nFieldHeight];

	//board
	for (int px = 0; px < nFieldWidth; px++) {
		for (int py = 0; py < nFieldHeight; py++) {
			pField[py*nFieldWidth + px] = (px % 2 == 1) ? 4 : 0;	// |
			pField[py*nFieldWidth + px] = (py % 2 == 1) ? 3 : pField[py*nFieldWidth + px]; // -
			pField[py*nFieldWidth + px] = (py % 2 == 1 && px % 2 == 1) ? 6 : pField[py*nFieldWidth + px]; //+
			pField[py*nFieldWidth + px] = (px == 0 || px == 1 || px == nFieldWidth - 1 || px == nFieldWidth - 2 || py == nFieldHeight - 1 || py == nFieldHeight - 2) ? 5 : pField[py*nFieldWidth + px]; // #
			pField[py*nFieldWidth + px] = (py == 0) ? 0 : pField[py*nFieldWidth + px]; // clear top row
		}
	}
	for (int i = 0; i < 7; i++) {
		pField[2 + 2 * i] = i + 7;
	}

	wchar_t *screen = new wchar_t[nScreenWidth*nScreenHeight];
	for (int i = 0; i < nScreenWidth*nScreenHeight; i++) {
		screen[i] = L' ';
	}
	hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	/* RENDER */
	//Draw field
	for (int i = 0; i < nFieldWidth; i++) {
		for (int j = 0; j < nFieldHeight; j++) {
			screen[(j + 2)*nScreenWidth + (i + 2)] = L" BR-|#+1234567"[pField[j*nFieldWidth + i]];
		}
	}
	//Draw Turn
	if (bTurn) {
		if (bServer)
			swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 23, L"OPPONENT'S TURN (RED) ");
		else
			swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 23, L"YOUR TURN (RED)       ");
	}
	else {
		if (bServer)
			swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 23, L"YOUR TURN (BLUE)      ");
		else
			swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 23, L"OPPONENT'S TURN (BLUE)");
	}

	//Display frame
	WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth*nScreenHeight, { 0,0 }, &dwBytesWritten);


	/* game logic */

	wColor[0] = FOREGROUND_BLUE | BACKGROUND_BLUE;
	wColor[1] = FOREGROUND_RED | BACKGROUND_RED;

	bool bGameOver = false;
	int nWin = -1;
	bool bKey[8];
	short nColumn;
	int nTokenInScreen[7] = { nFieldHeight - 3,nFieldHeight - 3, nFieldHeight - 3, nFieldHeight - 3, nFieldHeight - 3, nFieldHeight - 3, nFieldHeight - 3 };
	int nSpacesLeft = 42;

	//flush input buffer
	for (int k = 0; k < 8; k++) {
		bKey[k] = (1 & GetAsyncKeyState((unsigned char)("1234567\x1B"[k]))) != 0;
	}


	/* main game loop */

	while (!bGameOver) {
		/* INPUT */
		while (1) {
			//YOUR TURN
			if ((bTurn && bServer) || !bTurn && !bServer) {
				this_thread::sleep_for(1000ms);
				for (int k = 0; k < 8; k++) {
					bKey[k] = (1 & GetAsyncKeyState((unsigned char)("1234567\x1B"[k]))) != 0;
				}

				if (bKey[0]) {
					iResult = send((bServer)? ClientSocket : ConnectSocket, "0", 1, 0);
					if (insertToken(0)) {
						nColumn = 0;
						break;
					}
				}
				else if (bKey[1]) {
					iResult = send((bServer) ? ClientSocket : ConnectSocket, "1", 1, 0);
					if (insertToken(1)) {
						nColumn = 1;
						break;
					}
				}
				else if (bKey[2]) {
					iResult = send((bServer) ? ClientSocket : ConnectSocket, "2", 1, 0);
					if (insertToken(2)) {
						nColumn = 2;
						break;
					}
				}
				else if (bKey[3]) {
					iResult = send((bServer) ? ClientSocket : ConnectSocket, "3", 1, 0);
					if (insertToken(3)) {
						nColumn = 3;
						break;
					}
				}
				else if (bKey[4]) {
					iResult = send((bServer) ? ClientSocket : ConnectSocket, "4", 1, 0);
					if (insertToken(4)) {
						nColumn = 4;
						break;
					}
				}
				else if (bKey[5]) {
					iResult = send((bServer) ? ClientSocket : ConnectSocket, "5", 1, 0);
					if (insertToken(5)) {
						nColumn = 5;
						break;
					}
				}
				else if (bKey[6]) {
					iResult = send((bServer) ? ClientSocket : ConnectSocket, "6", 1, 0);
					if (insertToken(6)) {
						nColumn = 6;
						break;
					}
				}
				else if (bKey[7]) {
					return;
				}
			}
			else {
				//OPPONENT'S TURN
				iResult = recv((bServer)? ClientSocket : ConnectSocket, recvbuf, recvbuflen, 0);
				if (iResult > 0) {
					nColumn = stoi(recvbuf);
					if (insertToken(nColumn)) {
						break;
					}
				}
				else if (iResult == 0)
					return;
			}
		}


		/* GAME LOGIC */

		WriteConsoleOutputAttribute(hConsole, (bTurn) ? &wColor[1] : &wColor[0], 1, { nColumn * 2 + 4,  (short)nTokenInScreen[nColumn] + 2 }, &dwBytesWritten);
		pField[nTokenInScreen[nColumn] * nFieldWidth + nColumn * 2 + 2] = (bTurn) ? 2 : 1;
		nTokenInScreen[nColumn] -= 2;

		//check for Game Over
		nSpacesLeft--;

		if (checkForWin(bTurn)) {
			nWin = bTurn;
			bGameOver = true;
		}
		else if (nSpacesLeft == 0) {
			bGameOver = true;
		}
		else {
			//change turn
			bTurn = !bTurn;
		}

		/* RENDER */
		//Draw field
		for (int i = 0; i < nFieldWidth; i++) {
			for (int j = 0; j < nFieldHeight; j++) {
				screen[(j + 2)*nScreenWidth + (i + 2)] = L" BR-|#+1234567"[pField[j*nFieldWidth + i]];
			}
		}
		if (bGameOver) {
			//Draw Game Over
			switch (nWin) {
			case(-1):
				swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 23, L"TIE! NO WINNER!       ");
				break;
			case(0):
				if(bServer)
					swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 26, L"CONGRATULATIONS! YOU WON!");
				else
					swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 31, L"YOU LOST! BETTER LUCK NEXT TIME");
				break;
			case(1):
				if (!bServer)
					swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 26, L"CONGRATULATIONS! YOU WON!");
				else
					swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 31, L"YOU LOST! BETTER LUCK NEXT TIME");
				break;
			};
			swprintf_s(&screen[4 * nScreenWidth + nFieldWidth + 6], 20, L"PRESS ENTER TO QUIT");
		}
		else {
			//Draw Turn
			if (bTurn) {
				if (bServer)
					swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 23, L"OPPONENT'S TURN (RED) ");
				else
					swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 23, L"YOUR TURN (RED)       ");
			}
			else {
				if (bServer)
					swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 23, L"YOUR TURN (BLUE)      ");
				else
					swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 23, L"OPPONENT'S TURN (BLUE)");
			}
		}

		//Display frame
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth*nScreenHeight, { 0,0 }, &dwBytesWritten);

		if (bGameOver) {
			bool bEnter = 0;
			while (1) {
				this_thread::sleep_for(1000ms);
				bEnter = (1 & GetAsyncKeyState((unsigned char)('\x0D'))) != 0;
				if (bEnter) {
					break;
				}
			}
		}
	}

}


int main() {
	cout << "Connect Four Online Copyright(C) 2020 Alfredo Sepulveda Van Hoorde" << endl;
	cout << "This program comes with ABSOLUTELY NO WARRANTY" << endl;
	cout << "This is free software, and you are welcome to redistribute it" << endl;
	cout << "under certain conditions; see LICENSE.md for details" << endl << endl<<endl;


	string input = "";
	cout << "\t\t\tWELCOME TO CONNECT FOUR ONLINE!" << endl<<endl;

	cout << "To become a server, press 1 (and Enter)" << endl;
	cout << "To join a friend, press 2 (and Enter)" << endl;

	cin >> input;
	cout << endl;

	if (input == "1") {
		bServer = true;

		PIP_ADAPTER_INFO pAdapterInfo;
		PIP_ADAPTER_INFO pAdapter = NULL;
		DWORD dwRetVal = 0;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed with error: %d\n", iResult);
			system("pause");
			return 1;
		}

		//std::cout << "Socket initialized" << std::endl;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the server address and port
		iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			system("pause");
			return 1;
		}

		//std::cout << "Server address and port resolved" << std::endl;

		// Create a SOCKET for connecting to server
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			system("pause");
			return 1;
		}

		//std::cout << "Listen Socket created" << std::endl;

		// Setup the TCP listening socket
		iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(ListenSocket);
			WSACleanup();
			system("pause");
			return 1;
		}

		//std::cout << "Listen Socket setup successful" << std::endl;

		freeaddrinfo(result);

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			system("pause");
			return 1;
		}

		//std::cout << "Socket listening" << std::endl;

		ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
		pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(sizeof(IP_ADAPTER_INFO));
		if (pAdapterInfo == NULL) {
			printf("Error allocating memory needed to call GetAdaptersinfo\n");
			system("pause");
			return 1;
		}
		// Make an initial call to GetAdaptersInfo to get
		// the necessary size into the ulOutBufLen variable
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
			FREE(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(ulOutBufLen);
			if (pAdapterInfo == NULL) {
				printf("Error allocating memory needed to call GetAdaptersinfo\n");
				system("pause");
				return 1;
			}
		}

		if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
			std::cout << "Your IP Address is:" << std::endl;
			pAdapter = pAdapterInfo;
			while (pAdapter) {
				std::string ip = pAdapter->IpAddressList.IpAddress.String;
				if (ip != "0.0.0.0") {
					std::cout << ip << std::endl;
				}

				pAdapter = pAdapter->Next;
			}
			std::cout << "Share it with your friend!" << std::endl<< std::endl;
		}
		else {
			printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
		}
		if (pAdapterInfo)
			FREE(pAdapterInfo);

		cout << "Waiting for your friend..." << endl;

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			system("pause");
			return 1;
		}

		//std::cout << "Client accepted" << std::endl;

		// No longer need server socket
		closesocket(ListenSocket);
	}
	else if (input == "2") {
		bServer = false;

		struct addrinfo *ptr = NULL;
		std::string hostName;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed with error: %d\n", iResult);
			system("pause");
			return 1;
		}

		//std::cout << "Socket initialized" << std::endl;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		std::cout << "Enter the IP address of your friend:" << std::endl;
		std::cin >> hostName;
		const char *hostAddr = hostName.c_str();

		// Resolve the server address and port
		iResult = getaddrinfo(hostAddr, DEFAULT_PORT, &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			system("pause");
			return 1;
		}
		//std::cout << "Server address and port resolved" << std::endl;

		// Attempt to connect to an address until one succeeds
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

			// Create a SOCKET for connecting to server
			ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
				ptr->ai_protocol);
			if (ConnectSocket == INVALID_SOCKET) {
				printf("socket failed with error: %ld\n", WSAGetLastError());
				WSACleanup();
				system("pause");
				return 1;
			}

			std::cout << "Trying to connect to your friend..." << std::endl;

			// Connect to server.
			iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR) {
				closesocket(ConnectSocket);
				ConnectSocket = INVALID_SOCKET;
				continue;
			}
			break;
		}

		freeaddrinfo(result);

		if (ConnectSocket == INVALID_SOCKET) {
			printf("Unable to connect to server!\n");
			WSACleanup();
			system("pause");
			return 1;
		}

		//std::cout << "Connected to server" << std::endl;
	}
	else {
		return 1;
	}

	playGame();

	// Game Over
	CloseHandle(hConsole);

	// shutdown the connection since no more data will be sent
	iResult = shutdown((bServer)? ClientSocket : ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	//std::cout << "Connection was shut" << std::endl;

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}
