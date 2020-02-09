#include <iostream>
#include <thread>
#include <vector>
using namespace std;

#include <stdio.h>
#include <Windows.h>

const int c_NumOfRotations = 4;
const int c_NumOfInputs = 4;
const int c_ScreenOffset = 2;
const int c_CharacterX_Unicode = 88;

int nScreenWidth = 80;			// Console Screen Size X (columns)
int nScreenHeight = 30;			// Console Screen Size Y (rows)
int nFieldWidth = 12;
int nFieldHeight = 18;

wstring tetromino[7];
unsigned char* pField = nullptr;

int Rotate(int px, int py, int r)
{
	int pi = 0;
	
	switch (r % c_NumOfRotations)
	{
	case 0: // 0 degrees							// 0  1  2  3
		pi = py * c_NumOfRotations + px;			// 4  5  6  7
		break;										// 8  9 10 11
													//12 13 14 15

	case 1: // 90 degrees							//12  8  4  0
		pi = 12 + py - (px * c_NumOfRotations);		//13  9  5  1
		break;										//14 10  6  2
													//15 11  7  3

	case 2: // 180 degrees							//15 14 13 12
		pi = 15 - (py * c_NumOfRotations) - px;		//11 10  9  8
		break;										// 7  6  5  4
													// 3  2  1  0

	case 3: // 270 degrees							// 3  7 11 15
		pi = 3 - py + (px * c_NumOfRotations);		// 2  6 10 14
		break;										// 1  5  9 13
	}												// 0  4  8 12

	return pi;
}

bool DoesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY)
{
	// All Field cells > 0 are occupied
	for (int px = 0; px < c_NumOfRotations; px++)
	{
		for (int py = 0; py < c_NumOfRotations; py++)
		{
			// Get index into piece
			int pi = Rotate(px, py, nRotation);

			// Get index into field
			int fi = (nPosY + py) * nFieldWidth + (nPosX + px);

			// Check that test is in bounds. Note out of bounds does
			// not necessarily mean a fail, as the long vertical piece
			// can have cells that lie outside the boundary, so we'll
			// just ignore them
			if (nPosX + px >= 0 && nPosX + px < nFieldWidth)
			{
				if (nPosY + py >= 0 && nPosY + py < nFieldHeight)
				{
					// In Bounds so do collision check
					if (tetromino[nTetromino][pi] != L'.' && pField[fi] != 0)
					{
						return false; // fail on first hit
					}
				}
			}
		}
	}

	return true;
}

