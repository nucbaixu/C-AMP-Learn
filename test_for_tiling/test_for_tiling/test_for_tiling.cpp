// test_for_tiling.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <amp.h>
using namespace concurrency;

const int ROWS = 8;
const int COLS = 9;


// tileRow and tileColumn specify the tile that each thread is in.
// globalRow and globalColumn specify the location of the thread in the array_view.
// localRow and localColumn specify the location of the thread relative to the tile.
struct Description {
	int value;
	int tileRow;
	int tileColumn;
	int globalRow;
	int globalColumn;
	int localRow;
	int localColumn;
};

// A helper function for formatting the output.
void SetConsoleColor(int color) 
{
	int colorValue = (color == 0) ? 4 : 2;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorValue);
}


// A helper function for formatting the output.
void SetConsoleSize(int height, int width) 
{
	COORD coord; 
	coord.X = width;
	coord.Y = height;

	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coord);
	SMALL_RECT* rect = new SMALL_RECT();
	rect->Left = 0; rect->Top = 0; rect->Right = width; rect->Bottom = height;
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), true, rect);
}


// This method creates an 8x9 matrix of Description structures. In the call to parallel_for_each, the structure is updated 
// with tile, global, and local indices.
void TilingDescription()
{
	// Create 72 (8x9) Description structures.
	std::vector<Description> descs;
	for (int i = 0; i < ROWS * COLS; i++)
	{
		Description d = {i, 0, 0, 0, 0, 0, 0};
		descs.push_back(d);
	}


	// Create an array_view from the Description structures.
	extent<2> matrix(ROWS, COLS);
	
	array_view<Description, 2> descriptions(matrix, descs);

	// Update each Description with the tile, global, and local indices.
	parallel_for_each(descriptions.extent.tile< 2, 3>(),
		[=] (tiled_index< 2, 3> t_idx) restrict(amp) 
	{
		descriptions[t_idx].globalRow = t_idx.global[0];
		descriptions[t_idx].globalColumn = t_idx.global[1];
		descriptions[t_idx].tileRow = t_idx.tile[0];
		descriptions[t_idx].tileColumn = t_idx.tile[1];
		descriptions[t_idx].localRow = t_idx.local[0];
		descriptions[t_idx].localColumn= t_idx.local[1];
	});
	
	
	// Print out the Description structure for each element in the matrix.
	// Tiles are displayed in red and green to distinguish them from each other.
	SetConsoleSize(100, 150);
	for (int row = 0; row < ROWS; row++) 
	{
		for (int column = 0; column < COLS; column++)
		{
			SetConsoleColor((descriptions(row, column).tileRow + descriptions(row, column).tileColumn) % 2);
			std::cout << "Value: " << std::setw(2) << descriptions(row, column).value << "      ";
		}

		std::cout << "\n";

		for (int column = 0; column < COLS; column++) 
		{
			SetConsoleColor((descriptions(row, column).tileRow + descriptions(row, column).tileColumn) % 2);
			std::cout << "Tile:   " << "(" << descriptions(row, column).tileRow << "," << descriptions(row, column).tileColumn << ")  ";
		}
		std::cout << "\n";

		for (int column = 0; column < COLS; column++) 
		{
			SetConsoleColor((descriptions(row, column).tileRow + descriptions(row, column).tileColumn) % 2);
			std::cout << "Global: " << "(" << descriptions(row, column).globalRow << "," << descriptions(row, column).globalColumn << ")  ";
		}
		std::cout << "\n";

		for (int column = 0; column < COLS; column++)
		{
			SetConsoleColor((descriptions(row, column).tileRow + descriptions(row, column).tileColumn) % 2);
			std::cout << "Local:  " << "(" << descriptions(row, column).localRow << "," << descriptions(row, column).localColumn << ")  ";
		}
		std::cout << "\n";
		std::cout << "\n";
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	TilingDescription();
	char wait;
	std::cin >> wait;
	return 0;
}

