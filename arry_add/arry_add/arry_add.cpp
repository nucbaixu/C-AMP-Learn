// arry_add.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "amp.h"
#include "iostream"

using namespace concurrency;

inline double ElapsedTime(const LARGE_INTEGER& start, const LARGE_INTEGER& end)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq); // 获取时钟周期
	return (double(end.QuadPart) - double(start.QuadPart)) * 1000.0 / double(freq.QuadPart);
}

template <typename Func>
double TimeFunc(Func f)
{
	//  This assumes that the kernel runs on the default view. If it doesn't then this code 
	//  will need to be altered to account for that.
	accelerator_view view = accelerator(accelerator::default_accelerator).default_view;

	//  Ensure that the C++ AMP runtime is initialized.
	accelerator::get_all();

	//  Ensure that the C++ AMP kernel has been JITed.
	//f();

	//  Wait for all accelerator work to end.
	view.wait();

	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);

	f();

	//  Wait for all accelerator work to end.
	view.wait();
	QueryPerformanceCounter(&end);

	return ElapsedTime(start, end);
}

void StandardMethod() 
{

	int aCPP[] = {1, 2, 3, 4, 5};
	int bCPP[] = {6, 7, 8, 9, 10};
	int sumCPP[5];

	for (int idx = 0; idx < 5; idx++)
	{
		sumCPP[idx] = aCPP[idx] + bCPP[idx];
	}

	for (int idx = 0; idx < 5; idx++)
	{
		std::cout << sumCPP[idx] << "\n";
	}
}


const int size = 5;
void CppAmpMethod()
{
	int aCpp[] = {1,2,3,4,5};
	int bCpp[] = {6,7,8,9,10};
	int sumCpp[size];

	// Create C++ AMP object.

	array_view<const int ,1> a(size,aCpp);
	array_view<const int ,1> b(size,bCpp);
	array_view<int ,1> sum(size,sumCpp);
	
	//暗示无需把初始数值复制到加速器
	sum.discard_data();

	parallel_for_each(
		
		//  Define the compute domain, which is the set of threads that are created.
		sum.extent,
		
		// Define the code to run on each thread on the accelerator.
		[=](index<1> idx) restrict(amp)
	{
		sum[idx] = a[idx] + b[idx];
	}
		
		);

		for (int i = 0; i < size; i++)
		{
			std::cout<<sum[i]<<"\n";
		}
}


int _tmain(int argc, _TCHAR* argv[])
{
	accelerator defaultDevice(accelerator::default_accelerator);
	std::wcout << L" Using device : " << defaultDevice.get_description() << std::endl;

	accelerator_view defaultView = defaultDevice.default_view;
	std::wcout << L" Using accelerator_view : " << defaultView.get_version() << std::endl;


	//C++ 11 lambda的使用
	double elapsedTime = TimeFunc([&]()
	{
		CppAmpMethod();
	});

	 std::wcout << std::endl << "GPU exec time: " << elapsedTime << " (ms)" << std::endl;

	elapsedTime = TimeFunc([&]()
	 {
		 StandardMethod();
	 });

	 std::wcout << std::endl << "CPU exec time: " << elapsedTime << " (ms)" << std::endl;

	//CppAmpMethod();

	
	
	return 0;
}

