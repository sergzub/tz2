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
    // Сформировать список свободных блоков
    for (BlockIdx i = 0; i < n - 1; ++i)
    {
        blocksExt_[i].node_.next_ = i + 1;
    }
    availHeadIdx_ = 0;
    availCnt_     = n;

    std::stringstream ss;
    ss << "../swap/swap_" << std::setw(4) << std::setfill('0')
       << std::to_string(blockSize);
    const auto swapFileName = ss.str();
    TraceOut() << "Prepare swap file '" << swapFileName;

    fSwap.exceptions(std::fstream::failbit | std::fstream::badbit);
    fSwap.open(swapFileName,
               std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
}

char* Pull::GetBlockMemoryPos(BlockIdx blkIdx)
{
    return &blocks_.at(static_cast<size_t>(blkIdx) * blockSize_);
}

size_t Pull::GetBlockId(BlockIdx blkIdx)
{
    return blocksExt_.at(blkIdx).blkId_;
}

bool Pull::IsEmpty() const
{
    return availCnt_ == n_;
}

bool Pull::IsFull() const
{
    return availCnt_ == 0;
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

    return res;
}

void Pull::Free(const BlockAddress& addr, char* dst)
{
    std::lock_guard<std::mutex> lk(*mtx_);

    const auto releasedId = GetBlockId(addr.blkIdx_);
    if (addr.blkId_ == releasedId)
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
    const int n = std::snprintf(buf, sizeof(buf), fmt, blockSize_, availCnt_,
                                n_ - availCnt_, idGen_, swapCnt_);
    return n > 0 ? buf : "<error>";
}

BlockIdx Pull::AllocateBlock()
{
    if (IsFull())
    {
        return SwapAndRelocate();
    }

    // Извлекаем блок из списка свободных блоков
    const BlockIdx allocatedIdx = availHeadIdx_;
    availHeadIdx_               = blocksExt_[availHeadIdx_].node_.next_;

    // И вставляем ее в хвост цепочки распределенных блоков
    if (IsEmpty())
    {
        blocksExt_[allocatedIdx].node_.Clear();
        headAllocIdx_ = allocatedIdx;
        tailAllocIdx_ = allocatedIdx;
    }
    else
    {
        blocksExt_[allocatedIdx].node_.prev_  = tailAllocIdx_;
        blocksExt_[allocatedIdx].node_.next_  = -1;
        blocksExt_[tailAllocIdx_].node_.next_ = allocatedIdx;
        tailAllocIdx_                         = allocatedIdx;
    }

    --availCnt_;

    // Сохраняем ид нового распределенного блока и формируем новый
    blocksExt_[allocatedIdx].blkId_ = idGen_;
    ++idGen_;

    return allocatedIdx;
}

void Pull::FreeBlock(BlockIdx releasedIdx)
{
    assert(!IsEmpty());

    BlockNode& releasedNode = blocksExt_[releasedIdx].node_;

    if (releasedNode.prev_ != -1)
    {
        assert(releasedIdx != headAllocIdx_);

        BlockNode& prevNode = blocksExt_[releasedNode.prev_].node_;
        prevNode.next_      = releasedNode.next_;
    }
    else // это должен быть головной элемент
    {
        assert(releasedIdx == headAllocIdx_);

        headAllocIdx_ = releasedNode.next_;
    }

    if (releasedNode.next_ != -1)
    {
        assert(releasedIdx != tailAllocIdx_);

        BlockNode& nextNode = blocksExt_[releasedNode.next_].node_;
        nextNode.prev_      = releasedNode.prev_;
    }
    else // это должен быть хвостовой элемент
    {
        assert(releasedIdx == tailAllocIdx_);

        tailAllocIdx_ = releasedNode.prev_;
    }

    blocksExt_[releasedIdx].blkId_ = -1;
    // Возвращаем освобожденный блок в список свободных
    releasedNode.prev_ = -1;
    releasedNode.next_ = availHeadIdx_;
    availHeadIdx_      = releasedIdx;
    ++availCnt_;
}

BlockIdx Pull::SwapAndRelocate()
{
    const BlockExt& b = blocksExt_[headAllocIdx_];
    WriteBlockToSwap(headAllocIdx_, b.blkId_);

    // Переместить сброшенный в swap блок в LRU-списке аллоцированных блоков из
    // головы в хвост
    FreeBlock(headAllocIdx_);
    return AllocateBlock();
}

void Pull::ReadBlockFromSwap(size_t blkId, char* dst)
{
    fSwap.seekp(blkId * blockSize_);
    fSwap.read(dst, blockSize_);
    --swapCnt_;
}

void Pull::WriteBlockToSwap(BlockIdx blkIdx, size_t blkId)
{
    fSwap.seekp(blkId * blockSize_);
    fSwap.write(GetBlockMemoryPos(blkIdx), blockSize_);
    ++swapCnt_;
}
