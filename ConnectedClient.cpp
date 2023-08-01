#include <iostream>

#include <cstring>

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <vector>
#include <sstream>
#include <boost/algorithm/string.hpp>

#include "ChunkedDataSender.h"
#include "ConnectedClient.h"

namespace fs = boost::filesystem;
using boost::algorithm::split;
using std::cout;
using std::cerr;
using std::string;
using std::vector;
using std::ostringstream;

void ConnectedClient::send_response(int epoll_fd, fs::path mp3_path) {
	// Create a large array, just to make sure we can send a lot of data in
	// smaller chunks.
	cout << mp3_path << '\n';
	FileSender *fs = new FileSender(mp3_path);

	ssize_t num_bytes_sent;
	ssize_t total_bytes_sent = 0;

	// keep sending the next chunk until it says we either didn't send
	// anything (0 return indicates nothing left to send) or until we can't
	// send anymore because of a full socket buffer (-1 return value)
	while((num_bytes_sent = fs->send_next_chunk(this->client_fd)) > 0 && this->stream_is_open) {
		total_bytes_sent += num_bytes_sent;
	}
	if(!this->stream_is_open) {
		total_bytes_sent += num_bytes_sent;
	}
	cout << "sent " << total_bytes_sent << " bytes to client\n";

	/*
	 * If the last call to send_next_chunk indicated we couldn't send
	 * anything because of a full socket buffer, we should do the following:
	 *
	 * 1. update our state field to be sending
	 * 2. set our sender field to be the ArraySender object we created
	 * 3. update epoll so that it also watches for EPOLLOUT for this client
	 *    socket (use epoll_ctl with EPOLL_CTL_MOD).
	 */
	if (num_bytes_sent < 0) {
		cout << "Waiting" << '\n';
		this->state = RECEIVING;
		this->sender = fs;
		epoll_ctl(epoll_fd, EPOLL_CTL_MOD, this->client_fd, NULL);
		
	}
	else {
		// Sent everything with no problem so we are done with our FileSender
		// object.
		delete fs;
	}
}

void ConnectedClient::send_info(int epoll_fd, char *data, int data_length) {
	// Create a large array, just to make sure we can send a lot of data in
	// smaller chunks.
	char *data_to_send = new char[data_length];
	memcpy(data_to_send, data, data_length);

	ArraySender *as = new ArraySender(data_to_send, data_length);

	ssize_t num_bytes_sent;
	ssize_t total_bytes_sent = 0;

	// keep sending the next chunk until it says we either didn't send
	// anything (0 return indicates nothing left to send) or until we can't
	// send anymore because of a full socket buffer (-1 return value)
	while((num_bytes_sent = as->send_next_chunk(this->client_fd)) > 0) {
		total_bytes_sent += num_bytes_sent;
	}
	if(!this->stream_is_open) {
		total_bytes_sent += num_bytes_sent;
	}
	cout << "sent " << total_bytes_sent << " bytes to client\n";
	delete[] data_to_send;
	/*
	 * If the last call to send_next_chunk indicated we couldn't send
	 * anything because of a full socket buffer, we do the following:
	 *
	 * 1. update our state field to be sending
	 * 2. set our sender field to be the ArraySender object we created
	 * 3. update epoll so that it also watches for EPOLLOUT for this client
	 *    socket (use epoll_ctl with EPOLL_CTL_MOD).
	 */
	if (num_bytes_sent < 0) {
		cout << "Waiting" << '\n';
		struct epoll_event client_ev;
		client_ev.events = EPOLLIN | EPOLLRDHUP | EPOLLOUT;
		this->state = RECEIVING;
		this->sender = as;
		if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, this->client_fd, &client_ev) == -1) {
			perror("handle_close epoll_ctl");
			exit(EXIT_FAILURE);
		}
	}
	else {
		// Sent everything with no problem so we are done with our FileSender
		// object.
		delete as;
	}
}


