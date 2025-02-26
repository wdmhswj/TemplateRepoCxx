#include "httplib/httplib.h"
#include <iostream>
#include "nlohmann/json.hpp" // For JSON support

using json = nlohmann::json;

int main() {
    httplib::Client cli("http://localhost:5000");

    // Create a JSON object for the POST data
    json jsonData;
    jsonData["username"] = "sadf";
    jsonData["password"] = "3234nns";

    // Send a POST request with JSON data
    auto res = cli.Post("/test", jsonData.dump(), "application/json");

    if (res && res->status == 200) {
        std::cout << res->body << std::endl;
    } else if (res) {
        std::cerr << "Response returned with status code: " << res->status << std::endl;
    } else {
        auto err = res.error();
        std::cerr << "Error: " << httplib::to_string(err) << std::endl;
    }

    return 0;
}