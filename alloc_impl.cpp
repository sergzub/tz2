#include "trace.h"
#include "alloc_impl.h"
#include "pull.h"

#include <memory>
#include <cmath>


static_assert(sizeof(size_t) >= 8);

std::unique_ptr<IAllocator> CreateAllocator( size_t pullSizeBytes )
{
    return std::make_unique<AllocatorImpl>( pullSizeBytes );
}

static inline int GetPullNumberByLen(int len)
{
    return std::log2((( len + 15 )  / 16 ) * 16 ) - 4;
}

AllocatorImpl::AllocatorImpl( size_t pullSizeBytes )
    // Рассчитаем число блоков N в каждом из пулов на основе значения pullSizeBytes:
    // pullSizeBytes = N * (16 + 32 .. + 4096) = N * 8176
    : N_( pullSizeBytes / 8176 )
{
    TraceOut() << "N: " << N_;

    pulls_.reserve(9); // Отдельные пулы для блоков 16,32..4096
    for(int bs = 16; bs <= 4096; bs *= 2)
    {
        pulls_.emplace_back(bs, N_);
    }
}

AllocatorImpl::~AllocatorImpl()
{
}

BlockAddress AllocatorImpl::malloc(const char* src, int len)
{
    return pulls_.at(GetPullNumberByLen(len)).Alloc(src, len );
}
    
void AllocatorImpl::free(const BlockAddress& addr, char* dst)
{
    pulls_.at(GetPullNumberByLen(addr.len_)).Free(addr, dst);
}
