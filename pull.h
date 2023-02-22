#pragma once

#include "block_addr.h"

#include <fstream>
#include <memory>
#include <mutex>
#include <vector>

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
    Pull(unsigned blockSize, size_t n);

    BlockAddress Alloc(const char* src, int len);
    void Free(const BlockAddress& addr, char* dst);
    std::string PrintState() const;

private:
    const unsigned blockSize_;
    const BlockIdx n_;

    size_t idGen_; // хранит ид для следующего распределяемого блока (инкрементальный
                   // счетчик), в совокупоностью с номером пула образует уникальный
                   // адрес блока в аллокаторе
    BlockIdx availCnt_; // счетчик свободных блоков в пуле
    BlockIdx availHeadIdx_; // индекс первого свободного блока, если (availHeadIdx_
                            // == -1), то весь пул занят (нет свободных блоков)
    BlockIdx headAllocIdx_; // индекс самого старшего из распределенных блоков, если
                            // (headAllocIdx_ == -1), то распределенных блоков нет
                            // (весь пул свободен)
    BlockIdx tailAllocIdx_; // индекс самого молодого из распределенных блоков, если
                            // (tailAllocIdx_ == -1), то распределенных блоков нет
                            // (весь пул свободен)
    size_t swapCnt_; // счетчик сброшенных в swap блоков

    struct BlockExt
    {
        size_t blkId_ = -1; // ид распределенного блока памяти, сохраненного в
                            // соответствующем блоке этого пула
        BlockNode node_;
    };

    BlockIdx AllocateBlock();
    void FreeBlock(BlockIdx releasedIdx);
    BlockIdx SwapAndRelocate();

    char* GetBlockMemoryPos(BlockIdx blkIdx);
    size_t GetBlockId(BlockIdx blkIdx);
    bool IsEmpty() const;
    bool IsFull() const;

    void ReadBlockFromSwap(size_t blkId, char* dst);
    void WriteBlockToSwap(BlockIdx blkIdx, size_t blkId);

    std::vector<BlockExt> blocksExt_;
    std::vector<char> blocks_;
    std::fstream fSwap;
    std::unique_ptr<std::mutex> mtx_;
};
