#include <iostream>
#include "data_parser.hpp" 

int main() {
    const std::string api_token = "d9b66u1r01qmk4gkdjs0d9b66u1r01qmk4gkdjsg";
    optionlab::DataGatherer feed(sdt::getenv("api_token"));
    feed.subscribe("NVDA");
    feed.on_tick([](const optionlab::Trade& t));
    feed.start();
    feed.stop();
    return 0;
}