#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <random>
#include <vector>
#include <chrono>
#include <algorithm>

using namespace std;

// Patient structure
struct Patient {
    int id;
    double arrival_time;
    double service_time;
};

// Shared resources
queue<Patient> waitingQueue;
mutex queue_mutex;
condition_variable cv;
bool simulation_done = false;

// Server thread function
void server(int server_id) {
    while (true) {
        unique_lock<mutex> lock(queue_mutex);
        cv.wait(lock, []{ return !waitingQueue.empty() || simulation_done; });

        if (waitingQueue.empty() && simulation_done) {
            break;  // exit when all patients served
        }

        Patient p = waitingQueue.front();
        waitingQueue.pop();
        lock.unlock();

        cout << "Server " << server_id << " starts treating patient " << p.id 
             << " (service time: " << p.service_time << ")\n";

        // Simulate service (scaled for demo; you can remove sleep for faster run)
        this_thread::sleep_for(chrono::milliseconds((int)(p.service_time * 100)));

        cout << "Server " << server_id << " finished patient " << p.id << "\n";
    }
}

int main() {
    int num_patients = 10;
    int num_servers = 2;

    // Launch server threads
    vector<thread> servers;
    for (int i = 0; i < num_servers; i++)
        servers.emplace_back(server, i+1);

    // Random generators
    default_random_engine generator(time(0));
    exponential_distribution<double> interarrival(1.0 / 5.0);  // mean inter-arrival = 5
    normal_distribution<double> service_time_dist(5.0, 1.0);   // mean=5, std=1

    double current_time = 0;

    // Generate patients
    for (int i = 1; i <= num_patients; i++) {
        double t = interarrival(generator);
        current_time += t;

        Patient p;
        p.id = i;
        p.arrival_time = current_time;
        p.service_time = max(0.1, service_time_dist(generator));  // avoid negative

        // Push to queue safely
        {
            lock_guard<mutex> lock(queue_mutex);
            waitingQueue.push(p);
        }
        cv.notify_one(); // wake up a server

        cout << "Patient " << p.id << " arrives at t=" << current_time << "\n";

        // optional sleep to simulate real-time arrivals (scaled)
        this_thread::sleep_for(chrono::milliseconds((int)(t*50)));
    }

    // All patients generated
    {
        lock_guard<mutex> lock(queue_mutex);
        simulation_done = true;
    }
    cv.notify_all();  // wake all servers to finish

    // Join threads
    for (auto &t : servers) t.join();

    cout << "\nAll patients treated. Simulation finished.\n";
    return 0;
}