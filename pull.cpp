#include "trace.h"
#include "pull.h"

#include <cassert>
#include <cstring>


Pull::Pull( unsigned blockSize, size_t n )
    : blockSize_(blockSize)
    , n_(n)
    , blocks_(n)
    , availCnt_(n)
    , availHeadIdx_(0)
    , headAllocIdx_(-1)
    , tailAllocIdx_(-1)
    , idGen_(0)
{
    blocksExt_.reserve(n);
    for(size_t i = 0; i < n; ++i)
    {
        BlockExt blkInfo{ i + 1 };
        blocksExt_.emplace_back(blkInfo);
    }

    const auto swapFileName = "../swap/swap_" + std::to_string(blockSize);
    const auto swapFileSize = blockSize * n;
    TraceOut() << "Prepare swap file " << swapFileName;
    fSwap.exceptions(std::fstream::failbit | std::fstream::badbit);
    fSwap.open(swapFileName, std::ios::binary | std::ios::in | std::ios::out);
    fSwap.seekp(swapFileSize - 1);
    fSwap.write("", 1);
}

BlockAddress Pull::Alloc(const char* src, int len)
{
    const BlockIdx blkIdx = AllocateBlockUnsafe();
    std::memcpy(GetBlockMemoryPos(blkIdx), src, len);

    BlockAddress res;
    res.len_    = len;
    res.blkId_  = GetBlockId(blkIdx);
    res.blkIdx_ = blkIdx;

    return res;
}

void Pull::Free(const BlockAddress& addr, char* dst)
{
    const auto releasedId = GetBlockId(addr.blkIdx_);
    if(addr.blkId_ == releasedId)
    {
        std::memcpy(dst, GetBlockMemoryPos(addr.blkIdx_), addr.len_);
    }
    else
    {
        ReadBlockFromSwap(releasedId, dst);
    }

    FreeBlockUnsafe(addr.blkIdx_);
}

BlockIdx Pull::SwapAndAllocateUnsafe()
{
    WriteBlockToSwap(headAllocIdx_);

    BlockExt& b = blocksExt_[headAllocIdx_];
    WriteBlockToSwap(b.blkId_);
    b.blkId_ = idGen_;
    ++idGen_;

    // Переместить сброшенный в swap блок в LRU-списке из головы в хвост
    BlockNode& headNode = blocksExt_[headAllocIdx_].node_;
    BlockNode& tailNode = blocksExt_[tailAllocIdx_].node_;
    
    const BlockIdx newHeadIdx = headNode.next_;
    blocksExt_[newHeadIdx].node_.prev_ = -1;
    headNode.next_ = -1;
    headNode.prev_ = tailAllocIdx_;
    tailAllocIdx_ = headAllocIdx_;
    
    const BlockIdx oldHeadIdx = headAllocIdx_;
    headAllocIdx_ = newHeadIdx;
    
    return oldHeadIdx;
}

BlockIdx Pull::AllocateBlockUnsafe()
{
    if(IsFullUnsafe())
    {
        return SwapAndAllocateUnsafe();
    }

    // Извлекаем блок из цепочки свободных блоков
    const BlockIdx allocatedIdx = availHeadIdx_;
    availHeadIdx_ = blocksExt_[allocatedIdx].node_.next_;
    --availCnt_;

    // И вставляем ее в хвост цепочки распределенных блоков
    if(IsEmptyUnsafe())
    {
        blocksExt_[allocatedIdx].node_.Clear();
        headAllocIdx_ = allocatedIdx;
        tailAllocIdx_ = allocatedIdx;
    }
    else
    {
        blocksExt_[allocatedIdx].node_.prev_ = tailAllocIdx_;
        blocksExt_[allocatedIdx].node_.next_ = -1;
        blocksExt_[tailAllocIdx_].node_.next_ = allocatedIdx;
        tailAllocIdx_ = allocatedIdx;
    }

    // Сохраняем ид нового распределенного блока и формируем новый
    blocksExt_[allocatedIdx].blkId_ = idGen_;
    ++idGen_;

    return allocatedIdx;
}

void Pull::FreeBlockUnsafe(BlockIdx releasedIdx)
{
    assert(!IsEmptyUnsafe());

    BlockNode& releasedNode = blocksExt_[releasedIdx].node_;

    // Перемещаем блок из списка распределенных блоков в список свободных
    if(headAllocIdx_ == tailAllocIdx_) // если освобождаем последний блок
    {
        headAllocIdx_ = -1;
        tailAllocIdx_ = -1;
    }
    else
    {
        if(releasedIdx != headAllocIdx_) // освобождается не головной элемент
        {
            BlockNode& prevNode = blocksExt_[releasedNode.prev_].node_;
            prevNode.next_ = releasedNode.next_;
        }
        else
        {
            headAllocIdx_ = releasedNode.next_;
        }

        if(releasedIdx != tailAllocIdx_) // освобождается не хвостовой элемент
        {
            BlockNode& nextNode = blocksExt_[releasedNode.next_].node_;
            nextNode.prev_ = releasedNode.prev_;
        }
        else
        {
            tailAllocIdx_ = releasedNode.prev_;
        }
    }

    blocksExt_[releasedIdx].blkId_ = -1;
    releasedNode.prev_ = -1;
    releasedNode.next_ = availHeadIdx_;
    availHeadIdx_ = releasedIdx;
    ++availCnt_;
}

void Pull::ReadBlockFromSwap(size_t blkId, char* dst)
{
    fSwap.seekp(blkId * blockSize_);
    fSwap.read(dst, blockSize_);
}

void Pull::WriteBlockToSwap(size_t blkId)
{
    fSwap.seekp(blkId * blockSize_);
    fSwap.write(GetBlockMemoryPos(blkId), blockSize_);
}
