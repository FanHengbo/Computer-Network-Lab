#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
	size_t segmentLength = data.length();
	Block receivedSegment(index, index+segmentLength-1, index, data);

	if (_expectedIndex == index)	//No need to store into the streamBuffer
	{
		std::set<Block>::iterator itlow = _streamBuffer.lower_bound(receivedSegment);
		if(receivedSegment.end + 1 == *itlow.index)
		{
			receivedSegment.end = *itlow.end;
			receivedSegment.data += *itlow.data;
			_streamBuffer.erase(itlow);
		}
		_output.write(receivedSegment.data);
		_expectedIndex += _output.remaining_capacity();
	}
	else
	{
		std::pair<std::set<Block>::iterator, bool> ret;
		ret = _streamBuffer.insert(receivedSegment);
		std::set<Block>::iterator it = ret.first;
		if (it == _streamBuffer.begin())
		if(receivedSegment.end + 1 == *itlow.index)
		{
			receivedSegment.end = *itlow.end;
			receivedSegment.data += *itlow.data;
			_streamBuffer.erase(itlow);
			_streamBuffer.insert(receivedSegment);
		}
	}

}


size_t StreamReassembler::unassembled_bytes() const { return {}; }

bool StreamReassembler::empty() const { return {}; }
