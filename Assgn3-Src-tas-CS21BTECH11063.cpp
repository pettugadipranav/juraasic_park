/*
Code by
Operating System-2 (OS-3523)
Programming Assignment 3 :Implementing TAS, CAS and Bounded Waiting CAS
Mutual Exclusion Algorithms
*/
#include <iostream>
#include <stdlib.h>
#include <random>
#include <pthread.h>
#include <fstream>
#include <unistd.h>
#include <atomic>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
using namespace std;
int n, k, l1, l2;                    // n,k,lambda_1 ,lambda_2
string **req_t, **entry_t, **exit_t; // 2d arrays to store required timestamps
atomic_flag lock_stream = ATOMIC_FLAG_INIT;
stringstream stream;
random_device rd;
mt19937 gen(rd());
exponential_distribution<double> distribution1, distribution2;
exponential_distribution<double> var_rand(int meaner)
{
    // function to return objects which are used
    // to generate random exponentially distributed numbers with given mean
    double exp_dist_lambda = (double)1 / meaner;
    exponential_distribution<double> distribution(exp_dist_lambda);
    return distribution;
}
std::string get_time() // function to return timestamp in string format
{
    using namespace std::chrono;
    using clock = system_clock;

    const auto current_time_point{clock::now()};
    const auto current_time{clock::to_time_t(current_time_point)};
    const auto current_localtime{*std::localtime(&current_time)};
    const auto current_time_since_epoch{current_time_point.time_since_epoch()};
    const auto current_milliseconds{duration_cast<milliseconds>(current_time_since_epoch).count() % 1000};

    std::ostringstream stream;
    stream << std::put_time(&current_localtime, "%T") << "." << std::setw(3) << std::setfill('0') << current_milliseconds;
    return stream.str();
}
void file_printer() // function to print output file as specified
{
    int i, j;
    ofstream fp("output.txt");
    string temp = "th";
    fp << "TAS ME Output:" << endl;
    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; j++)
        {
            if (i >= 3)
                temp = "th";
            else if (i == 0)
                temp = "st";
            else if (i == 1)
                temp = "nd";
            else
                temp = "rd";
            fp << i + 1 << temp << " CS Requested at " << req_t[j][i] << " by thread " << j + 1 << endl;
            fp << i + 1 << temp << " CS Entered at " << entry_t[j][i] << " by thread " << j + 1 << endl;
            fp << i + 1 << temp << " CS Exited at " << exit_t[j][i] << " by thread " << j + 1 << endl;
        }
    }
    fp.close();
}
void sleeper(double req_time) // function to make sleep a thread for given time
{
    req_time = (double)req_time / 1000;
    struct timespec tim;
    tim.tv_sec = (int)req_time / 1;
    tim.tv_nsec = (req_time - (int)req_time / 1) * 1000000000;
    tim.tv_nsec = ceil(tim.tv_nsec);
    tim.tv_sec = ceil(tim.tv_sec);
    nanosleep(&tim, NULL);
}
void *each_thread_tas(void *arg) // main algorithm for exectution of TAS
{
    std::string tempo;
    int i;
    int t_no = *((int *)arg); // get thread number
    free(arg);
    for (i = 0; i < k; i++)
    {
        tempo = get_time();
        req_t[t_no][i] = tempo; // requested section
        while (lock_stream.test_and_set())
            ;
        tempo = get_time();
        entry_t[t_no][i] = tempo; // entry section
        double t1 = distribution1(gen);
        sleeper(t1); // Simulation of CS
        lock_stream.clear();
        tempo = get_time();
        exit_t[t_no][i] = tempo; // exit section
        double t2 = distribution2(gen);
        sleeper(t2); // Simulation of Reminder Section
    }
    return NULL;
}
void foo() //function to create n threads each thread calling function each_thread_tas()
{
    int i;
    pthread_t tid = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_t *pid = (pthread_t *)calloc(n, (sizeof(pthread_t)));
    for (i = 0; i < n; i++)
    {
        int *arg = (int *)malloc(sizeof(*arg));
        *arg = i;
        // create n threads
        if (pthread_create(&tid, &attr, each_thread_tas, arg) != 0)
        {
            printf("Error occured in creating the thread \n");
        }
        pid[i] = tid;
    }
    for (i = 0; i < n; i++)
        pthread_join(pid[i], NULL);
    file_printer(); // calling file printer to print output file
    free(pid);
}
void metric() // function to print statistical data for graph plotting
{
    int i, j;
    string temp = entry_t[0][0];
    unsigned long long int avg = 0, worst = 0, hour = 0, minute = 0, sec = 0, msec = 0;
    unsigned long long int differ = 0;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < k; j++)
        {
            hour = (stoi(entry_t[i][j].substr(0, 2)) - stoi(req_t[i][j].substr(0, 2))) * 60 * 60 * 1000;
            minute = (stoi(entry_t[i][j].substr(3, 2)) - stoi(req_t[i][j].substr(3, 2))) * 60 * 1000;
            sec = (stoi(entry_t[i][j].substr(6, 2)) - stoi(req_t[i][j].substr(6, 2))) * 1000;
            msec = (stoi(entry_t[i][j].substr(9, 2)) - stoi(req_t[i][j].substr(9, 2)));
            differ = hour + minute + sec + msec;
            avg = avg + differ;
            if (differ >= worst)
                worst = differ;
        }
    }
    cout << "Average case CS entry time : " << (double)avg / (n * k) << endl;
    cout << "Worst case CS entry time : " << worst << endl;
}
int main()
{
    int i;
    FILE *fp;
    fp = fopen("inp-params.txt", "r");
    if (fscanf(fp, "%d %d %d %d", &n, &k, &l1, &l2) == EOF)
    {
        cout << "Error reading file" << endl;
        fclose(fp);
        return 0;
    }
    if (n <= 0 || k <= 0 || l1 <= 0 || l2 <= 0)
    {
        cout << "Invalid inputs" << endl;
        fclose(fp);
        return 0;
    }
    fclose(fp);
    entry_t = (string **)calloc(n, sizeof(string *));
    req_t = (string **)calloc(n, sizeof(string *));
    exit_t = (string **)calloc(n, sizeof(string *));
    for (i = 0; i < n; i++)
    {
        entry_t[i] = (string *)calloc(k, sizeof(string));
        req_t[i] = (string *)calloc(k, sizeof(string));
        exit_t[i] = (string *)calloc(k, sizeof(string));
    }
    distribution1 = var_rand(l1);
    distribution2 = var_rand(l2);
    foo(); // main function to call
    cout << "**** Statistical data  *****  " << endl;
    metric();
    for (i = 0; i < n; i++)
    {
        free(entry_t[i]);
        free(req_t[i]);
        free(exit_t[i]);
    }
    free(entry_t);
    free(req_t);
    free(exit_t);
    return 0;
}
