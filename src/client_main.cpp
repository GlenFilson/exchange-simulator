#include "exchange_client.hpp"
#include "binary_serializer.hpp"
#include "order_simulator.hpp"

int main() {
    BinarySerializer serializer;
    OrderSimulator simulator(1000.0);
    //localhost
    ExchangeClient client("127.0.0.1", 8080, serializer, simulator);
    client.connect();
    client.run();
    return 0;
}