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
	/*
	传入数据之后存在以下几种可能性:
	1. _expectedIndex >= index && index + data.length > _expectedIndex
		此时需要将后面重复的数据去掉之后直接送进output,不需要往buffer中插入
	2. _expectedIndex < index
		此时需要将数据插入到buffer以便进行装配,
		(1).使用lowerbound函数获取对应的位置itlow, 返回的迭代器存在多种状态需要进行讨论
			a.	itlow->index <= index
				aa.	newblock.end > itlow->end
					去掉前半部分重复的字符串, 插入
				bb.	else
					丢弃, 因为完全重复了
			b. itlow == begin
				新插入的这个segment的index是最小的
		(2).使用upperbound函数获取对应的位置itup, 返回的迭代器存在多种状态需要进行讨论
			a.	itup->index > index
				aa.	newblock.end > itup->index
					去掉后半部分重复的字符串, 插入
				bb.	else
					说明后半部分没有重合
			b. itlow == end
				新插入的这个segment的index是最大的
	
	能否尽可能延迟插入的操作

	*/
	if (_expectedIndex + _capacity <= index)
	{
		return; //Segment过长了
	}

	if (eof)
	{
		_eof_flag = eof;
	}
	if (data.length() == 0)
	{
		check_eof();
		return;
	}

	Block receivedSegment = {index, index+data.length()-1, data.length(), data};
	if (index + data.length() <= _expectedIndex)	//完全重复
	{
		check_eof();
		return;
	}
	else if (index < _expectedIndex)
	{
		size_t duplicateLength = _expectedIndex - index;
		receivedSegment._begin = _expectedIndex;
		receivedSegment._data.assign(data.begin()+duplicateLength, data.end());
		receivedSegment._length = receivedSegment._data.length();
	}
	
	_unassembledBytes += receivedSegment._length;

	auto itlow = _streamBuffer.lower_bound(receivedSegment);
	long mergedBytes = 0;

	while (itlow != _streamBuffer.end() && (mergedBytes = Merge(receivedSegment, *itlow)) >= 0)
	{
		_unassembledBytes -= mergedBytes;
		_streamBuffer.erase(itlow);
		itlow = _streamBuffer.lower_bound(receivedSegment);
	}
	if (itlow != _streamBuffer.begin())
	{
		itlow--;
		while((mergedBytes = Merge(receivedSegment, *itlow)) >= 0)
		{
			_unassembledBytes -= mergedBytes;
			_streamBuffer.erase(itlow);
			itlow = _streamBuffer.lower_bound(receivedSegment);
			if (itlow == _streamBuffer.begin())
				break;
			itlow--;
		}
	}
	



	//尝试往_output中写
	size_t written;
	if (_expectedIndex == receivedSegment._begin)
	{
		written = _output.write(receivedSegment._data);
		_expectedIndex += written;
		_unassembledBytes -= written;
		if (written < receivedSegment._length) //_output现在满了，修改数据准备把它们存起来
		{
			receivedSegment._begin += written;
			receivedSegment._data.assign(receivedSegment._data.begin()+written, receivedSegment._data.end());
			receivedSegment._length = receivedSegment._data.length();
			_streamBuffer.insert(receivedSegment);
			_unassembledBytes += (receivedSegment._length);
			return;
		}
		else
		{
			
			check_eof();
			return;
		}

	}
	_streamBuffer.insert(receivedSegment);

	
}


size_t StreamReassembler::unassembled_bytes() const 
{ 
	return _unassembledBytes; 
}

bool StreamReassembler::empty() const { return (_streamBuffer.empty()); }

long StreamReassembler::Merge(Block &b1, const Block &b2)
{
	Block a, c;
	if (b1._begin > b2._begin)
	{
		a = b2;
		c = b1;
	}
	else
	{
		a = b1;
		c = b2;
	} 	
	if (a._end < c._begin)		//没有重叠的部分
	{
		if (a._end + 1 == c._begin)
		{
			a._data += c._data;
			a._end = c._end;
			a._length = a._data.length();
			b1 = a;
			return 0;
		}
		else
			return -1;
	}
	else if(a._end >= c._end)	//其中一个segment是另外一个segment的片段
	{
		b1 = a;
		return c._length;
	}
	else
	{
		string temp;
		size_t offset = a._end - c._begin + 1;
		temp.assign(c._data.begin()+offset, c._data.end());
		a._data += temp;
		a._end = c._end;
		a._length = a._data.length();
		b1 = a;
		return offset;
	}
}
void StreamReassembler::check_eof()
{
	if (_eof_flag == true && empty())
	{
		_output.end_input();
	}

}