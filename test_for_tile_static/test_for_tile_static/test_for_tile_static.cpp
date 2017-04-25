// test_for_tile_static.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <amp.h>
using namespace concurrency;

#define SAMPLESIZE 2
#define MATRIXSIZE 8

void SamplingExample()
{

	// Create data and array_view for the matrix.
	std::vector<float> rawData;
	for (int i = 0; i < MATRIXSIZE * MATRIXSIZE; i++) 
	{
		rawData.push_back((float)i);
	}

	extent<2> dataExtent(MATRIXSIZE, MATRIXSIZE);
	array_view<float, 2> matrix(dataExtent, rawData);

	// Create the array for the averages.
	// There is one element in the output for each tile in the data.
	std::vector<float> outputData;
	int outputSize = MATRIXSIZE / SAMPLESIZE;
	
	for (int j = 0; j < outputSize * outputSize; j++)
	{
		outputData.push_back((float)0);
	}


	extent<2> outputExtent(MATRIXSIZE / SAMPLESIZE, MATRIXSIZE / SAMPLESIZE);
	array<float, 2> averages(outputExtent, outputData.begin(), outputData.end());

	// Use tiles that are SAMPLESIZE x SAMPLESIZE.
	// Find the average of the values in each tile.
	// The only reference-type variable you can pass into the parallel_for_each call
	// is a concurrency::array.
	parallel_for_each(matrix.extent.tile<SAMPLESIZE, SAMPLESIZE>(),
		[=, &averages] (tiled_index<SAMPLESIZE, SAMPLESIZE> t_idx) restrict(amp) 
	{
		// Copy the values of the tile into a tile-sized array.
		tile_static float tileValues[SAMPLESIZE][SAMPLESIZE];
		tileValues[t_idx.local[0]][t_idx.local[1]] = matrix[t_idx];

		// Wait for the tile-sized array to load before you calculate the average.
		t_idx.barrier.wait();

		// If you remove the if statement, then the calculation executes for every
		// thread in the tile, and makes the same assignment to averages each time.
		if (t_idx.local[0] == 0 && t_idx.local[1] == 0) 
		{
			for (int trow = 0; trow < SAMPLESIZE; trow++) 
			{
				for (int tcol = 0; tcol < SAMPLESIZE; tcol++) 
				{
					averages(t_idx.tile[0],t_idx.tile[1]) += tileValues[trow][tcol];
				}
			}

			averages(t_idx.tile[0],t_idx.tile[1]) /= (float) (SAMPLESIZE * SAMPLESIZE);
		}
	});

	// Print out the results.
	// You cannot access the values in averages directly. You must copy them
	// back to a CPU variable.
	outputData = averages;
	for (int row = 0; row < outputSize; row++)
	{
		for (int col = 0; col < outputSize; col++) 
		{
			std::cout << outputData[row*outputSize + col] << " ";
		}

		std::cout << "\n";
	}
	
	// Output for SAMPLESSIZE = 2 is:
	//  4.5  6.5  8.5 10.5
	// 20.5 22.5 24.5 26.5
	// 36.5 38.5 40.5 42.5
	// 52.5 54.5 56.5 58.5

	// Output for SAMPLESIZE = 4 is:
	// 13.5 17.5
	// 45.5 49.5
}

void default_properties() 
{


	std::vector<accelerator> accs = accelerator::get_all();
	for (int i = 0; i < accs.size(); i++) 
	{
		std::wcout << accs[i].device_path << "\n";
		std::wcout << accs[i].dedicated_memory << "\n";
		/*std::wcout << (accs[i].supports_cpu_shared_memory ? 
			"CPU shared memory: true" : "CPU shared memory: false") << "\n";*/
	
		std::wcout << (accs[i].supports_double_precision ? 
			"double precision: true" : "double precision: false") << "\n";
		std::wcout << (accs[i].supports_limited_double_precision ? 
			"limited double precision: true" : "limited double precision: false") << "\n";
	}
	
	
}
int _tmain(int argc, _TCHAR* argv[])
{
	SamplingExample();
	default_properties();
	return 0;
}

