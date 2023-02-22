#include "trace.h"
#include "alloc_impl.h"
#include "pull.h"

#include <array>
#include <cassert>
#include <cmath>
#include <memory>


std::unique_ptr<IAllocator> CreateAllocator(size_t pullSizeBytes)
{
    return std::make_unique<AllocatorImpl>(pullSizeBytes);
}

static int GetPullNumberByLen(int len)
{
    assert(len >= 1 && len <= 4096);

    int res = 0;
    if (len > 16)
        res = std::ceil(std::log2(len)) - 4;

    return res;
}

AllocatorImpl::AllocatorImpl(size_t pullSizeBytes)
    // Рассчитаем число блоков N в каждом из пулов на основе значения pullSizeBytes:
    // pullSizeBytes = N * (16 + 32 .. + 4096) = N * 8176
    : N_(pullSizeBytes / 8176)
{
    TraceOut() << "Number of blocks in every pull (N): " << N_;

    pulls_.reserve(9); // Отдельные пулы для блоков 16,32..4096
    for (int bs = 16; bs <= 4096; bs *= 2)
    {
        pulls_.emplace_back(bs, N_);
    }
}

AllocatorImpl::~AllocatorImpl()
{
}

BlockAddress AllocatorImpl::MAlloc(const char* src, int len)
{
    return pulls_.at(GetPullNumberByLen(len)).Alloc(src, len);
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
    for (const auto& p : pulls_)
    {
        res += p.PrintState() + '\n';
    }
    res += "--------------------------------------------------------------------------------------\n";
    return res;
}