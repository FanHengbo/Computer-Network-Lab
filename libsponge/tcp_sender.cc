#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , _retransmission_timeout{retx_timeout}
    {}

uint64_t TCPSender::bytes_in_flight() const 
{ 
    return _bytes_in_flight; 
}

void TCPSender::fill_window() 
{
    TCPSegment sending_segment;
    size_t window = _window_size == 0 ? 1 : _window_size;
    while (window > _bytes_in_flight)
    {
        if (!_syn)
        {
            sending_segment.header().syn = true;
            _syn = true;
        }

        sending_segment.header().seqno = next_seqno();
        size_t payload_size = min(TCPConfig::MAX_PAYLOAD_SIZE, window - _bytes_in_flight - sending_segment.header().syn);
        string payload = _stream.read(payload_size);
        size_t length;
        sending_segment.payload() = Buffer(std::move(payload));
        if(!_fin && _stream.input_ended() && window > _bytes_in_flight + sending_segment.payload().size())
        {
            _fin = true;
            sending_segment.header().fin = true;
        }
        if ((length = sending_segment.length_in_sequence_space()) == 0)
        {
            //Abosultely nothing
            return;
        }
        if (_outstanding_segments.empty())
        {
            //If the outstanding_segment set is empty, set the timer running
            _timer = 0;
            _retransmission_timeout = _initial_retransmission_timeout;
        }
        _next_seqno += length;
        _bytes_in_flight += length;
        _segments_out.push(sending_segment);
        _outstanding_segments.push(sending_segment);
        
        if (_fin)
        {
            //Already sent FIN
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) 
{
    _window_size = window_size; 
    TCPSegment seg;
    if (!_outstanding_segments.empty())
    {
        seg = _outstanding_segments.back();
        if (seg.header().seqno.raw_value() + seg.length_in_sequence_space() < ackno.raw_value())
        {
            //Ackno is too large, ignore it
            return;
        }
        do
        {
            seg = _outstanding_segments.front();
            if (seg.header().seqno.raw_value() + seg.length_in_sequence_space() > ackno.raw_value())
            {
                break;
            }
            _bytes_in_flight -= seg.length_in_sequence_space();
            _outstanding_segments.pop();
            _timer = 0;
            _retransmission_timeout = _initial_retransmission_timeout;
        }while (!_outstanding_segments.empty());
    }
    _consecutive_retransmission_count = 0;

    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) 
{ 
    TCPSegment seg;
    _timer += ms_since_last_tick;
    if (!_outstanding_segments.empty() && _timer >= _retransmission_timeout)
    {
        //Timer has expired, retansmit the earliest segment again
        seg = _outstanding_segments.front();
        _segments_out.push(seg);
        _consecutive_retransmission_count++;
        if (_window_size > 0 || seg.header().syn == true)
        {
            //Only double RTO when _window_size > 0, because when _window_size == 0, all sending one-byte segment acts like a probe.
            //However, in the case that sender hasn't received any ACK from receiver, we need to retransmit SYN constantly until get a ACK. 
            //_window_size is also 0 in this circumstance, but we need double the RTO.
            _retransmission_timeout *= 2;
        }
        _timer = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission_count; }

void TCPSender::send_empty_segment() 
{
    TCPSegment sending_segment;
    sending_segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(sending_segment);
}
