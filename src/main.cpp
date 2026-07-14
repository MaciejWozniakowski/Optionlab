#include <iostream>
#include "data_parser.hpp" 

int main() {
    const char* api_token = std::getenv("APIKEY");
    if (!api_token) {std::cerr << "No api token provided" << "\n"; return 1;}
    optionlab::DataGatherer feed(api_token);
    feed.subscribe("NVDA");
    feed.on_tick([](const optionlab::Trade& t) {
        std::cout << t.symbol <<'\n' << t.price <<'\n' << t.volume << '\n' << t.timestamp << '\n';
    });
    //TODO make this run on a boost thread 
    //TODO store the data in a hash map?
    //TODO implement pricing logic

    feed.start();
    feed.stop();
    return 0;
}