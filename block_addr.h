#pragma once


// Тип для хранения позиции блока в пуле (uint32_t должно хватить)
using BlockIdx = std::uint32_t;

// Структура для предсталвения размера и адреса распределяемого блока
struct BlockAddress
{
    int         len_    = 0;
    BlockIdx    blkIdx_ = -1;
    size_t      blkId_  = 0;
};
