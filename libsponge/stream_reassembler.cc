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
	Block receivedSegment(index, index+segmentLength-1, segmentLength, data);
	size_t written;
	if (_expectedIndex == index)	//No need to store into the streamBuffer
	{
		std::set<Block>::iterator itlow = _streamBuffer.lower_bound(receivedSegment);
		if(receivedSegment.end + 1 == (*itlow).begin)
		{
			receivedSegment.end = (*itlow).end;
			receivedSegment.data += (*itlow).data;
			segmentLength += (*itlow).data.length();
			_streamBuffer.erase(itlow);
		}
		written = _output.write(receivedSegment.data);
		//We didn't consider the situation that written < segmentLength
		_expectedIndex += written;
	}
	else	//Put unassembled string into set
	{
		size_t unassembledByteLength = unassembled_bytes();
		if (segmentLength > _capacity - unassembledByteLength)
		{
			segmentLength = _capacity - unassembledByteLength;
		}
		
		Block receivedSegment(index, index+segmentLength-1, segmentLength, data);
		receivedSegment.data.erase(segmentLength, data.length()); //change the length of segment to fit
		std::pair<set<Block>::iterator, bool> ret;
		ret = _streamBuffer.insert(receivedSegment);
		if (ret.second == false) //overlapping substring
		{
			return;
		}
		std::set<Block>::iterator it = ret.first;
		std::set<Block>::iterator itprev = prev(it, 1);
		std::set<Block>::iterator itnext = next(it, 1);
		Block mergeSegment;
		if (it == _streamBuffer.begin())
		{
			if((*it).end >= (*itnext).begin)	//overlapping substring
			{
				_streamBuffer.erase(it);
				return;
			}
			if((*it).end + 1 == (*itnext).begin) //Merge
			{
				mergeSegment = (*it);
				Merge(mergeSegment, *itnext);
				_streamBuffer.erase(it);
				_streamBuffer.erase(itnext);
				_streamBuffer.insert(mergeSegment);
			}
			
		}
		else if (it == _streamBuffer.end())
		{
			if ((*itprev).end >= (*it).begin)	//overlapping substring
			{
				_streamBuffer.erase(it);
				return;
			}
			if ((*itprev).end + 1 == (*it).begin)
			{
				mergeSegment = (*itprev);
				Merge(mergeSegment, *it);
				_streamBuffer.erase(it);
				_streamBuffer.erase(itprev);
				_streamBuffer.insert(mergeSegment);
			}
		}
		else
		{
			
			if ((*itprev).end >= (*it).begin || (*it).end >= (*itnext).begin)	//overlapping substring
			{
				_streamBuffer.erase(it);
				return;
			}
			else if ((*itprev).end + 1 == (*it).begin)
			{
				mergeSegment = (*itprev);
				Merge(mergeSegment, *it);
				_streamBuffer.erase(itprev);
				
				if ((*it).end + 1 == (*itnext).begin)	//perfectly fill the gap
				{
					Merge(mergeSegment, *itnext);
					_streamBuffer.erase(itnext);
				}
				_streamBuffer.erase(it);
				_streamBuffer.insert(mergeSegment);
			}
			else if ((*it).end + 1 == (*itnext).begin)
			{
				mergeSegment = (*it);
				Merge(mergeSegment, *itnext);
				_streamBuffer.erase(it);
				_streamBuffer.erase(itnext);
				_streamBuffer.insert(mergeSegment);
			}
		}
			
	}

}


size_t StreamReassembler::unassembled_bytes() const 
{ 
	size_t unassembledByteLength = 0;
	for (set<Block>::iterator it = _streamBuffer.begin(); it != _streamBuffer.end(); ++it)
	{
		unassembledByteLength += (*it).length;
	}
	return unassembledByteLength; 
}

bool StreamReassembler::empty() const { return !_streamBuffer.size(); }

void Merge(Block &e1, const Block &e2)
{
	e1.end = e2.end;
	e1.data += e2.data;
	e1.length += e2.length;
}