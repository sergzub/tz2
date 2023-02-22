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

    static const std::array<int, 9> data = {16,  32,   64,   128, 256,
                                            512, 1024, 2048, 4096};

    const auto lower = std::lower_bound(data.begin(), data.end(), len);

    return std::distance(data.begin(), lower);
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