#include <iostream>
#include <cstdlib>
#include <ctime>
#include <windows.h>

using namespace std;

int patients = 5;
int service_time = 5;
float mean_inter_arrival_time = 10;

void simulation()
{
    float lambda = 1.0 / mean_inter_arrival_time;
    float mu = 1.0 / service_time;

    float rho = lambda / mu;

    float Lq = (rho*rho)/(1-rho);
    float L  = rho/(1-rho);
    float Wq = Lq/lambda;
    float W  = 1/(mu-lambda);
    float P0 = (1 - rho) * 100;

    cout << "--- FORMULA RESULTS ---\n";
    cout << "Lq: Average number of patients in queue: " << Lq << endl;
    cout << "L : Average number of patients in system: " << L << endl;
    cout << "Wq: Average waiting time in queue: " << Wq << endl;
    cout << "W : Average time spent in system: " << W << endl;
    cout << "P0: Probability of zero patients in system: " << P0 << "%\n";

    cout << "\n--- SIMULATION ---\n";

    int arrival_time = 0;
    int doctor_free_time = 0;

    for(int i=1;i<=patients;i++)
    {
        int interarrival = rand()%10 + 1;
        arrival_time += interarrival;

        cout << "\nPatient " << i << " arrives at " << arrival_time << endl;

        int service_start;

        if (arrival_time < doctor_free_time) {
            cout << "Doctor busy - patient waits in queue\n";
            service_start = doctor_free_time;
        }
        else {
            service_start = arrival_time;
        }

        int waiting_time = service_start - arrival_time;
        cout << "Waiting time: " << waiting_time << endl;
        doctor_free_time = service_start + service_time;
        cout << "Doctor treating patient " << i << endl;
        Sleep(service_time*1000);
        cout << "will finish at " << doctor_free_time << endl;
    }
}

int main()
{
    srand(time(0));
    simulation();
}