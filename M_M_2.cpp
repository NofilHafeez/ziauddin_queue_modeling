#include <iostream>
#include <cstdlib>
#include <ctime>
#include <queue>
#include <cmath>

using namespace std;

int patients = 5;
int service_time = 5;
int mean_inter_arrival_time = 10;

int simulation()
{
    queue<pair<int,int> > waitingQueue1;
    queue<pair<int,int> > waitingQueue2;

    double lambda = 1.0 / 10;
    double mu     = 1.0 / 5;
    int c = 2;

    double rho = lambda / (c * mu);

    if (rho >= 1.0) {
        cout << "System unstable\n";
        return 0;
    }

    double a = lambda / mu;

    double sum = 0;
    for (int n = 0; n < c; n++) {
        sum += pow(a, n) / tgamma(n + 1);
    }

    double last_term = pow(a, c) / (tgamma(c + 1) * (1 - rho));
    double P0 = 1.0 / (sum + last_term);

    double Lq = (pow(a, c) * rho / (tgamma(c + 1) * pow(1 - rho, 2))) * P0;
    double L = Lq + a;
    double Wq = Lq / lambda;
    double W = Wq + (1.0 / mu);

    cout << "P0: " << P0 << endl;
    cout << "Lq: " << Lq << endl;
    cout << "L : " << L << endl;
    cout << "Wq: " << Wq << endl;
    cout << "W : " << W << endl;

    cout << "\n--- SIMULATION ---\n";

    int arrival_time = 0;
    int doctor_free_time1 = 0;
    int doctor_free_time2 = 0;

    for (int i = 1; i <= patients; i++)
    {
        int interarrival = rand() % mean_inter_arrival_time + 1;
        arrival_time += interarrival;

        cout << "\nPatient " << i << " arrives at t=" << arrival_time << endl;

        // correct condition grouping
        while ( (!waitingQueue1.empty() && doctor_free_time1 <= arrival_time) ||
                (!waitingQueue2.empty() && doctor_free_time2 <= arrival_time) )
        {
            // choose earlier available doctor
            if (doctor_free_time1 <= doctor_free_time2 && !waitingQueue1.empty())
            {
                int pid = waitingQueue1.front().first;
                int at  = waitingQueue1.front().second;
                waitingQueue1.pop();

                int service_start = doctor_free_time1;
                int waiting_time = service_start - at;

                cout << "Serving queued patient " << pid << endl;
                cout << "Waiting time: " << waiting_time << endl;
                cout << "Doctor 1 treating patient " << pid << endl;

                doctor_free_time1 = service_start + service_time;
                cout << "Doctor 1 free at t=" << doctor_free_time1 << endl;
            }
            else if (!waitingQueue2.empty())
            {
                int pid = waitingQueue2.front().first;
                int at  = waitingQueue2.front().second;
                waitingQueue2.pop();

                int service_start = doctor_free_time2;
                int waiting_time = service_start - at;

                cout << "Serving queued patient " << pid << endl;
                cout << "Waiting time: " << waiting_time << endl;
                cout << "Doctor 2 treating patient " << pid << endl;

                doctor_free_time2 = service_start + service_time;
                cout << "Doctor 2 free at t=" << doctor_free_time2 << endl;
            }
        }

        // correct queue assignment logic
        if (arrival_time < doctor_free_time1 && arrival_time < doctor_free_time2)
        {
            if (waitingQueue1.size() <= waitingQueue2.size())
            {
                waitingQueue1.push({i, arrival_time});
                cout << "Both busy, patient " << i << " joins queue 1\n";
            }
            else
            {
                waitingQueue2.push({i, arrival_time});
                cout << "Both busy, patient " << i << " joins queue 2\n";
            }
        }
        else if (arrival_time < doctor_free_time1)
        {
            waitingQueue1.push({i, arrival_time});
            cout << "Doctor 1 busy, patient " << i << " joins queue 1\n";
        }
        else if (arrival_time < doctor_free_time2)
        {
            waitingQueue2.push({i, arrival_time});
            cout << "Doctor 2 busy, patient " << i << " joins queue 2\n";
        }
        else
        {
            //  assign to correct doctor
            int service_start = arrival_time;
            int waiting_time = 0;

            cout << "Waiting time: " << waiting_time << endl;
            cout << "Doctor treating patient " << i << endl;

            if (doctor_free_time1 <= doctor_free_time2)
            {
                doctor_free_time1 = service_start + service_time;
                cout << "Doctor 1 free at t=" << doctor_free_time1 << endl;
            }
            else
            {
                doctor_free_time2 = service_start + service_time;
                cout << "Doctor 2 free at t=" << doctor_free_time2 << endl;
            }
        }
    }

    // process remaining queue correctly
    while (!waitingQueue1.empty() || !waitingQueue2.empty())
    {
        if (doctor_free_time1 <= doctor_free_time2 && !waitingQueue1.empty())
        {
            int pid = waitingQueue1.front().first;
            int at  = waitingQueue1.front().second;
            waitingQueue1.pop();

            int service_start = doctor_free_time1;
            int waiting_time = service_start - at;

            cout << "\nServing queued patient " << pid << endl;
            cout << "Waiting time: " << waiting_time << endl;
            cout << "Doctor 1 treating patient " << pid << endl;

            doctor_free_time1 = service_start + service_time;
            cout << "Doctor 1 free at t=" << doctor_free_time1 << endl;
        }
        else if (!waitingQueue2.empty())
        {
            int pid = waitingQueue2.front().first;
            int at  = waitingQueue2.front().second;
            waitingQueue2.pop();

            int service_start = doctor_free_time2;
            int waiting_time = service_start - at;

            cout << "\nServing queued patient " << pid << endl;
            cout << "Waiting time: " << waiting_time << endl;
            cout << "Doctor 2 treating patient " << pid << endl;

            doctor_free_time2 = service_start + service_time;
            cout << "Doctor 2 free at t=" << doctor_free_time2 << endl;
        }
    }
}

int main()
{
    srand(time(0));
    simulation();
}