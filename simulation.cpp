#include <iostream>
using namespace std;
#include <cstdlib>
#include <windows.h>
#include <cmath>
#include <ctime>


int patients = 5;
int docters = 1;    

float lq = 0;                                    // average number of patients in the queue
float l = 0;
float wq = 0;
float w = 0;
float mean_inter_arrival_time = 10;            // average time between patient arrivals
float lambda = 1.0 / mean_inter_arrival_time;    // arrival rate of patients
int service_time = 5;                        // average service time for each patient




void doctor(int patient, int service_time) {
    cout << "Doctor is treating patient " << patient << "." << endl;

    Sleep(service_time * 1000); // Simulate treatment time in milliseconds

    cout << "Doctor finished patient " << patient << "." << endl;
}



void simulation() {
    cout << lambda << endl;
    float u = 1.0 / service_time;       // utilization of the system
    float p = lambda / u;  // traffic intensity of system
    lq = p*p / (1-p);                 // average number of patients in the queue
    wq = lq / lambda;                 // average waiting time in the queue
    w = wq + 1 / u;                   // average time a patient spends in the system
    l = lambda * w;                   // average number of patients in the system
    float server_idle_time = (1 - p) * 100;   // percentage of time the doctor is idle


    cout << "Traffic intensity (p): " << p << endl;
    cout << "Utilization (u): " << u << endl;
    cout << "Average waiting time in queue (wq): " << wq << " seconds" << endl;
    cout << "Average number of patients in queue (lq): " << lq << endl;
    cout << "Average time a patient spends in system (w): " << w << " seconds" << endl;
    cout << "Average number of patients in system (l): " << l << endl;
    cout << "Server idle time: " << server_idle_time << "%" << endl;



    int patient = 0;
    int time = 0;
    int i = 1;

    while(patient < 5){
        patient++;
        i = rand() % 10 + 1;
        time += i;

        cout << "Patient " << patient << " arrived at time " << time << " seconds." << endl;
        doctor(patient, service_time);
        // i = -log(u) / lambda; // Generate inter-arrival time using exponential distribution

    }

}

int main() {
    srand(time(0));
    simulation();
    return 0;
}