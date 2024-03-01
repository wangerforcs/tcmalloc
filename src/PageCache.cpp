#include"PageCache.h"

Span* PageCache::_NewSpan(size_t numpage)
{
	//_spanLists[numpage].Lock();
	if (!_spanLists[numpage].Empty())
	{
		Span* span = _spanLists[numpage].Begin();
		_spanLists[numpage].PopFront();
		return span;
	}

	for (size_t i = numpage + 1; i < MAX_PAGES; ++i)
	{
		if (!_spanLists[i].Empty())
		{
			// 分裂
			Span* span = _spanLists[i].Begin();
			_spanLists[i].PopFront();

			Span* splitspan = new Span;
			splitspan->_pageid = span->_pageid + span->_pagesize - numpage;
			splitspan->_pagesize = numpage;
			for (PAGE_ID i = 0; i < numpage; ++i)
			{
				_idSpanMap[splitspan->_pageid + i] = splitspan;
				// _idSpanMap.set(splitspan->_pageid + i, splitspan);
			}

			span->_pagesize -= numpage;

			_spanLists[span->_pagesize].PushFront(span);

			return splitspan;
		}
	}

	void* ptr = SystemAlloc(MAX_PAGES - 1);

	Span* bigspan = new Span;
	bigspan->_pageid = (PAGE_ID)ptr >> PAGE_SHITF;
	bigspan->_pagesize = MAX_PAGES - 1;

	for (PAGE_ID i = 0; i < bigspan->_pagesize; ++i)
	{
		_idSpanMap[bigspan->_pageid + i] = bigspan;
		// _idSpanMap.set(bigspan->_pageid + i, bigspan);
	}

	_spanLists[bigspan->_pagesize].PushFront(bigspan);

	return _NewSpan(numpage);
}

Span* PageCache::NewSpan(size_t numpage)
{
	_mtx.lock();
	Span* span = _NewSpan(numpage);
	_mtx.unlock();
	return span;
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{
	// 向前合并
	while (1)
	{
		PAGE_ID prevPageId = span->_pageid - 1;
		auto pit = _idSpanMap.find(prevPageId);
		// auto pit = (Span*)_idSpanMap.get(prevPageId);
		// 前面的页不存在
		if (pit == _idSpanMap.end())
		// if (pit == nullptr)
		{
			break;
		}

		// 说明前一个也还在使用中，不能合并
		Span* prevSpan = pit->second;
		// Span* prevSpan = pit;
		if (prevSpan->_usecount != 0)
		{
			break;
		}

		if (span->_pagesize + prevSpan->_pagesize >= MAX_PAGES)
		{
			break;
		}
		// 合并
		span->_pageid = prevSpan->_pageid;
		span->_pagesize += prevSpan->_pagesize;
		for (PAGE_ID i = 0; i < prevSpan->_pagesize; ++i)
		{
			_idSpanMap[prevSpan->_pageid + i] = span;
			// _idSpanMap.set(prevSpan->_pageid + i, span);
		}


		_spanLists[prevSpan->_pagesize].Erase(prevSpan);
		delete prevSpan;
	}


	// 向后合并
	while (1)
	{
		PAGE_ID nextPageId = span->_pageid + span->_pagesize;
		auto nextIt = _idSpanMap.find(nextPageId);
		// auto nextIt = (Span*)_idSpanMap.get(nextPageId);
		if (nextIt == _idSpanMap.end())
		// if (nextIt == nullptr)
		{
			break;
		}

		Span* nextSpan = nextIt->second;
		// Span* nextSpan = nullptr;
		if (nextSpan->_usecount != 0)
		{
			break;
		}

		if (span->_pagesize + nextSpan->_pagesize >= MAX_PAGES)
		{
			break;
		}
		span->_pagesize += nextSpan->_pagesize;
		for (PAGE_ID i = 0; i < nextSpan->_pagesize; ++i)
		{
			_idSpanMap[nextSpan->_pageid + i] = span;
			// _idSpanMap.set(nextSpan->_pageid + i, span);
		}

		_spanLists[nextSpan->_pagesize].Erase(nextSpan);
		delete nextSpan;
	}

	_spanLists[span->_pagesize].PushFront(span);
}

Span* PageCache::GetIdToSpan(PAGE_ID id)
{
	//std::unordered_map<PAGE_ID, Span*>::iterator it = _idSpanMap.find(id);
	//std::map<PAGE_ID, Span*>::iterator it = _idSpanMap.find(id);
	auto it = _idSpanMap.find(id);
	// auto it = (Span*)_idSpanMap.get(id);
	if (it != _idSpanMap.end())
	// if(it != nullptr)
	{
		return it->second;
		// return it;
	}
	else
	{
		return nullptr;
	}
}