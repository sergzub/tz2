#pragma once

#include "block_addr.h"

#include <memory>

// Класс интерфейса менеджера памяти
struct IAllocator
{
    virtual BlockAddress malloc(const char* src, int len) = 0;
    virtual void         free(const BlockAddress& addr, char* dst) = 0;
};

std::unique_ptr<IAllocator> CreateAllocator( size_t pullSizeBytes );
