#include "inet/http/client.hpp"
#include <iostream>
#include <clocale>

using namespace inet;

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "en_US.utf8");

	http::client client("www.google.com", true);

	client.connect();
	client.send(http::method_e::GET, "/");

	http::response buffer;
	client.retrieve(buffer);

	std::cout << "status code: " << std::to_string(buffer.status) << " " << buffer.reasonphrase << std::endl;

	return 0;
}