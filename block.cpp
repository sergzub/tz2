#include "block.h"


SizedPull::SizedPull( unsigned blockSize, size_t n )
    : blockSize_(blockSize)
    , n_(n)
    , blocks_(n)
    , availCnt_(n)
    , firstAvailIdx_(0)
    , oldestAllocIdx_(n)
    , idCnt_(0)
{
    blockMeta_.reserve(n);
    for(size_t i = 0; i < n; ++i)
    {
        BlockInfo blkInfo{ i+1 };
        blockMeta_.emplace_back(blkInfo);
    }
}
