#ifndef CONNECTEDCLIENT_H
#define CONNECTEDCLIENT_H

#include <string>
#include <vector>
using std::string;

/**
 * Represents the state of a connected client.
 */
enum ClientState { RECEIVING, SENDING };

enum MessageType {PLAY, STOP, LIST, INFO, HELLO, PROMPT, OK, BAD};

/**
 * A structure to contain the mp3 file and any associted info
 */ 
struct music
{
	fs::path mp3;
	string info;
};

/**
 * Class that models a connected client.
 * 
 * One object of this class will be created for every client that you accept a
 * connection from.
 */
class ConnectedClient {
  public:
	// Member Variables (i.e. fields)
	int client_fd;
	ChunkedDataSender *sender;
	ClientState state;

	// Constructors
	/**
	 * Constructor that takes the client's socket file descriptor and the
	 * initial state of the client.
	 */
	ConnectedClient(int fd, ClientState initial_state) : 
		client_fd(fd), sender(NULL), state(initial_state), stream_is_open(false) {}

	// No-argument constructor
	ConnectedClient() : client_fd(-1), sender(NULL), state(RECEIVING), stream_is_open(false) {}

	// Member Functions (i.e. Methods)
	
	/**
	 * Sends a response to the client.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 * @param mp3path The directory in which to search for mp3's
	 * @note mp3path must be a directory
	 */
	void send_response(int epoll_fd, fs::path mp3path);

	/**
	 * Sends a set of info to the client.
	 * 
	 * @param epoll_fd File descriptor for epoll.
	 * @param data A char array of data to send
	 * @param data_length The length of data in bytes
	 **/ 
	void send_info(int epoll_fd, char *data, int data_length);

	/**
	 * Generates the info from the music library if info exists and puts it in 
	 * string format.
	 * 
	 * @param int selection The number called to get info
	 * @param vector<struct music> song_list The vector containing the music info
	 * 
	 * @return the string containing the called info
	 */
	string generate_info(int selection, std::vector<struct music> song_list);
	
	/**
	 * Continue a response if client side buffer is full
	 * 
	 * @param epoll_fd The epoll file descriptor
	 */
	void continue_response(int epoll_fd);

	/**
	 * Generates a list of all available songs
	 * 
	 * @param string directory_path the path to the music root folder
	 * @return A string representation of the list
	 */ 
	std::string generate_song_list(std::vector<struct music> song_list);
	
	/**
	 * Handles new input from the client.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void handle_input(int epoll_fd, std::vector<struct music> song_list);

	/**
	 * Handles a close request from the client.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void handle_close(int epoll_fd);

  private: 
    /**
	 * Private member variables 
	 */
	bool stream_is_open;

	

	/**
	 * Converts a plain string to it's corresponding enum type
	 * 
	 * @param s Plain string
	 * @return MessageType enumerated value corresponding to the plain string argument
	**/
	MessageType string_to_enum(std::string s);
};

#endif
