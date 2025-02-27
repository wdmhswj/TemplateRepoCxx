#include "httplib/httplib.h"
#include <iostream>

int main(void) {
    // httplib::Client cli("http://example.com");
    httplib::Client cli("http://localhost:8080");

    // Send a GET request to the specified path
    if (auto res = cli.Get("/hi")) {
        if (res->status == 200) { // Check if the status code is 200 OK
            std::cout << res->body << std::endl; // Print the response body
        } else {
            std::cerr << "Response returned with status code: " << res->status << std::endl;
        }
    } else {
        auto err = res.error();
        std::cerr << "Error: " << httplib::to_string(err) << std::endl;
    }

    return 0;
}