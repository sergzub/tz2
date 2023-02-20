#pragma once

// Структура для предсталвения размера и адреса распределяемого блока

struct BlockAddress
{
    int     len_ = 0;
    size_t  id_  = 0;
};
