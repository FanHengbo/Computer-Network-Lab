#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`


using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
	Capacity = capacity;
	Read = 0;
	Written = 0;
	q = {};
	input_end_flag = false;
	}

size_t ByteStream::write(const string &data) {
    size_t length = data.length();
    if (length > Capacity - q.size())
    {
    	length = Capacity - q.size();
    }
    
	Written += length;
	for (size_t i = 0; i < length; i++)
		q.push_back(data[i]);
	return length;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
	size_t length = len;
	if (len > q.size())
		length = q.size();

	string output = "";
	for (size_t i = 0;i < length;i++)
	{
		output += q[i];
	}
    return output; 
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
	size_t  length = len;
	if (len > q.size())
		length = q.size();
	Read += length;
	for (size_t i = 0 ;i < length;i++)
	{
		q.pop_front();
	}

}
//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t  length = len;
	if (len > q.size())
		length = q.size();
	string output = "";
	while(length--)
	{
		output += q[0];
		q.pop_front();
	}
	Read += length;
	return output; 
}

void ByteStream::end_input() {input_end_flag = true;}

bool ByteStream::input_ended() const { return  input_end_flag; }

size_t ByteStream::buffer_size() const { return q.size();}

bool ByteStream::buffer_empty() const { return q.empty();}

bool ByteStream::eof() const { return q.empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return Written; }

size_t ByteStream::bytes_read() const { return Read; }

size_t ByteStream::remaining_capacity() const { return Capacity - q.size(); }
