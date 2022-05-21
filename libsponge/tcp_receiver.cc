#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    bool SYN = seg.header().syn;
    bool FIN = seg.header().fin;
    size_t segmentLength = seg.length_in_sequence_space();
    uint64_t index;
    uint64_t streamIndex;
    size_t expectedIndex;
    if (SYN && _SYN == false)
    {
        _SYN = true;
        _isn = seg.header().seqno;
        if (FIN && _FIN == false && segmentLength == 2)
        {
            //Only set SYN and FIN bits, no payload
            _FIN = true;
            _reassembler.push_substring("", 0, _FIN);
            return;
        }
    }
    if (_SYN)
    {
        index = unwrap(seg.header().seqno, _isn, _checkpoint);
        if (index == 0)
        {
            if (segmentLength == 1)
            {
                //First segment with SYN bit set, but no payload
                return;
            }
            else
            {
                streamIndex = 0;
            }
        }
        else
        {   
            streamIndex = index - 1;
        }
        string data = seg.payload().copy();
        if (FIN)
        {
            _FIN = true;
        }
        _reassembler.push_substring(data, streamIndex, FIN);
        expectedIndex = _reassembler.get_expected_index();
        _checkpoint = expectedIndex == 0? 0 : expectedIndex - 1;
    }
    
}

optional<WrappingInt32> TCPReceiver::ackno() const 
{
    size_t expectedIndex = _reassembler.get_expected_index();

    if (_SYN == false)
        return nullopt;
    else if (_FIN != true)                      //Not receive FIN flag yet
        return wrap(expectedIndex + 1, _isn);   // convert from stream index to absolute sequno, then wrap to sequno
    else
    {
        if (!_reassembler.empty())
        {
            return wrap(expectedIndex + 1, _isn);
        }
        else
            return wrap(expectedIndex + 2, _isn);
    }
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.unread_bytes(); }
