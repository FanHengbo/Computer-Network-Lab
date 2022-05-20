#include "wrapping_integers.hh"
#include <cmath>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
   return WrappingInt32(static_cast<uint32_t>(n) + isn.raw_value()); 
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t offset;
    uint64_t high;
    uint64_t absoluteSeq;
    offset = n - isn;
    high = checkpoint >> 32;
    if (high)
    {
        //高32位有值
        absoluteSeq = (high << 32) + offset;
        if (absoluteSeq < checkpoint && checkpoint - absoluteSeq > INT32_MAX)
        {
            //尽管已经提取出了checkpoint的高32位，但是差还是大于INT32_MAX，说明需要再往上取
            high++;
        }
        else if (absoluteSeq > checkpoint && absoluteSeq - checkpoint > INT32_MAX)
        {
            //取过头了，往下舍去
            high--;
        }
        absoluteSeq = (high << 32) + offset;
    }
    else
    {    
        absoluteSeq = offset;
        if (absoluteSeq < checkpoint && checkpoint - absoluteSeq > INT32_MAX)
        {
            high++;
            absoluteSeq = (high << 32) + offset;
        }
    }
    return absoluteSeq;
}
