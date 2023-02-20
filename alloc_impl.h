#pragma once

#include "alloc.h"
#include "block.h"

#include <>

struct AllocatorImpl : public IAllocator
{
    AllocatorImpl( size_t pullSizeBytes );
    ~AllocatorImpl();

    // overrides IAllocator
    virtual BlockAddress malloc( int len ) final;
    virtual void free( const BlockAddress& addr ) final;

    const size_t N_;

    std::vector<SizedPull> pulls_;
};