int main()
{
	// Create Screen Buffer
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	for (int i = 0; i < nScreenWidth * nScreenHeight; i++) screen[i] = L' ';
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	// Tetronimos 4x4
	tetromino[0].append(L"..X...X...X...X.");
	tetromino[1].append(L"..X..XX...X.....");
	tetromino[2].append(L".....XX..XX.....");
	tetromino[3].append(L"..X..XX..X......");
	tetromino[4].append(L".X...XX...X.....");
	tetromino[5].append(L".X...X...XX.....");
	tetromino[6].append(L"..X...X..XX.....");
	
	// Create play field buffer
	pField = new unsigned char[nFieldWidth * nFieldHeight]; 
	
	// Board Boundary
	for (int x = 0; x < nFieldWidth; x++)
	{
		for (int y = 0; y < nFieldHeight; y++)
		{
			pField[y * nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 9 : 0;
		}
	}

	// Game Logic
	bool bKey[c_NumOfInputs];
	bool bGameOver = false;
	bool bForceDown = false;
	bool bRotateHold = true;

	int nCurrentPiece = 0;
	int nCurrentRotation = 0;
	int nCurrentX = nFieldWidth / 2;
	int nCurrentY = 0;
	int nSpeed = 20;
	int nSpeedCount = 0;
	int nPieceCount = 0;
	int nScore = 0;

	vector<int> vLines;

	while (!bGameOver) // Main Loop
	{
		// Timing =======================
		this_thread::sleep_for(50ms); // Small Step = 1 Game Tick
		nSpeedCount++;
		bForceDown = (nSpeedCount == nSpeed);

		// Input ========================
		for (int k = 0; k < c_NumOfInputs; k++)
		{
			// Left Key		- x25
			// Right Key	- x27
			// Down Key		- x28
			// Z Key		- Z
			bKey[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;
		}

		// Game Logic ===================

		// Handle player movement
		nCurrentX += (bKey[0] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY)) ? 1 : 0; // Right Key
		nCurrentX -= (bKey[1] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY)) ? 1 : 0; // Left Key
		nCurrentY += (bKey[2] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)) ? 1 : 0; // Down Key

		// Rotate, but latch to stop wild spinning
		if (bKey[3])
		{
			nCurrentRotation += (bRotateHold && DoesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY)) ? 1 : 0;
			bRotateHold = false;
		}
		else
		{
			bRotateHold = true;
		}

		// Force the piece down the playfield if it's time
		if (bForceDown)
		{
			// Update difficulty every 50 pieces
			nSpeedCount = 0;
			nPieceCount++;

			if ( (nPieceCount % 50 == 0) && (nSpeed >= 10) )
			{
				nSpeed--;
			}

			// Test if piece can be moved down
			if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1))
			{
				// It can, so do it!
				nCurrentY++;
			}
			else
			{
				// It can't! Lock the piece in place
				for (int px = 0; px < c_NumOfRotations; px++)
				{
					for (int py = 0; py < c_NumOfRotations; py++)
					{
						if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
						{
							pField[(nCurrentY + py) * nFieldWidth + (nCurrentX + px)] = nCurrentPiece + 1;
						}
					}
				}

				// Check for lines
				for (int py = 0; py < 4; py++)
				{
					if (nCurrentY + py < nFieldHeight - 1)
					{
						bool bLine = true;
						for (int px = 1; px < nFieldWidth - 1; px++)
						{
							bLine &= (pField[(nCurrentY + py) * nFieldWidth + px]) != 0;
						}

						if (bLine)
						{
							// Remove Line, set to =
							for (int px = 1; px < nFieldWidth - 1; px++)
								pField[(nCurrentY + py) * nFieldWidth + px] = 8;
							vLines.push_back(nCurrentY + py);
						}
					}
				}

				nScore += 25;

				if (!vLines.empty())
				{
					nScore += (1 << vLines.size()) * 100;
				}

				// Pick New Piece
				nCurrentX = nFieldWidth / 2;
				nCurrentY = 0;
				nCurrentRotation = 0;
				nCurrentPiece = rand() % 7;

				// If piece does not fit straight away, game over!
				bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);
			}
		}

		// Display ======================

		// Draw Field
		for (int x = 0; x < nFieldWidth; x++)
		{
			for (int y = 0; y < nFieldHeight; y++)
			{
				int screenIndex = (y + c_ScreenOffset) * nScreenWidth + (x + c_ScreenOffset);
				int stringIndex = pField[y * nFieldWidth + x];

				if (y == nFieldHeight - 1)
				{
					stringIndex++;
				}

				//screen[screenIndex] = L" ABCDEFG-|~"[stringIndex];
				screen[screenIndex] = L" XXXXXXX-|~"[stringIndex];
			}
		}

		// Draw Current Piece
		for (int px = 0; px < 4; px++)
		{
			for (int py = 0; py < 4; py++)
			{
				if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
				{
					screen[(nCurrentY + py + 2) * nScreenWidth + (nCurrentX + px + 2)] = nCurrentPiece + 65;
					screen[(nCurrentY + py + 2) * nScreenWidth + (nCurrentX + px + 2)] = c_CharacterX_Unicode;
				}
			}
		}

		// Draw Score
		swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 16, L"SCORE: %8d", nScore);

		// Animate Line Completion
		if (!vLines.empty())
		{
			// Display Frame (cheekily to draw lines)
			WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
			this_thread::sleep_for(400ms); // Delay a bit

			for (auto& v : vLines)
			{
				for (int px = 1; px < nFieldWidth - 1; px++)
				{
					for (int py = v; py > 0; py--)
					{
						pField[py * nFieldWidth + px] = pField[(py - 1) * nFieldWidth + px];
					}

					pField[px] = 0;
				}
			}

			vLines.clear();
		}

		// Display Frame
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	// Oh Dear
	CloseHandle(hConsole);
	cout << "Game Over!!\n\nScore:" << nScore << endl << endl;
	system("pause");
	return 0;
}