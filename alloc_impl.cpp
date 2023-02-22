#include "trace.h"
#include "alloc_impl.h"
#include "pull.h"

#include <memory>
#include <cmath>
#include <cassert>


static_assert(sizeof(size_t) >= 8);

std::unique_ptr<IAllocator> CreateAllocator( size_t pullSizeBytes )
{
    return std::make_unique<AllocatorImpl>( pullSizeBytes );
}

static int GetPullNumberByLen(int len)
{
    static int LenIdx[4097];
    static const bool l = [](int* idx)
    {
        for(int j = 1; j <= 16; ++j)
        {
            idx[j] = 0;
        }
        for(int j = 17; j <= 32; ++j)
        {
            idx[j] = 1;
        }
        for(int j = 33; j <= 64; ++j)
        {
            idx[j] = 2;
        }
        for(int j = 65; j <= 128; ++j)
        {
            idx[j] = 3;
        }
        for(int j = 129; j <= 256; ++j)
        {
            idx[j] = 4;
        }
        for(int j = 257; j <= 512; ++j)
        {
            idx[j] = 5;
        }
        for(int j = 513; j <= 1024; ++j)
        {
            idx[j] = 6;
        }
        for(int j = 1025; j <= 2048; ++j)
        {
            idx[j] = 7;
        }
        for(int j = 2049; j <= 4096; ++j)
        {
            idx[j] = 8;
        }
        return true;
    }(&LenIdx[0]);

    // const int n = std::log2((( len + 15)  / 16) * 16) - 3;

    // if( LenIdx[len] != n )
    // {
    //     TraceErr() << "GetPullNumberByLen( " << len << "): " << n << " != " << LenIdx[len];
    // }

    // assert(len >= 1 && len <= 4096);
    // assert(LenIdx[len] == n);


    TraceErr() << "GetPullNumberByLen( " << len << " ): " << LenIdx[len];

    return LenIdx[len];
}

AllocatorImpl::AllocatorImpl( size_t pullSizeBytes )
    // Рассчитаем число блоков N в каждом из пулов на основе значения pullSizeBytes:
    // pullSizeBytes = N * (16 + 32 .. + 4096) = N * 8176
    : N_( pullSizeBytes / 8176 )
{
    TraceOut() << "Number of blocks in every pull (N): " << N_;

    pulls_.reserve(9); // Отдельные пулы для блоков 16,32..4096
    for(int bs = 16; bs <= 4096; bs *= 2)
    {
        pulls_.emplace_back(bs, N_);
    }
}

AllocatorImpl::~AllocatorImpl()
{
}

BlockAddress AllocatorImpl::MAlloc(const char* src, int len)
{
    return pulls_.at(GetPullNumberByLen(len)).Alloc(src, len );
}
    
void AllocatorImpl::Free(const BlockAddress& addr, char* dst)
{
    pulls_.at(GetPullNumberByLen(addr.len_)).Free(addr, dst);
}

std::string AllocatorImpl::PrintState() const
{
    std::string res;
    res += "--------------------------------------------------------------------------------------\n";
    res += "|   #  |     Avail    |  Allocated   |         idGen         |         swapCnt       |\n";
    res += "--------------------------------------------------------------------------------------\n";
    for(const auto& p : pulls_)
    {
        res += p.PrintState() + '\n';
    }
    res += "--------------------------------------------------------------------------------------\n";
    return res;
}