MessageType ConnectedClient::string_to_enum(string s) {
	if(s == "PLAY") {
		return PLAY;
	}
	else if(s == "STOP") {
		return STOP;
	}
	else if(s == "LIST") {
		return LIST;
	}
	else if(s == "INFO") {
		return INFO;
	}
	else if(s == "HELLO") {
		return HELLO;
	}
	else if(s == "PROMPT") {
		return PROMPT;
	}
	else {
		return PROMPT;
	}
}


void ConnectedClient::handle_input(int epoll_fd, vector<struct music> song_list) {
	cout << "Ready to read from client " << this->client_fd << "\n";

	// receive data from the client
	char data[1024];
	memset(data, 0, 1024);
	ssize_t bytes_received = recv(this->client_fd, data, 1024, 0);
	if (bytes_received < 0) {
		perror("client_read recv");
		exit(EXIT_FAILURE);
	}

	cout << "Received data: ";
	for (int i = 0; i < bytes_received; i++) {
		cout << data[i];
	}
	cout << "\n";

	// parse the command
	string command(data, bytes_received);
	vector<string> split_command;
	boost::split(split_command, command, [](char c){return c == ' ';});
	switch (string_to_enum(split_command[0]))
	{
		case PLAY:
		{
			// Get a reference to the mp3 filepath
			unsigned index = stoi(split_command[1]);
			if(index < song_list.size()) {
				fs::path mp3_path = song_list[index].mp3;
				// send the response
				this->stream_is_open = true;
				this->send_response(epoll_fd, mp3_path);
				cout << "Playing song!" << '\n';
			}
			break;
		}
		case STOP:
			// close the stream with member variable
			this->stream_is_open = false;
			break;
		case LIST:
		{
			// generate a header and response, then send
			string raw_list = this->generate_song_list(song_list);
			string list = std::to_string(raw_list.size()) + "\n" + raw_list;
			char newList[list.size() + 1];
			memset(newList, 0, list.size()+1);
			strcpy(newList, list.c_str()); 
			this->send_info(epoll_fd, newList, sizeof(newList));
			this->stream_is_open = false;	// close stream
			break;
		}
		case INFO:
		{
			// parse input index
			int index = stoi(split_command[1]);

			// generate a header and response, then send
			string raw_info = this->generate_info(index, song_list);
			string info = std::to_string(raw_info.size()) + "\n" + raw_info;
			char infoChar[info.size()+1];
			memset(infoChar, 0, info.size()+1);
			strcpy(infoChar, info.c_str());
			this->send_info(epoll_fd, infoChar, sizeof(infoChar));
			break;
		}	
		case PROMPT:
		default:
			break;
	}
}


void ConnectedClient::continue_response(int epoll_fd) {
	ssize_t num_bytes_sent;
	ssize_t total_bytes_sent = 0;

	while((num_bytes_sent = this->sender->send_next_chunk(this->client_fd)) > 0) {
		total_bytes_sent += num_bytes_sent;
	}
	cout << "sent " << total_bytes_sent << " bytes to client\n";

	if(num_bytes_sent < 0) {
		this->state = RECEIVING;
		epoll_ctl(epoll_fd, EPOLL_CTL_MOD, this->client_fd, NULL);
	}
	else {
		delete this->sender;
	}	
}


string ConnectedClient::generate_song_list(vector<struct music> song_list) {
	ostringstream print_statement;
	print_statement << "Songs available to stream:\n";
	for(unsigned i = 0; i < song_list.size(); i++) {
		print_statement << '\t' << i << ") " << song_list[i].mp3.filename().string() << "\n";
	}
	return print_statement.str();
}


string ConnectedClient::generate_info(int selection, vector<struct music> song_list) {
	if((unsigned) selection < song_list.size()) {
		if(!song_list[selection].info.empty()) {
			return song_list[selection].info;
		}
		else {
			return "Info could not be found!";
		}
	}
	else {
		return "Invalid song number!";
	}
}

void ConnectedClient::handle_close(int epoll_fd) {
	cout << "Closing connection to client " << this->client_fd << "\n";

	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, this->client_fd, NULL) == -1) {
		perror("handle_close epoll_ctl");
		exit(EXIT_FAILURE);
	}

	close(this->client_fd);
}

