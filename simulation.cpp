#include <iostream>
#include <cstdlib>
#include <ctime>
#include <queue>

using namespace std;

int patients = 5;
int service_time = 5;
int mean_inter_arrival_time = 10;

void simulation()
{
    queue<pair<int,int> > waitingQueue; // (patient_id, arrival_time)

    float lambda = 1.0 / mean_inter_arrival_time;
    float mu     = 1.0 / service_time;
    float rho    = lambda / mu;

    if (rho >= 1.0) {
        cout << "--- FORMULA RESULTS ---\n";
        cout << "ERROR: rho = " << rho << " (must be < 1)\n";
        cout << "System is unstable.\n";
        return;
    }

    float Lq = (rho * rho) / (1 - rho);
    float L  = rho / (1 - rho);
    float Wq = Lq / lambda;
    float W  = 1.0 / (mu - lambda);
    float P0 = (1 - rho) * 100;

    cout << "--- FORMULA RESULTS ---\n";
    cout << "Lq: " << Lq << endl;
    cout << "L : " << L  << endl;
    cout << "Wq: " << Wq << endl;
    cout << "W : " << W  << endl;
    cout << "P0: " << P0 << "%\n";

    cout << "\n--- SIMULATION ---\n";

    int arrival_time = 0;
    int doctor_free_time = 0;

    for (int i = 1; i <= patients; i++)   
    {
        int interarrival = rand() % mean_inter_arrival_time + 1;
        arrival_time += interarrival;

        cout << "\nPatient " << i << " arrives at t=" << arrival_time << endl;

        //  serve queued patients FIRST (correct timing)
        while (!waitingQueue.empty() && doctor_free_time <= arrival_time)
        {
            int pid = waitingQueue.front().first;
            int at  = waitingQueue.front().second;
            waitingQueue.pop();

            int service_start = doctor_free_time;
            int waiting_time = service_start - at;

            cout << "Serving queued patient " << pid << endl;
            cout << "Waiting time: " << waiting_time << endl;
            cout << "Doctor treating patient " << pid << endl;

            doctor_free_time = service_start + service_time;
            cout << "Doctor free at t=" << doctor_free_time << endl;
        }

        if (arrival_time < doctor_free_time)
        {
            // Doctor busy push
            waitingQueue.push({i, arrival_time});   //  store arrival time

            cout << "Doctor busy, patient " << i
                 << " joins queue (size: " << waitingQueue.size() << ")\n";
        }
        else
        {
            // Doctor free → serve current patient
            int service_start = arrival_time;
            int waiting_time = 0;

            cout << "Waiting time: " << waiting_time << endl;
            cout << "Doctor treating patient " << i << endl;

            doctor_free_time = service_start + service_time;
            cout << "Doctor free at t=" << doctor_free_time << endl;
        }
    }

    // serve remaining queue AFTER all arrivals
    while (!waitingQueue.empty())
    {
        int pid = waitingQueue.front().first;
        int at  = waitingQueue.front().second;
        waitingQueue.pop();

        int service_start = doctor_free_time;
        int waiting_time = service_start - at;

        cout << "\nServing queued patient " << pid << endl;
        cout << "Waiting time: " << waiting_time << endl;
        cout << "Doctor treating patient " << pid << endl;

        doctor_free_time = service_start + service_time;
        cout << "Doctor free at t=" << doctor_free_time << endl;
    }
}

int main()
{
    srand(time(0));
    simulation();
}