#include "trace.h"
#include "pull.h"

#include <cassert>
#include <cstring>
#include <thread>
#include <iomanip>


Pull::Pull( unsigned blockSize, size_t n )
    : blockSize_(blockSize)
    , n_(n)
    , blocksExt_(n)
    , blocks_(blockSize * n)
    , availCnt_(0)
    , availHeadIdx_(-1)
    , headAllocIdx_(-1)
    , tailAllocIdx_(-1)
    , idGen_(0)
    , swapCnt_(0)
    , mtx_(new std::mutex())
{
    // Подготовить список свободных блоков
    for(BlockIdx i = 0; i < n-1; ++i)
    {
        blocksExt_[i].node_.next_ = i+1;
    }
    availHeadIdx_ = 0;
    availCnt_ = n;

    std::stringstream ss;
    ss << "../swap/swap_" << std::setw(4) << std::setfill('0') << std::to_string(blockSize);
    const auto swapFileName = ss.str();
    TraceOut() << "Prepare swap file '" << swapFileName;

    fSwap.exceptions(std::fstream::failbit | std::fstream::badbit);
    fSwap.open(swapFileName, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
}

BlockAddress Pull::Alloc(const char* src, int len)
{
    std::lock_guard<std::mutex> lk(*mtx_);

    const BlockIdx blkIdx = AllocateBlock();
    std::memcpy(GetBlockMemoryPos(blkIdx), src, len);

    BlockAddress res;
    res.len_    = len;
    res.blkId_  = GetBlockId(blkIdx);
    res.blkIdx_ = blkIdx;

    TraceErr() << "Alloc: pull=" << blockSize_ << ", len=" << len << ", blk=" << blkIdx;

    return res;
}

void Pull::Free(const BlockAddress& addr, char* dst)
{
    std::lock_guard<std::mutex> lk(*mtx_);

    TraceErr() << "Free: pull=" << blockSize_ << ", len=" << addr.len_ << ", blk=" << addr.blkIdx_;

    const auto releasedId = GetBlockId(addr.blkIdx_);
    if(addr.blkId_ == releasedId)
    {
        std::memcpy(dst, GetBlockMemoryPos(addr.blkIdx_), addr.len_);
        FreeBlock(addr.blkIdx_);
    }
    else
    {
        ReadBlockFromSwap(addr.blkId_, dst);
    }
}

std::string Pull::PrintState() const
{
    std::lock_guard<std::mutex> lk(*mtx_);

    char buf[4096];
    static const char fmt[] = "| %4u | %12u | %12u | %21lu | %21lu |";
    const int n = std::snprintf(buf, sizeof(buf), fmt,
        blockSize_, availCnt_, n_ - availCnt_, idGen_, swapCnt_ );
    return n > 0 ? buf : "<error>";
}

BlockIdx Pull::SwapAndRelocate()
{
    TraceErr() << "SwapAndRelocate(): headAllocIdx_=" << headAllocIdx_;

    const BlockExt& b = blocksExt_[headAllocIdx_];
    WriteBlockToSwap(headAllocIdx_, b.blkId_);

    assert(n_ > 1);

    // // Переместить сброшенный в swap блок в LRU-списке аллоцированных блоков из головы в хвост
    // BlockNode& headNode = blocksExt_[headAllocIdx_].node_;
    // BlockNode& tailNode = blocksExt_[tailAllocIdx_].node_;
    
    // const BlockIdx newHeadIdx = headNode.next_;
    // blocksExt_[newHeadIdx].node_.prev_ = -1;
    // headNode.next_ = -1;
    // headNode.prev_ = tailAllocIdx_;
    // tailNode.next_ = headAllocIdx_;
    // tailAllocIdx_ = headAllocIdx_;
    
    // const BlockIdx oldHeadIdx = headAllocIdx_;
    // headAllocIdx_ = SetHeadAllocIdx(newHeadIdx);
    // return oldHeadIdx;

    FreeBlock(headAllocIdx_);
    return AllocateBlock();
}

BlockIdx Pull::AllocateBlock()
{
    if(IsFull())
    {
        return SwapAndRelocate();
    }

    // Извлекаем блок из списка свободных блоков
    const BlockIdx allocatedIdx = availHeadIdx_;
    availHeadIdx_ = blocksExt_[availHeadIdx_].node_.next_;

    // И вставляем ее в хвост цепочки распределенных блоков
    if(IsEmpty())
    {
        blocksExt_[allocatedIdx].node_.Clear();
        headAllocIdx_ = SetHeadAllocIdx(allocatedIdx);
        tailAllocIdx_ = allocatedIdx;
    }
    else
    {
        blocksExt_[allocatedIdx].node_.prev_ = tailAllocIdx_;
        blocksExt_[allocatedIdx].node_.next_ = -1;
        blocksExt_[tailAllocIdx_].node_.next_ = allocatedIdx;
        tailAllocIdx_ = allocatedIdx;
    }

    --availCnt_;

    // Сохраняем ид нового распределенного блока и формируем новый
    blocksExt_[allocatedIdx].blkId_ = idGen_;
    ++idGen_;

    return allocatedIdx;
}

BlockIdx Pull::SetHeadAllocIdx(BlockIdx idx)
{
    TraceErr() << "SetHeadAllocIdx = " << idx;
    return idx;
}

void Pull::FreeBlock(BlockIdx releasedIdx)
{
    assert(!IsEmpty());

    BlockNode& releasedNode = blocksExt_[releasedIdx].node_;

    // Перемещаем блок из списка распределенных блоков в список свободных
    // if(headAllocIdx_ == tailAllocIdx_) // если освобождаем последний блок
    // {
    //     headAllocIdx_ = SetHeadAllocIdx(-1);
    //     tailAllocIdx_ = -1;
    // }
    // else
    {
        // assert( availCnt_ + 1 != n_ );

        if(releasedNode.prev_ != -1)
        {
            assert(releasedIdx != headAllocIdx_);

            BlockNode& prevNode = blocksExt_[releasedNode.prev_].node_;
            prevNode.next_ = releasedNode.next_;
        }
        else // это должен быть головной элемент
        {
            assert(releasedIdx == headAllocIdx_);

            headAllocIdx_ = SetHeadAllocIdx(releasedNode.next_);
        }

        if(releasedNode.next_ != -1)
        {
            assert(releasedIdx != tailAllocIdx_);

            BlockNode& nextNode = blocksExt_[releasedNode.next_].node_;
            nextNode.prev_ = releasedNode.prev_;
        }
        else // это должен быть хвостовой элемент
        {
            assert(releasedIdx == tailAllocIdx_);

            tailAllocIdx_ = releasedNode.prev_;
        }

        // if(releasedIdx == headAllocIdx_) // освобождается головной элемент
        // {
        //     BlockNode& nextNode = blocksExt_[releasedNode.next_].node_;
        //     nextNode.prev_ = releasedNode.prev_;
        //     headAllocIdx_ = SetHeadAllocIdx(releasedNode.next_);
        // }
        // else
        // {
        //     BlockNode& prevNode = blocksExt_[releasedNode.prev_].node_;
        //     prevNode.next_ = releasedNode.next_;
        // }

        // if(releasedIdx == tailAllocIdx_) // освобождается хвостовой элемент
        // {
        //     BlockNode& prevNode = blocksExt_[releasedNode.prev_].node_;
        //     prevNode.next_ = -1;
        //     tailAllocIdx_ = releasedNode.prev_;
        // }
        // else
        // {
        //     BlockNode& nextNode = blocksExt_[releasedNode.next_].node_;
        //     nextNode.prev_ = releasedNode.prev_;
        // }
    }

    blocksExt_[releasedIdx].blkId_ = -1;
    // Возвращаем освобожденный блок в список свободных
    releasedNode.prev_ = -1;
    releasedNode.next_ = availHeadIdx_;
    availHeadIdx_ = releasedIdx;
    ++availCnt_;
}

void Pull::ReadBlockFromSwap(size_t blkId, char* dst)
{
    TraceErr() << "ReadBlockFromSwap(" << blockSize_ << "): " << "blkId=" << blkId;

    fSwap.seekp(blkId * blockSize_);
    fSwap.read(dst, blockSize_);
    --swapCnt_;
}

void Pull::WriteBlockToSwap(BlockIdx blkIdx, size_t blkId)
{
    TraceErr() << "WriteBlockToSwap(" << this << "): blockSize_=" << blockSize_ << ", blkIdx=" << blkIdx << ", blkId=" << blkId;

    fSwap.seekp(blkId * blockSize_);
    fSwap.write(GetBlockMemoryPos(blkIdx), blockSize_);
    ++swapCnt_;
}
