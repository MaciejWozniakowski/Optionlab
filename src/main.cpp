#include <iostream>
#include "data_parser.hpp" 

int main() {
    const char* api_token = std::getenv("APIKEY");
    if (!api_token) {std::cerr << "No api token provided" << "\n"; return 1;}
    optionlab::DataGatherer feed(api_token);
    feed.subscribe("NVDA");
    feed.on_tick([](const optionlab::Trade& t) {
        std::cout << t.symbol << t.price << t.volume << '\n';
    }
);
    feed.start();
    feed.stop();
    return 0;
}