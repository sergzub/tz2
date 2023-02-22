#include "trace.h"

#include <mutex>

#include <iomanip>
#include <iostream>


template <TraceDirection TD>
TraceT<TD>::~TraceT()
try
{
    static std::mutex mtx;

    auto& os = []() -> std::ostream& {
        if constexpr (TD == TraceDirection::STDOUT)
        {
            return std::cout;
        }
        else // TD == TraceDirection::STDERR
        {
            return std::cerr;
        }
    }();

    {
        std::lock_guard<std::mutex> guard(mtx);

        os << m_ss.rdbuf();
        os << '\n';
        os.flush();
    }
}
catch (...)
{
}

template class TraceT<TraceDirection::STDOUT>;
template class TraceT<TraceDirection::STDERR>;
