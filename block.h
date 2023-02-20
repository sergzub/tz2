#pragma once

#include <vector>

class SizedPull
{
public:
    SizedPull( unsigned blockSize, size_t n );

private:
    const unsigned blockSize_;
    const size_t n_;

    std::vector<char> blocks_;
    size_t availCnt_; // счетчик свободных блоков
    size_t firstAvailIdx_; // индекс первого свободного блока, если (firstAvailIdx_ == _n), то весь пул занят (нет свободных блоков)
    size_t oldestAllocIdx_; // индекс самого старшего из распределенных блоков, если (oldestAllocIdx_ == _n), то распределенных блоков нет (весь пул свободен)
    size_t idCnt_; // ид последнего распределенного блока (инкрементальный), в совокупонстью с номером пула образует уникальный адрес блока в аллокаторе

    struct BlockInfo
    {
        size_t chainIdx_;
    };

    std::vector<BlockInfo> blockMeta_;
};
