#pragma once

#include <sstream>

enum TraceDirection
{
    STDOUT,
    STDERR
};

template <TraceDirection TD>
class TraceT
{
public:
    TraceT() = default;
    ~TraceT();

    template <typename T>
    std::ostream& operator<<(const T& t)
    {
        return m_ss << t;
    }

private:
    std::stringstream m_ss;
};

using TraceOut = TraceT<TraceDirection::STDOUT>;
using TraceErr = TraceT<TraceDirection::STDERR>;
