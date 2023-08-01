package edu.sandiego.comp375.jukebox;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.InputStreamReader;
import java.net.Socket;
import java.util.Scanner;

/**
 * Class representing a client to our Jukebox.
 * Proper usage to launch java code: 
 * java -cp \"target/*\" edu.sandiego.comp375.jukebox.AudioClient' [ip-address] [port-num]
 */
public class AudioClient {
	public static void main(String[] args) throws Exception {
		// Validate command line usage
		if(args.length != 2)
		{
			System.out.println("Error: Invalid usage.");
			System.out.println("Proper usage: java -cp \"target/*\" edu.sandiego.comp375.jukebox.AudioClient' [ip-address] [port-num]");
			System.exit(-1);
		}

		// Parse ip and port number from command line
		final String ip_address = args[0];
		final String port_num = args[1];

		Scanner s = new Scanner(System.in);
		BufferedInputStream in = null;
		BufferedOutputStream out = null;
		Thread player = null;

		System.out.println("Client: Connecting to host " + ip_address + " on port " + port_num);
		printPrompt();
		int max_song_index = printList(ip_address, port_num);
		
		while (true) {
			// get user command
			System.out.print(">> ");
			String command = s.nextLine().toUpperCase();

			try {
				// open a new socket for each command
				Socket socket = new Socket(ip_address, Integer.parseInt(port_num));
				if(socket.isConnected()) {
					String[] split_command = command.split(" ");
					switch(split_command[0])
					{
						case "PLAY":
							// ensure that the given song number is valid
							try {
								if(Integer.parseInt(split_command[1]) > max_song_index) {
									System.out.println("Error: Invalid song request, please try again.");
									break;
								}
							}
							catch(NumberFormatException ne) {
								System.out.println("Error: Invalid song request, please try again.");
								break;
							}
							// if we have something playing already, stop it
							if (player != null) {
								player.interrupt();
								player.join();
								player = null;
							}

							// send the command
							out = new BufferedOutputStream(socket.getOutputStream(), 1024);
							out.write(command.getBytes());
							out.flush();

							// play the song
							in = new BufferedInputStream(socket.getInputStream(), 2048);
							if(in.read() == -1) {
								System.out.println("Invalid song request, please try again");
							}
							else {
								player = new Thread(new AudioPlayerThread(in));
								player.start();
							}
							break;
						case "INFO":
							// validate input for info command
							if(split_command.length != 2) {
								System.out.println("Error: INFO for which song?");
								break;
							}
							if(Integer.parseInt(split_command[1]) > max_song_index) {
								System.out.println("Error: Invalid song request, please try again.");
								break;
							}
						case "LIST":
							// send command
							Socket infoSocket = new Socket(ip_address, Integer.parseInt(port_num));
							out = new BufferedOutputStream(infoSocket.getOutputStream(), 1024);
							out.write(command.getBytes());
							out.flush();

							// read and output response
							BufferedReader read = new BufferedReader(new InputStreamReader(infoSocket.getInputStream()));
							int offset = Integer.parseInt(read.readLine());
							//System.out.println(offset);
							char[] cbuffer = new char[offset+6];
							read.read(cbuffer, 0, offset+6);
							System.out.println(cbuffer);
							read.close();
							infoSocket.close();
							break;
						case "STOP":
							// interrupt the thread and close the socket
							if(player != null) {
								player.interrupt();
								player.join();
								player = null;
							}
							socket.close();
							break;
						case "EXIT":
							if(player != null) {
								player.interrupt();
								player.join();
								player = null;
							}
							System.out.println("Thank you for using Audio Client");
							System.exit(0);
							break;
						default:
							System.out.println("Error: Unrecognized Command.");
							printPrompt();	
					}
				}
			}
			catch (Exception e) {
				System.out.println(e);
			}
		}
	}

	/**
	 * A utility method to print out user-friendly usage information
	 */
	public static void printPrompt() {
		System.out.println("Use the following Commands to interact with Audio Client:");
		System.out.println("\tLIST");
		System.out.println("\tINFO [song number]");
		System.out.println("\tPLAY [song number]");
		System.out.println("\tSTOP");
		System.out.println("\tEXIT");
	}

	/**
	 * This method sends a LIST command to the server, then prints the response.<br>
	 * It has a secondary function of counting the number of songs available on the server.<br>
	 * This information will later be used to validate commands on the client side.
	 * 
	 * @param ip_address The ip address of the server
	 * @param port_num The port number to access the server on
	 * @return The number of songs available on the server
	 */
	public static int printList(String ip_address, String port_num) {
		try {
			int count = 0;
			String command = "LIST";

			// send command
			Socket infoSocket = new Socket(ip_address, Integer.parseInt(port_num));
			BufferedOutputStream out = new BufferedOutputStream(infoSocket.getOutputStream(), 1024);
			out.write(command.getBytes());
			out.flush();

			// read response
			BufferedReader read = new BufferedReader(new InputStreamReader(infoSocket.getInputStream()));
			int offset = Integer.parseInt(read.readLine());
			char[] cbuffer = new char[offset+6];
			read.read(cbuffer, 0, offset+6);

			// count and print lines
			String[] lines = new String(cbuffer).split("\r\n|\r|\n");
			count = lines.length - 3; // -3 because we have title line, terminating newline, and index starting at 0 not 1
			System.out.println(cbuffer);

			// close socket and stream before returning
			read.close();
			infoSocket.close();
			return count;
		}
		catch(Exception e) {
			System.out.println(e);
		}
		return -1;
	}

	/**
	 * A utility method to convert a char array (of integers) to an int
	 * 
	 * @param data the byte array
	 * @param start the index to start reading
	 * @param end the index to stop reading
	 * @return The integer represented in the char array
	 * @throws NumberFormatException
	 */
	public static int charArrayToInt(char []data,int start,int end) throws NumberFormatException
	{
		int result = 0;
		for (int i = start; i < end; i++)
		{
			int digit = (int)data[i] - (int)'0';
			if ((digit < 0) || (digit > 9)) throw new NumberFormatException();
			result *= 10;
			result += digit;
		}
		return result;
	}
}
