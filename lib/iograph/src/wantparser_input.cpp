/* This file is part of MathTrader++.
 *
 * Copyright (C) 2018 George Ioannidis
 *
 * MathTrader++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MathTrader++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MathTrader++.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iograph/wantparser.hpp>

#include <fstream>
#include <sstream>

#include "PracticalSocket.hpp"

/**************************************
 * 	PUBLIC METHODS - PARSING
 **************************************/

void
WantParser::parseUrl( const std::string & url ) {

	/* Payload data. */
	std::string data;

	/* Retrieve the remote file. */
	try {
		getUrl_( url, data );

	} catch ( const socket_utils::SocketException & error ) {
		throw std::runtime_error("Socket Exception: "
				+ std::string(error.what()));

	} catch ( const std::exception & error ) {
		throw std::runtime_error("Error during retrieving data: "
				+ std::string(error.what()));
	}

	/* Append data to input buffer. */
	std::stringstream input_buffer;
	input_buffer << data;

	/* Parse the want-file buffer. */
	this->parseStream(input_buffer);
}

void
WantParser::parseFile( const std::string & fn ) {

	/* Open the file. */
	std::filebuf fb;
	auto fb_ptr = fb.open(fn, std::ios::in);

	/* Check if failed */
	if ( fb_ptr == NULL ) {
		throw std::runtime_error("Failed to open "
				+ fn);
	}

	/* Parse the want-file. */
	std::istream is(&fb);
	try {
		this->parseStream(is);
	} catch ( const std::exception & e ) {
		/* If any exception is caught, close the file first.
		 * Then re-throw. */
		fb.close();
		throw;
	}

	/* Close the file. */
	fb.close();
}

void
WantParser::parseStream( std::istream & is ) {

	/* We will read line-by-line.
	 * Allocate a buffer to read. */
	const size_t BUFSIZE = (1<<10);
	std::string buffer;
	buffer.reserve(BUFSIZE);

	/* The line number. */
	uint64_t line_n = 0;

	/* Repeat for every line
	 * until the end of the stream. */
	while (std::getline( is, buffer )) {

		/* Increase line number;
		 * useful to document the line number if it throws an error. */
		++ line_n;
		try {
			/* Parse the individual line. */
			this->parseLine_( buffer );

		} catch ( const std::runtime_error & e ) {

			/* Add the exception text to the error list.
			 * Continue with the next line. */
			this->errors_.push_back( std::to_string(line_n)
					+ ":"
					+ e.what() );
		}
	}
}

/************************************************
 * 	STATIC PRIVATE METHODS - INPUT UTILS	*
 ************************************************/

void
WantParser::getUrl_( const std::string & url,
		std::string & data ) {

	/* Clear the input data. */
	data.clear();

	/* Sanity check: url beginning with 'http://' */
	if ( url.compare(0,7,"http://") != 0 ) {
		throw std::runtime_error("Provided url "
				"is not HTTP; "
				"expected url beginning "
				"with 'http://'");
	}

	size_t prot_pos = url.find("/",7);
	if ( prot_pos == std::string::npos ) {
		throw std::logic_error("Could not find '/'"
				" in well-formed HTTP header.");
	}

	/* Craft server url. Craft request url. */
	const std::string
		server  = url.substr(7,prot_pos-7), /* strip 'http://' */
		request = "GET "
			+ url.substr(prot_pos,std::string::npos)
			+ " HTTP/1.1\r\n"
			+ "Host: " + server + "\r\n"
			+ "\r\n";

	/* Open the socket;
	 * socket destructor will close it.
	 * Throws exception on failure. */
	socket_utils::TCPSocket sock(server, 80);

	/* Send the HTTP request. */
	sock.send(request.c_str(), request.length());

	/* Receive buffer. */
	const int BUFSIZE = (10 * (1 << 20));
	auto buffer = std::make_unique<char[]>(BUFSIZE);

	/* Fetch the HTTP header. */
	int message_size = sock.recv(buffer.get(), BUFSIZE);
	if ( message_size <= 0 ) {
		throw std::runtime_error("No data received");
	}

	int payload  = 0;	/* total payload size */
	int received = 0;	/* data retrieved so far */

	/* Open scope to calculate
	 * content length and remove header. */
	{
		std::string header(buffer.get(), message_size);

		/* Get the response code. */
		size_t i = header.find("HTTP/1.1 ");
		if ( i == std::string::npos ) {
			throw std::runtime_error("Malformed HTTP "
					"header; could not find "
					" 'HTTP/1.1'");
		}
		size_t code_start = i + 9;

		i = header.find_first_of("\n\r", code_start);
		size_t code_end = i;

		const std::string & response_code =
			header.substr(code_start, code_end-code_start);

		if ( response_code.compare("200 OK") != 0 ) {
			throw std::runtime_error("Unexpected response code; "
					"received " + response_code);
		}

		/* Get the content length. */
		i = header.find("Content-Length: ");
		if ( i == std::string::npos ) {
			throw std::runtime_error("Malformed HTTP "
					"header; could not find "
					" 'Content-Length'");
		}
		size_t content_start = i + 16;

		i = header.find_first_of("\n\r ", content_start);
		if ( i == std::string::npos ) {
			throw std::logic_error("Receive buffer "
					"is too short");
		}
		size_t content_end = i;

		/* Get the payload substring.
		 * Convert to integer. */
		const std::string & content =
			header.substr(content_start, content_end-content_start);
		payload = std::stoi(content);

		/* HTTP headers end with an empty line,
		 * as the protocol specifies. */
		i = header.find("\r\n\r\n");
		if ( i == std::string::npos ) {
			throw std::runtime_error("Malformed HTTP "
					"header; could not determine "
					" header end");
		}

		size_t payload_pos = i + 4;

		const std::string & payload =
			header.substr(payload_pos, std::string::npos);

		data.append(payload);
		received += payload.length();
	}

	/* Receive response until
	 * no further bytes are received. */
	while ((message_size = sock.recv(buffer.get(), BUFSIZE)) > 0 ) {
		data.append(buffer.get(), message_size);
		received += message_size;

		if ( received >= payload ) {
			break;
		}
	}

	/* Sanity check if we have received the expected
	 * number of bytes. */
	if ( received != payload ) {
		throw std::logic_error("Expected payload of "
				+ std::to_string(payload)
				+ " bytes; received "
				+ std::to_string(received)
				+ " bytes");
	}
}
