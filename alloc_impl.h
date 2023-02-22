#pragma once

#include "alloc.h"
#include "pull.h"

#include <vector>

struct AllocatorImpl : public IAllocator
{
    AllocatorImpl( size_t pullSizeBytes );
    ~AllocatorImpl();

    // overrides IAllocator
    virtual BlockAddress MAlloc(const char* src, int len) final;
    virtual void Free(const BlockAddress& addr, char* dst) final;
    virtual std::string PrintState() const final;

    const size_t N_;

private:
    std::vector<Pull> pulls_;
};
