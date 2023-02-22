#include "trace.h"
#include "alloc.h"

#include <string>
#include <iostream>
#include <filesystem>
#include <algorithm>
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

static bool breakFlag = false;
static void PrintStateProc()
try
{
    while(!breakFlag)
    {
        const auto t = std::time(nullptr);
        const auto tm = *std::localtime(&t);

        TraceOut() << "\f"
            << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << '\n'
            << SwappedAllocator->PrintState();
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
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

static void JobProc(const std::filesystem::path fn)
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
    is.open(inPath / fn, std::ios::binary | std::ios::in);
    if(!is)
    {
        TraceErr() << "ERROR at open input file " << fn;
        return;
    }
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
        // TraceErr() << "Allocated: " << realRdLen;
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

    jobs.reserve( filesToProcess.size() );
    for (auto const& fn : filesToProcess)
    {
        jobs.push_back(std::thread(JobProc, fn));
    }

    std::thread prnStateJob;
    prnStateJob = std::thread(PrintStateProc);

    for (auto& j : jobs)
    {
        j.join();
    }

    breakFlag = true;
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
