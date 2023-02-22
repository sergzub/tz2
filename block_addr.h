#pragma once

#include <cstddef>
#include <cstdint>


// Тип для хранения позиции блока в пуле (uint32_t должно хватить)
using BlockIdx = uint32_t;

// Структура для предсталвения размера и адреса распределяемого блока
struct BlockAddress
{
    // длина области в блоке
    int len_ = 0;

    // номер блока в пуле, который определяется по значению len_
    BlockIdx blkIdx_ = -1;

    // уникальный ид распределнного блока, в пределах пула
    size_t blkId_ = 0;
};
