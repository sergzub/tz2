#include "trace.h"
#include "alloc.h"
#include "job_stat.h"

#include <string>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <list>
#include <deque>
#include <chrono>
#include <thread>
#include <random>
#include <fstream>
#include <memory>
#include <cassert>


namespace fs = std::filesystem;
using Threads = std::vector<std::thread>;

const std::filesystem::path inPath  = "../in";
const std::filesystem::path outPath = "../out";
const size_t MaxBlockSize = 4096;

static std::unique_ptr<IAllocator> SwappedAllocator;

std::list<FileProcessingStat> JobStats;

static void PrintStateProc(int& breakBarrier)
try
{
    for(; breakBarrier > 0; std::this_thread::sleep_for(std::chrono::seconds(1)))
    {
        if(breakBarrier == 1)
        {
            --breakBarrier;
            // показать итоговую статистику и заверишиться
        }

        const auto t = std::time(nullptr);
        const auto tm = *std::localtime(&t);
        TraceOut() << "\f" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '\n';

        for( const auto& js : JobStats)
        {
            std::string procentStr;
            if( js.fileSize_ > 0)
            {
                char buf[64];
                if( std::snprintf(buf, sizeof(buf), " (%.2f %%)", 100.0 * js.bytesProcessed_ / js.fileSize_) > 0)
                    procentStr = buf;
            }

            TraceOut() << "File " << js.fileName_ << ": " << js.bytesProcessed_ << " / " << js.fileSize_ << procentStr;
        }

        TraceOut() << '\n' << SwappedAllocator->PrintState();
    }
}
catch(const std::exception& ex)
{
    TraceErr() << "Exception in PrintStateProc(): " << ex.what();
}
catch(...)
{
    TraceErr() << "Unknown exception in PrintStateProc()";
}

static void JobProc(const std::filesystem::path fn, FileProcessingStat& jobStat)
try
{
    TraceOut() << "Thread for processing file " << fn << " has been started";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, MaxBlockSize);

    using ThAllocatedBlocks = std::deque<BlockAddress>;
    ThAllocatedBlocks thBlocks;

    // Read input file
    std::ifstream is;
    is.open(inPath / fn, std::ios::binary | std::ios::in | std::ios::ate);
    if(!is)
    {
        TraceErr() << "ERROR at open input file " << fn;
        return;
    }

    jobStat.fileName_ = fn;
    jobStat.fileSize_ = is.tellg();
    is.seekg(0);

    while(!is.eof())
    {
        // std::this_thread::sleep_for(std::chrono::seconds(1));

        const std::streamsize rndLen = distrib(gen);
        char buf[MaxBlockSize];
        is.read(buf, rndLen);
        const std::streamsize realRdLen = is.gcount();
        if (realRdLen == 0)
            break;

        thBlocks.emplace_back(SwappedAllocator->MAlloc(buf, realRdLen));

        jobStat.bytesProcessed_ += realRdLen;
    }
    is.close();

    // Write output file
    std::ofstream os;
    os.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    os.open( outPath / fn, std::ios::binary | std::ios::out | std::ios::trunc);
    for(const auto& blk : thBlocks)
    {
        // std::this_thread::sleep_for(std::chrono::seconds(1));

        char buf[MaxBlockSize];
        SwappedAllocator->Free(blk, buf);
        os.write(buf, blk.len_);
        // TraceErr() << "Free: " << blk.len_;
    }
    os.close();

    TraceOut() << "Done " << fn;
}
catch(std::exception const& ex)
{
    TraceErr() << "Exception in thread for file " << fn << ": " << ex.what();
}
catch(...)
{
    TraceErr() << "Unknown exception in thread for file " << fn;
}

int main(int argc, char *argv[])
try
{
    if( argc < 2 )
    {
        throw std::runtime_error("Total pull size must be specified as the second argument (Mb)");
    }

    const int pullSizeMb = std::stol( argv[1] );
    assert( pullSizeMb > 0 );
    // TraceOut() << "Total pull size specified: " << pullSizeMb << " Mb";
    // SwappedAllocator.reset( CreateAllocator( size_t( pullSizeMb ) * 1024 * 1024 ) );
    TraceOut() << "Total pull size specified: " << pullSizeMb << " Kb";
    SwappedAllocator = CreateAllocator( size_t( pullSizeMb ) * 1024 );

    Threads jobs;

    std::vector<fs::path> filesToProcess;
    for (auto const& fn : fs::directory_iterator(inPath)->path().filename())
    {
        filesToProcess.push_back(fn);
    }

    // Сортируем файлы по имени
    std::sort(filesToProcess.begin(), filesToProcess.end());

    {
        TraceOut() << "Check the settings and press Enter...";
        std::string dummy;
        std::getline(std::cin, dummy);
    }

    jobs.reserve( filesToProcess.size() );
    for (auto const& fn : filesToProcess)
    {
        JobStats.emplace_back();
        jobs.push_back(std::thread(JobProc, fn, std::ref(JobStats.back())));
    }

    int breakBarrier = 2;
    std::thread prnStateJob;
    prnStateJob = std::thread(PrintStateProc, std::ref(breakBarrier));

    for (auto& j : jobs)
    {
        j.join();
    }

    breakBarrier = 1;
    if (prnStateJob.joinable())
    {
        TraceOut() << "Waiting for 'print state thread...'";
        prnStateJob.join();
    }

    return 0;
}
catch(std::exception const& ex)
{
    TraceErr() << "ERROR: " << ex.what();
    return 1;
}
catch(...)
{
    TraceErr() << "ERROR: unknown exception";
    return 2;
}
