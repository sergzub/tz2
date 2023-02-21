#pragma once

#include "alloc.h"
#include "pull.h"

struct AllocatorImpl : public IAllocator
{
    AllocatorImpl( size_t pullSizeBytes );
    ~AllocatorImpl();

    // overrides IAllocator
    virtual BlockAddress malloc(const char* src, int len) final;
    virtual void free(const BlockAddress& addr, char* dst) final;

    const size_t N_;

    std::vector<Pull> pulls_;
};
