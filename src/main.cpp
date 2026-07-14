#include <iostream>
#include "data_parser.hpp" 

int main() {
    optionlab::DataGatherer feed(sdt::getenv("api_token"));
    feed.subscribe("NVDA");
    feed.on_tick([](const optionlab::Trade& t));
    feed.start();
    feed.stop();
    return 0;
}