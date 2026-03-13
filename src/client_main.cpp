#include "exchange_client.hpp"
#include "binary_serializer.hpp"
#include "order_simulator.hpp"
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>



// Create a thread-safe atomic counter so each thread gets totally unique order IDs
std::atomic<uint64_t> global_order_id{1};

void run_client_thread(int thread_id) {
    try {
        // Every thread creates its OWN completely separate simulator and serializer!
        BinarySerializer serializer;
        // Assuming OrderSimulator takes a starting ID, or just let them overlap if your exchange doesn't care
        OrderSimulator simulator(1000); 

        ExchangeClient client("127.0.0.1", 8080, serializer, simulator);
        client.connect();
        
        std::cout << "Thread " << thread_id << " connected and running." << std::endl;
        client.run(); 
        std::cout << "Thread " << thread_id << " finished sending all orders." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Client thread " << thread_id << " error: " << e.what() << "\n";
    }
}

int main() {
    int num_threads = 4;
    std::vector<std::thread> threads;

    std::cout << "Starting " << num_threads << " client threads...\n";

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(run_client_thread, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "All clients finished." << std::endl;
    return 0;
}