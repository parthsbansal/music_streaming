#include <algorithm>

#include <cstring>
#include <cerrno>
#include <cstdio>

#include <fstream>
#include <iostream>
// C++ extra libraries
#include <boost/filesystem.hpp>

#include <sys/types.h>
#include <sys/socket.h>

#include "ChunkedDataSender.h"

namespace fs = boost::filesystem;

/*
Note: ArraySender Object can be largely ignored 
*/
ArraySender::ArraySender(const char *array_to_send, size_t length) {
	this->array = new char[length];
	std::copy(array_to_send, array_to_send+length, this->array);
	this->array_length = length;
	this->curr_loc = 0;
}

ssize_t ArraySender::send_next_chunk(int sock_fd) {
	// Determine how many bytes we need to put in the next chunk.
	// This will be either the CHUNK_SIZE constant or the number of bytes left
	// to send in the array, whichever is smaller.
	size_t num_bytes_remaining = array_length - curr_loc;
	size_t bytes_in_chunk = std::min(num_bytes_remaining, CHUNK_SIZE);

	if (bytes_in_chunk > 0) {
		// Create the chunk and copy the data over from the appropriate
		// location in the array
		char chunk[CHUNK_SIZE];
		memcpy(chunk, array+curr_loc, bytes_in_chunk);

		ssize_t num_bytes_sent = send(sock_fd, chunk, bytes_in_chunk, 0);

		if (num_bytes_sent < 0 && errno != EAGAIN) {
			perror("send_next_chunk send");
			exit(EXIT_FAILURE);
		}
		else if (num_bytes_sent > 0) {
			// We successfully send some of the data so update our location in
			// the array so we know where to start sending the next time we
			// call this function.
			curr_loc += num_bytes_sent;
		}
		return num_bytes_sent;
	}
	else {
		return 0;
	}
}


FileSender::FileSender(fs::path mp3_path) {
	this->length = fs::file_size(mp3_path);
	this->file = mp3_path.filename().string();
	this->stream_pos = 0;
	this->ifs = std::ifstream(mp3_path.string());
}

ssize_t FileSender::send_next_chunk(int sock_fd) {
	// Determine how many bytes we need to put in the next chunk.
	// This will be either the CHUNK_SIZE constant or the number of bytes left
	// to send in the array, whichever is smaller.
	size_t num_bytes_remaining = this->length - this->stream_pos;
	size_t bytes_in_chunk = std::min(num_bytes_remaining, CHUNK_SIZE);
	
	
	if(!this->ifs.eof()) {
		if (bytes_in_chunk > 0) {
			// Create the chunk and copy the data over from the appropriate
			// location in the array
			char chunk[CHUNK_SIZE];
			this->ifs.seekg(this->stream_pos);
			this->ifs.read(chunk, CHUNK_SIZE);
			ssize_t num_bytes_sent = send(sock_fd, chunk, bytes_in_chunk, 0);

			if (num_bytes_sent < 0 && errno != EAGAIN) {
				perror("send_next_chunk send");
				exit(EXIT_FAILURE);
			}
			else if (num_bytes_sent > 0) {
				// We successfully send some of the data so update our location in
				// the array so we know where to start sending the next time we
				// call this function.
				this->stream_pos += num_bytes_sent;
			}
			return num_bytes_sent;
		}
		else {
			return 0;
		}
	}
	else{
		return 0;
	}
}
