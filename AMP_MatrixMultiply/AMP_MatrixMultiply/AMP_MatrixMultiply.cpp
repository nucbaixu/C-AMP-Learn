// AMP_MatrixMultiply.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "amp.h"
#include <random>

using namespace concurrency;
using namespace std;

/************************************************************************/
/* CPU                                                                  */
/************************************************************************/
void MultiplyWithOutAMP() 
{

	int aMatrix[3][2] = {{1, 4}, {2, 5}, {3, 6}};
	int bMatrix[2][3] = {{7, 8, 9}, {10, 11, 12}};
	int product[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

	for (int row = 0; row < 3; row++) 
	{
		for (int col = 0; col < 3; col++) 
		{
			// Multiply the row of A by the column of B to get the row, column of product.
			for (int inner = 0; inner < 2; inner++) 
			{
				product[row][col] += aMatrix[row][inner] * bMatrix[inner][col];
			}
			std::cout << product[row][col] << "  ";
		}
		std::cout << "\n";
	}
}


/************************************************************************/
/* GPU :AMP                                                             */
/************************************************************************/
void MultiplyWithAMP()
{
	int aMatrix[] = { 1, 4, 2, 5, 3, 6 };
	int bMatrix[] = { 7, 8, 9, 10, 11, 12 };
	int productMatrix[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	array_view<int,2> a(3,2,aMatrix);
	array_view<int,2> b(2,3,bMatrix);
	array_view<int,2> product(3,3,productMatrix);

	parallel_for_each(
		product.extent,
		[=](index<2> idx) restrict(amp)
	{ 
		int row = idx[0];            //idx[0] 代表行的个数
		int col = idx[1];

		for (int i = 0; i < 2; i++)
		{
			product[idx] += a(row,i) * b(i,col); 
		}
	}

		);

		product.synchronize();


		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				//std::cout << productMatrix[row*3 + col] << "  ";
				std::cout << product(row, col) << "  ";
			}
			std::cout << "\n";
		}



}


/************************************************************************/
/* GPU :tiling                                                          */
/************************************************************************/

//===========================================================================================================================//
//  分组 （tiling）是将你的数据分成同等大小的子集，被叫做 tiles. 当您使用tiling），会有三样改变。                                 //
//	您可以创建 tile_static 变量。 访问 tile_static 空间中的数据要比访问全局空间中的数据快得多。                                   //
//  tile_static 变量的实例为每个分组而创建，且分组中的所有线程都可以访问该变量。                                                  //
//  分组（tiling）的主要好处是访问 tile_static 获得的高性能。                                                                   //
//	你可以在特定一行代码中调用 tile_barrier::wait 方法停止一个分组中的所有线程.                                                  //
//  若非调用 tile_barrier::wait 使得一个分组（tiling）中的所有的线程在它们继续执行之前停止，您将无法确保线程的运行顺序。            //
//	您可以访问既与整个 array_view 对象相对的又与分组（tiling）相对的线程的索引。 使用本地索引可使您的代码更易阅读和调试。           //
//	在矩阵求积中要利用分组（tiling）的优势，你的算法必须将矩阵分块，然后将每一块数据复制到 tile_static 变量中，从而加快访问速度。    //
//============================================================================================================================//
//  在下面的示例中，该矩阵被划分为同等大小的子矩阵。

//  1. 在 parallel_for_each 调用中使用 tiled_extent 对象替换 extent 对象。
//	2. 在 parallel_for_each 调用中使用 tiled_index 对象替换 index 对象。
//	3. 创建 tile_static 变量以保存子矩阵.
//	4.使用 tile_barrier::wait 方法停止进行子矩阵求积的线程.
 
void MultiplyWithTiling()
{
	// The tile size is 2.
	static const int TS = 2;

	// The raw data.
	int aMatrix[] =       { 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8 };
	int bMatrix[] =       { 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8 };
	int productMatrix[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	// Create the array_view objects.
	array_view<int, 2> a(4, 4, aMatrix);
	array_view<int, 2> b(4, 4, bMatrix);
	array_view<int, 2> product(4, 4, productMatrix);

	// Call parallel_for_each by using  2x2 tiles.
	 parallel_for_each(product.extent.tile< TS, TS >(),
		 [=] (tiled_index< TS, TS> t_idx) restrict(amp)
	 {
		 // Get the location of the thread relative to the tile (row, col) and the entire array_view (rowGlobal, colGlobal).
		 int row = t_idx.local[0];
		 int col = t_idx.local[1];

		 int rowGlobal = t_idx.global[0];
		 int colGlobal = t_idx.global[1];

		 int sum = 0;

		 // Given a 4x4 matrix and a 2x2 tile size, this loop executes twice for each thread.
		 // For the first tile and the first loop, it copies a into locA and e into locB.
		 // For the first tile and the second loop, it copies b into locA and g into locB.

		 for (int i = 0; i < 4; i+=TS)    //i+=TS 移动到下一分组
		 {
			 tile_static int locA[TS][TS];
			 tile_static int locB[TS][TS];
			 locA[row][col] = a(rowGlobal, col + i);
			 locB[row][col] = b(row + i, colGlobal);
			 // The threads in the tile all wait here until locA and locB are filled.
			 t_idx.barrier.wait();

			 // Return the product for the thread. The sum is retained across
			 // both iterations of the loop, in effect adding the two products
			 // together, for example, a*e.
			 for (int k = 0; k < TS; k++) 
			 {
				 sum += locA[row][k] * locB[k][col];
			 }

			 // All threads must wait until the sums are calculated. If any threads
			 // moved ahead, the values in locA and locB would change.      
			 t_idx.barrier.wait();
			 // Now go on to the next iteration of the loop. 
		 }

		 // After both iterations of the loop, copy the sum to the product variable by using the global location.
		 product[t_idx.global] = sum;

	 }
	 );

	 // Copy the contents of product back to the productMatrix variable.
	 product.synchronize();

	 for (int row = 0; row < 4; row++) 
	 {
		 for (int col = 0; col < 4; col++) 
		 {
			 // The results are available from both the product and productMatrix variables.
			 //std::cout << productMatrix[row*3 + col] << "  ";
			 std::cout << product(row, col) << "  ";
		 }
		 std::cout << "\n";
	 }
}


/************************************************************************/
/* test for tileSize                                                    */
/************************************************************************/
void MultiplyWithTilingtestForSize()
{
	const int M = 64;
	const int N = 512;
	const int W = 256;

	std::vector<float> vA(M * W);
	std::vector<float> vB(W * N);
	std::vector<float> vC(M * N);
	std::vector<float> vRef(M * N);

	std::random_device rd;
	std::default_random_engine engine(rd());
	std::uniform_real_distribution<float> rand(0.0f,1.0f);

	std::generate(vA.begin(),vA.end(),[&rand,&engine](){return rand(engine);});
	std::generate(vB.begin(),vB.end(),[&rand,&engine](){return rand(engine);});

	for (int i = 0; i < M; i++)
	{
		for (int j = 0; j < N; j++)
		{
			float result = 0.0f;

			for (int p = 0; p < W; p++)
			{
				int idxa = i* W + p;
				int idxb = p * N + j;
				result += vA[idxa] + vB[idxb];

			}
			vRef[i * N + j] = result;
		}
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	MultiplyWithOutAMP();

	MultiplyWithAMP();

	MultiplyWithTiling();

	MultiplyWithTilingtestForSize();
	getchar();
	return 0;
}

