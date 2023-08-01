#ifndef CHUNKEDDATASENDER_H
#define CHUNKEDDATASENDER_H

#include <cstddef>
#include <fstream>

#include <string>
// C++ extra libraries
#include <boost/filesystem.hpp>

const size_t CHUNK_SIZE = 4096;
namespace fs = boost::filesystem;

/**
 * An interface for sending data in fixed-sized chunks over a network socket.
 * This interface contains one function, send_next_chunk, which should send
 * the next chunk of data.
 */
class ChunkedDataSender {
  public:
	virtual ~ChunkedDataSender() {}

	virtual ssize_t send_next_chunk(int sock_fd) = 0;
};

/**
 * Class that allows sending a file over a network socket
 */ 
class FileSender : public virtual ChunkedDataSender {
	private:
		size_t length;	// the size of the file
		std::string file;	// the name of the file
		int stream_pos;		// the position of the stream "cursor" in the file
		std::ifstream ifs;	// the input filestream

	public:
		/**
		 * Constructor for FileSender object
		 * @param mp3_path The filepath to the mp3 file to send
		 */ 
		FileSender(fs::path mp3_path);
		~FileSender() { }	// destructor
		virtual ssize_t send_next_chunk(int sock_fd);
};




/**
 * Class that allows sending an array of over a network socket.
 * @note: ArraySender is unused in our final version
 */
class ArraySender : public virtual ChunkedDataSender {
  private:
	char *array; // the array of data to send
	size_t array_length; // length of the array to send (in bytes)
	size_t curr_loc; // index in array where next send will start

  public:
	/**
	 * Constructor for ArraySender class.
	 */
	ArraySender(const char *array_to_send, size_t length);

	/**
	 * Destructor for ArraySender class.
	 */
	~ArraySender() {
		delete[] array;
	}

	/**
	 * Sends the next chunk of data, starting at the spot in the array right
	 * after the last chunk we sent.
	 *
	 * @param sock_fd Socket which to send the data over.
	 * @return -1 if we couldn't send because of a full socket buffer,
	 * 	otherwise the number of bytes actually sent over the socket.
	 */
	virtual ssize_t send_next_chunk(int sock_fd);
};

#endif // CHUNKEDDATASENDER_H
