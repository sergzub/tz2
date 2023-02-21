#pragma once

#include "block_addr.h"

#include <vector>
#include <fstream>


struct BlockNode
{
    BlockIdx prev_ = -1;
    BlockIdx next_ = -1;

    void Clear()
    {
        *this = BlockNode();
    }
};

class Pull
{
public:
    Pull( unsigned blockSize, size_t n );

    BlockAddress Alloc(const char* src, int len);
    void Free(const BlockAddress& addr, char* dst);

private:
    const unsigned blockSize_;
    const BlockIdx n_;

    size_t   idGen_; // хранит ид для следующего распределяемого блока (инкрементальный счетчик), в совокупоностью с номером пула образует уникальный адрес блока в аллокаторе
    BlockIdx availCnt_; // счетчик свободных блоков в пуле
    BlockIdx availHeadIdx_; // индекс первого свободного блока, если (availHeadIdx_ == -1), то весь пул занят (нет свободных блоков)
    BlockIdx headAllocIdx_; // индекс самого старшего из распределенных блоков, если (headAllocIdx_ == -1), то распределенных блоков нет (весь пул свободен)
    BlockIdx tailAllocIdx_; // индекс самого молодого из распределенных блоков, если (tailAllocIdx_ == -1), то распределенных блоков нет (весь пул свободен)

    struct BlockExt
    {
        size_t blkId_ = -1; // ид распределенного блока памяти, сохраненного в соответствующем блоке этого пула
        BlockNode node_;
    };

    std::vector<BlockExt> blocksExt_;
    std::vector<char> blocks_;

    char* GetBlockMemoryPos(BlockIdx blkIdx)
    {
        return & blocks_.at(static_cast<size_t>(blkIdx) * blockSize_);
    }

    size_t GetBlockId(BlockIdx blkIdx)
    {
        return blocksExt_.at(blkIdx).blkId_;
    }

    bool IsEmptyUnsafe() const
    {
        return availCnt_ == n_;
    }

    bool IsFullUnsafe() const
    {
        return availCnt_ == 0;
    }

    BlockIdx SwapAndAllocateUnsafe();
    BlockIdx AllocateBlockUnsafe();
    void FreeBlockUnsafe(BlockIdx releasedIdx);

    void ReadBlockFromSwap(size_t blkId, char* dst);
    void WriteBlockToSwap(size_t blkId);
    std::fstream fSwap;
};