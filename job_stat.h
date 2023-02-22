#pragma once

#include <atomic>
#include <string>


struct FileProcessingStat
{
    std::string         fileName_;
    size_t              fileSize_ = 0;
    std::atomic_size_t  bytesProcessed_ = 0;
};
