﻿#include "trace.h"
#include "alloc.h"

#include <string>
#include <iostream>
#include <filesystem>
#include <list>
#include <chrono>
#include <thread>
#include <random>
#include <fstream>
#include <memoty>


namespace fs = std::filesystem;
using Threads = std::list<std::thread>;

const std::filesystem::path inPath  = "../in";
const std::filesystem::path outPath = "../out";
const size_t MaxBlockSize = 4096;

static std::unique_ptr<IAllocator> SwappedAllocator;

using MyUniquePtr = std::unique_ptr<void, TZ2::free>;
using ThFileMemBlocks = std::list<MyUniquePtr>;

static void JobProc(const std::filesystem::path fn)
try
{
    TraceOut() << "Thread for processing file " << fn << " has been started";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, MaxBlockSize);

    ThFileMemBlocks blocks;

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
        const std::streamsize rndLen = distrib(gen);
        char buf[MaxBlockSize];
        is.read(buf, rndLen);
        const std::streamsize realRdLen = is.gcount();
        if ( realRdLen == 0)
            break;
        
        ThFileMemBlocks& b = blocks.emplace_back(realRdLen)
        TZ2::alloc()

    }

    //////////////////////////////////////////////////////////

    // Write output file
    // std::ifstream os;
    // os.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    // os.open( outPath / fn, std::ios::binary | std::ios::out | std::ios::trunc);

    TraceOut() << "Done " << fn;
}
catch(std::exception const& ex)
{
    TraceErr() << "Exception in thread for file " << fn << ": " << ex.what();
}
catch(...)
{
    TraceErr() << "Unknown уxception in thread for file " << fn;
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
    TraceOut() << "Total pull size specified: " << pullSizeMb << " Mb";
    SwappedAllocator.reset( CreateAllocator( size_t( pullSizeMb ) * 1024 * 1024 ) );

    Threads jobs;

    for (auto const& fn : fs::directory_iterator(inPath)->path().filename())
    {
        jobs.push_back(std::thread(JobProc, fn));
        break; // create just one thread
    }

    for (auto& j : jobs)
    {
        j.join();
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