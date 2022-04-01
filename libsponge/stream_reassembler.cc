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
	_eof_flag = eof;
	if (segmentLength == 0)
	{
		check_eof();
		return;
	}
	Block receivedSegment(index, index+segmentLength-1, segmentLength, data);
	size_t written;
	
	if (_expectedIndex >= index)	//No need to store into the streamBuffer
	{
		if (_expectedIndex > index)
		{
			if (_expectedIndex < index + segmentLength)
				receivedSegment._data.erase(receivedSegment._data.begin(), receivedSegment._data.begin()+_expectedIndex - index);
			else
			{
				check_eof();
				return;
			}
		}	
		if (!empty())
		{
			std::set<Block>::iterator itlow = _streamBuffer.begin();
			if (receivedSegment._end + 1 == itlow->_begin)
			{
				receivedSegment._data += itlow->_data;
				_streamBuffer.erase(itlow);
			}
			else if(receivedSegment._end >= (*itlow)._begin)
			{
				string temp = itlow->_data;
				temp.erase(temp.begin(), temp.begin()+receivedSegment._end+1 - itlow->_begin);
				receivedSegment._data += temp;
				_streamBuffer.erase(itlow);
			}
		}
		written = _output.write(receivedSegment._data);
		_expectedIndex += written;
	}
	else	//Put unassembled string into set
	{
		if (empty())
		{
			_streamBuffer.insert(receivedSegment);
			return;
		}
		Block truncatedSegment = {index, index+segmentLength-1, segmentLength, data};
		size_t unassembledByteLength = unassembled_bytes();
		if (segmentLength > _capacity - unassembledByteLength)
		{
			segmentLength = _capacity - unassembledByteLength;
			truncatedSegment._data.erase(truncatedSegment._data.begin()+segmentLength, truncatedSegment._data.end()); //change the length of segment to fit
			truncatedSegment._length = segmentLength;
			truncatedSegment._end = truncatedSegment._begin + segmentLength - 1;
		}
		std::set<Block>::iterator itlow = _streamBuffer.lower_bound(truncatedSegment);
		std::set<Block>::iterator itprev = prev(itlow, 1);
		std::set<Block>::iterator itnext = next(itlow, 1);
		if (itlow == _streamBuffer.end())
		{
			_streamBuffer.insert(truncatedSegment);
		}
		else if (itlow->_begin == truncatedSegment._begin)	//Overlap substring
		{
			if (itlow->_length < truncatedSegment._length)	//need update
			{
				if (_streamBuffer.size() == 1 || itnext == itlow) //Only one element
				{
					_streamBuffer.erase(itlow);
					_streamBuffer.insert(truncatedSegment);
				}
				else if (itprev == itlow)
				{
					_streamBuffer.erase(itlow);
					if (truncatedSegment._end >= itnext->_begin)
					{
						string temp;
						temp = itnext->_data;
						temp.erase(temp.begin(), temp.begin()+truncatedSegment._end+1-itnext->_begin);
						truncatedSegment._data += temp;
						truncatedSegment._end = itnext->_end;
						truncatedSegment._length = truncatedSegment._data.length();
					}
					_streamBuffer.insert(truncatedSegment);
				}
				else
				{
					Merge(truncatedSegment, itprev, itnext);
				}
				
			}
		}
		else if (itlow->_begin > truncatedSegment._begin)
		{
			Merge(truncatedSegment, itprev, itlow);
		}
	}	
	check_eof();

}


size_t StreamReassembler::unassembled_bytes() const 
{ 
	size_t unassembledByteLength = 0;
	for (set<Block>::iterator it = _streamBuffer.begin(); it != _streamBuffer.end(); ++it)
	{
		unassembledByteLength += (*it)._length;
	}
	return unassembledByteLength; 
}

bool StreamReassembler::empty() const { return (_streamBuffer.empty()); }

void StreamReassembler::Merge(Block &b1, set<Block>::iterator prev, set<Block>::iterator next)
{
	string temp;

	if (b1._end >= next->_begin)
	{
		temp = next->_data;
		temp.erase(temp.begin(), temp.begin()+b1._end +1 - next->_begin);
		b1._data += temp;
		b1._end = next->_end;
		b1._length = b1._data.length();
		_streamBuffer.erase(next);
	}
	if (prev->_end >= b1._begin && _streamBuffer.size()) //To detect whether there's only one element in the set
	{
		temp = prev->_data;
		b1._data.erase(b1._data.begin(), b1._data.begin()+prev->_end + 1 -b1._begin);
		temp += b1._data;
		b1._data = temp;
		b1._begin = prev->_begin;
		b1._length = b1._data.length();
		_streamBuffer.erase(prev);
	}
	_streamBuffer.insert(b1);
}
void StreamReassembler::check_eof()
{
	if (_eof_flag == true)
	{
		if (empty())
			_output.end_input();
	}

}