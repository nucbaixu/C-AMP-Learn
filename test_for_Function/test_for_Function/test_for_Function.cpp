// test_for_Function.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "amp.h"
#include "iostream"
using namespace concurrency;

void AddElementsWithRestrictedFunction(index<1> idx, array_view<int, 1> sum, array_view<int, 1> a, array_view<int, 1> b) restrict(amp)
{
	sum[idx] = a[idx] + b[idx];
}


void AddArraysWithFunction() {

	int aCPP[] = {1, 2, 3, 4, 5};
	int bCPP[] = {6, 7, 8, 9, 10};
	int sumCPP[5];

	array_view<int, 1> a(5, aCPP);
	array_view<int, 1> b(5, bCPP);
	array_view<int, 1> sum(5, sumCPP);
	sum.discard_data();

	parallel_for_each(
		sum.extent, 
		[=](index<1> idx) restrict(amp)
	{
		AddElementsWithRestrictedFunction(idx, sum, a, b);
	}
	);

	for (int i = 0; i < 5; i++) {
		std::cout << sum[i] << "\n";
	}
}

int _tmain(int argc, _TCHAR* argv[])
{

	
	AddArraysWithFunction();
	Sleep(10000);
	return 0;
}

