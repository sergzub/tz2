#pragma once

#include "trace.h"
#include "alloc_impl.h"
#include "block.h"

#include <memory>

static_assert(sizeof(size_t) >= 8);

std::unique_ptr<IAllocator> CreateAllocator( size_t pullSizeBytes )
{
    return std::make_unique<AllocatorImpl>( pullSizeBytes );
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

BlockAddress AllocatorImpl::malloc( int len )
{
    BlockAddress ba;
    return ba;
}
    
void AllocatorImpl::free( const BlockAddress& addr )
{
    (void)addr;
}
