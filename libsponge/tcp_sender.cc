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
    if (_syn == false)
    {
        //Not establish connection yet, make first hand-shake
        sending_segment.header().syn = true;
        sending_segment.header().seqno = _isn;
        _next_seqno++;
        _bytes_in_flight++;
        _syn = true;
        _segments_out.push(sending_segment);
        _outstanding_segments.push(sending_segment);
        if (_timer_is_running == false)
        {
            //If the timer is not running, set the timer running
            _timer_is_running = true;
            _timer = 0;
        }
        return;
    }
    size_t window = _window_size == 0 ? 1 : _window_size;
    window = window > TCPConfig::MAX_PAYLOAD_SIZE ? TCPConfig::MAX_PAYLOAD_SIZE : window;
    string payload = _stream.read(window);
    sending_segment.payload() = Buffer(std::move(payload));
    size_t length;
    //Now to determine the header
    if (_timer_is_running == false)
    {
        //If the timer is not running, set the timer running
        _timer_is_running = true;
        _timer = 0;
    }
    if (_stream.input_ended())
    {
        //Need to set FIN flag in the header
        sending_segment.header().fin = true;
        _fin = true;
    }
    if (sending_segment.length_in_sequence_space() == 0)
    {
        return;
    }
    sending_segment.header().seqno = wrap(_next_seqno, _isn);
    length = sending_segment.length_in_sequence_space();
    _next_seqno += length;
    _bytes_in_flight += length;
    _window_size -= length;
    _segments_out.push(sending_segment);
    //Save a copy in buffer in case of retransmission
    _outstanding_segments.push(sending_segment);
    
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
            
        }while (!_outstanding_segments.empty());
    }
    if (_outstanding_segments.empty())
    {
        //All sent segments have been ACKed, stop the timer
        _retransmission_timeout = _initial_retransmission_timeout;
        _timer_is_running = false;
        _timer = 0;
        _consecutive_retransmission_count = 0;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) 
{ 
    TCPSegment seg;

    if (_timer_is_running)
    {
        _timer += ms_since_last_tick;
        if (_timer >= _retransmission_timeout)
        {
            //Timer has expired, retansmit the earliest segment again
            seg = _outstanding_segments.front();
            _segments_out.push(seg);
            //_bytes_in_flight += seg.length_in_sequence_space();
            _consecutive_retransmission_count++;
            _retransmission_timeout *= 2;
            _timer = 0;
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission_count; }

void TCPSender::send_empty_segment() 
{
    TCPSegment sending_segment;
    sending_segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(sending_segment);
}
