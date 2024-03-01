### 整体思路

#### ThreadCache
线程具有独立内存，分别具有多个桶，用于存放固定大小的内存链表
最小桶为8字节，最大桶为65536字节
前16个桶，大小每个依次增加8，从8-128
之后56个桶，大小每个依次增加16，从128-1024
再后56个桶，大小每个依次增加128，从1024-8192
最后56个桶，大小每个依次增加1024，从8192-65536
申请内存时到对应的桶中查找，如果有空闲内存则分配，否则向CentralCache申请内存，注意此时根据桶的大小和返回这个大小的块的个数成反比，由NumMoveSize函数决定，将返回的内存链中第一个内存块返回给线程，剩下的内存块放入桶中；释放内存时，将内存块放入对应的桶中，如果桶中的内存块数超过一定数量(此处设置为NumMoveSize的倍数)，则将内存块放入CentralCache中。


#### CentralCache
相同大小的内存块组织成链，成为一个Span，Span连接起来组成固定大小的SpanList，盖层存放着不同大小的SpanList，每个SpanList视为一个桶，具有独立的锁，这样可以减少锁的争用。  

ThreadCache向CentralCache申请内存时，首先根据内存大小找到对应的SpanList，然后从SpanList中取出一个不为空的Span，从该Span中取出尽可能满足num要求个数的内存链；如果没有满足要求的Span，则向PageCache申请一定数量的页，然后将页分割成指定大小后接入新的Span中，放入对应的SpanList中，然后再次用该Span分配内存。  

释放内存时，由于PageCache分配时将每页所在Span的信息保存了起来，因此可以找到对应的Span，然后将内存块放入Span中，如果这个Span的计数值减为0，表明切出去的内存块都已经释放，可以将这个包含整页内存的Span放入PageCache中。

#### PageCache
每个Span大小都为页的倍数，组织成SpanList，每个SpanList视为一个桶，根据页数访问对应的SpanList。

获取指定页大小的Span时，首先从对应的SpanList中查找，如果没有则向更大的SpanList中查找，分割对应的Span，然后放入对应的SpanList中，修改Span的pageid和pagesize等参数，如果还没有则向系统申请最大页数的内存，整个放入对应的SpanList中，然后再次调用该函数。

将Span还给PageCache时，循环尝试和前面的页以及后面的页合并，将合并后的Span放入对应的SpanList中。


#### malloc

小于64k的内存，直接从ThreadCache中分配；大于64k，小于PageCache最大桶的内存(128页，512k)，直接从PageCache中分配Span；否则直接向系统申请内存。

#### free

小于64k的内存，直接放入ThreadCache中；大于64k，小于PageCache最大桶的内存(128页，512k)，直接合并进PageCache中；否则直接系统调用释放内存。