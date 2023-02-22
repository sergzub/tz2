#pragma once

#include "block_addr.h"

#include <memory>
#include <string>

// Класс интерфейса менеджера памяти
struct IAllocator
{
    virtual BlockAddress MAlloc(const char* src, int len) = 0;
    virtual void         Free(const BlockAddress& addr, char* dst) = 0;
    virtual std::string  PrintState() const = 0;
};

std::unique_ptr<IAllocator> CreateAllocator( size_t pullSizeBytes );
