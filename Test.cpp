#include "ConcurrentMalloc.h"
#include <vector>
#include<stdlib.h>
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	size_t time = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&, k]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					void* p = malloc(16);
					if(i%2==0)
						free(p);
					else
						v.push_back(p);
				}
				for(auto p:v)
				{
					free(p);
				}
				size_t end1 = clock();
				v.clear();
				time += end1 - begin1;
			}
		});
	}

	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%lu threads %lu rounds %lu times %lu ms\n",
		nworks, rounds, ntimes, time);

}


// 单轮次申请释放次数 线程数 轮次
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	size_t time = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					void*p = ConcurrentMalloc(16);
					if(i%2==0)
						ConcurrentFree(p);
					else
						v.push_back(p);
				}
				for(auto p:v)
				{
					ConcurrentFree(p);
				}
				size_t end1 = clock();
				time += end1 - begin1;
				v.clear();
			}
		});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	printf("%lu threads %lu rounds %lu times %lu ms\n",
		nworks, rounds, ntimes, time/2);

}

int main()
{
	cout << "==========================================================" << endl;
	BenchmarkMalloc(10000, 6, 10);
	cout << endl << endl;
	BenchmarkConcurrentMalloc(10000, 6, 10);
	cout << "==========================================================" << endl;
	return 0;
}