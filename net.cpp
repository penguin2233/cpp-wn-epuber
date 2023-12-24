// net.cpp: contains http client, adapted from other projects
// should switch to libressl instead of boost

#include <string>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/exception/all.hpp>

#include "net.h"

int http_get(std::string url, std::vector<unsigned char>* out) { // bin mode
	std::vector<unsigned char> ret;
	std::string host = url;
	if (host.find("https://") != std::string::npos) {
		host = host.substr(8, host.find('/', 8) - 8);
	}
	else host = host.substr(0, host.find('/'));
	std::string endpoint = url.substr(url.find('/', host.size()));
	// connect
	boost::asio::ssl::context sslcontext(boost::asio::ssl::context::sslv23);
	boost::asio::io_context io_context;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> httpsocket(io_context, sslcontext);
	boost::asio::ip::tcp::resolver resolver(io_context);
	boost::asio::ip::tcp::resolver::query query(host, "443");
	try {
		boost::asio::connect(httpsocket.lowest_layer(), resolver.resolve(query));
		httpsocket.lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
		httpsocket.handshake(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::client); // no cert checking yet !
	}
	catch (boost::exception& e) {
		std::cout << "Exception thrown trying to connect with SSL. " << boost::diagnostic_information(e) << '\n';
		return -1;
	}
	// write
	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "GET " << endpoint << " HTTP/1.1\r\n";
	request_stream << "User-Agent: Mozilla/5.0 (Windows NT 10.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/108.0.0.0 Safari/537.36 \r\n";
	request_stream << "Host: " << host << " \r\n";
	request_stream << "\r\n";
	try {
		boost::asio::write(httpsocket, request);
	}
	catch (boost::system::system_error& e) {
		std::cout << "Exception thrown trying to write to socket (http). " << boost::diagnostic_information(e) << '\n';
		return -1;
	}
	// read
	bool chunked = false;
	size_t contentLength = 0;
	std::vector<unsigned char> buf;
	std::string buf_s = "";
	Sleep(100);
	try { boost::asio::read_until(httpsocket, boost::asio::dynamic_buffer(buf), "\r\n\r\n"); }
	catch (boost::exception& e) {
		std::cout << "Exception thrown trying to read from socket. " << boost::diagnostic_information(e) << '\n';
		return -1;
	}
	for (size_t i = 0; i < buf.size(); i++) { buf_s.push_back(buf[i]); }
	size_t loc = buf_s.find("\r\n\r\n");
	std::string header = buf_s.substr(0, loc + 4);
	if (header.find("HTTP/1.1 200 OK\r\n") == std::string::npos) {
		if (header.find("HTTP/1.1 301 Moved Permanently\r\n") != std::string::npos) {
			// handle redirect
			size_t lo = buf_s.find("Location: ");
			if (lo == std::string::npos) lo = buf_s.find("location: ");
			if (lo != std::string::npos) {
				lo = lo + 10;
				size_t loEnd = buf_s.find("\r\n", lo);
				endpoint = buf_s.substr(lo, loEnd - lo);
				http_get(host + endpoint, out);
				return 0;
			}
			else { std::cout << "HTTP Error: 301 redirected but don't know where to \n"; return -1; }
		}
		else {
			// discard non OK responses but print to console
			std::cout << "HTTP Error: " << buf_s << '\n';
			return -1;
		}
	}
	if (buf_s.find("\r\n\r\n", loc + 4) == std::string::npos) {
		size_t cl = buf_s.find("Content-Length: ");
		if (cl == std::string::npos) cl = buf_s.find("content-length: ");
		if (cl != std::string::npos) {
			cl = cl + 16;
			size_t clEnd = buf_s.find("\r\n", cl);
			std::stringstream h(buf_s.substr(cl, clEnd - cl));
			h >> contentLength;
		}
	}
	if (header.find("Transfer-Encoding: chunked\r\n") == std::string::npos) {
		if (header.find("transfer-encoding: chunked\r\n") != std::string::npos) chunked = true;
	} else chunked = true;
	buf_s = buf_s.substr(loc + 4);
	buf.erase(buf.begin(), buf.begin() + loc + 4);
	// body
	if (chunked) { // handle chunked encoding, may inf loop
		try { if (buf_s.find("\r\n0\r\n\r\n") == std::string::npos) boost::asio::read_until(httpsocket, boost::asio::dynamic_buffer(buf), "\r\n0\r\n\r\n"); }
		catch (boost::exception& e) {
			std::cout << "Exception thrown trying to read from socket. " << boost::diagnostic_information(e) << '\n';
			return -1;
		}
		while (chunked) {
			if (buf_s == "\r\n0\r\n\r\n") { break; } // finished chunked transfer
			size_t loc = buf_s.find("\r\n");
			size_t chunk;
			std::stringstream h;
			h << std::hex << buf_s.substr(0, loc); h >> chunk; // hexdecimal
			for (size_t i = 0; i < chunk; i++) { ret.push_back(buf[i]); }
			buf.erase(buf.begin(), buf.begin() + loc + 2 + chunk );
			buf_s.clear();
			for (size_t i = 0; i < buf.size(); i++) { buf_s.push_back(buf[i]); }
		}
	} 
	else if (contentLength != 0) { 
		size_t leftToRead = contentLength - buf.size();
		try { boost::asio::read(httpsocket, boost::asio::dynamic_buffer(buf), boost::asio::transfer_exactly(leftToRead)); }
		catch (boost::exception& e) {
			std::cout << "Exception thrown trying to read from socket. " << boost::diagnostic_information(e) << '\n';
			return -1;
		}
		for (size_t i = 0; i < buf.size(); i++) { ret.push_back(buf[i]); }
	}
	else { // assume all data has been read to buffer
		for (size_t i = 0; i < buf.size(); i++) { ret.push_back(buf[i]); }
	}
	*out = ret;
	return 0;
}

int http_get(std::string url, std::string* out) { // str mode
	std::string o = "";
	std::vector<unsigned char> h;
	int ret = http_get(url, &h);
	if (ret != 0) return ret;
	for (size_t i = 0; i < h.size(); i++) {
		o.push_back(h[i]);
	}
	*out = o;
	return ret;
}
