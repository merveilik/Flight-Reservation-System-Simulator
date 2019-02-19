#include <iostream>
#include <fstream>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <queue>
using namespace std;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

static int totalSeats = 0;
struct Seat {
    bool isChosen;
    bool isReserved;
    int id;
    int clientID;

    Seat(int _id=0) {
        isReserved = false;
        isChosen = false;
        id = _id;
        clientID = 0;
    }
};

queue <Seat> outputList;
Seat seats[101];

/*
 * arguments that will be passed to server threads from client threads
 */
struct sendToServer{
    int seatID;
    int clientID;
    sendToServer() {
        seatID = 0;
        clientID = 0;
    }
};


//server threads' execution begins
void *serverRunner (void *param){

    struct sendToServer *serverArgs = (struct sendToServer *) param;

    //critical section begins
    pthread_mutex_lock(&m);
    int seat = serverArgs->seatID;
    while (seats[seat].isReserved || seats[seat].isChosen ){  //check if seat is available if not choose new one
        seat = (rand() % (totalSeats)) + 1;
    }
    seats[seat].clientID = serverArgs->clientID; //update chosen seat's clientID in global seats array
    seats[seat].isChosen = true;
    seats[seat].isReserved = true;
    outputList.push(seats[seat]);   // record seat information to output later
    pthread_mutex_unlock(&m);
    //critical section ends

    pthread_exit(0); // terminate the thread

}

//client threads' execution begins
void *requestASeat(void *param)
{
    usleep( (rand() % (200-50+1)) + 50 );  //each thread sleeps for a random time
    int id = *(int*) param;                //thread id was passed from main

    pthread_mutex_lock(&m);
    pthread_t serverThread;
    sendToServer serverArgs;     //chosen seatID and clientID will be sent to server thread
    serverArgs.clientID = id;
    pthread_create(&serverThread,NULL,serverRunner,&serverArgs);  //server thread is created
    serverArgs.seatID = (rand() % (totalSeats)) + 1;           //a random seat is chosen and sent to server thread
    pthread_mutex_unlock(&m);

    pthread_join(serverThread,NULL);  //thread will wait for server thread to finish
    pthread_exit(0);                 //terminate the thread

}

int main(int argc, char *argv[]) {

    int numOfSeats = stoi(argv[1]);  //num of seats given as argument
    totalSeats = numOfSeats;         //global variable for total seat number
    pthread_t threads[numOfSeats];

    for(int i = 1; i<=numOfSeats; i++){
        Seat s(i);
        seats[i] = s;
    }

    int ids[numOfSeats];
    for (int a = 1; a <= numOfSeats; a++) {
        ids[a] = a;
        pthread_create(&threads[a], NULL, requestASeat, &ids[a]); //threads are created sequentially
    }

    for(int b=1;b<=numOfSeats;b++){
        pthread_join(threads[b],NULL);  //wait for threads to finish
    }


    //reservations info is written to output file
    ofstream myfile("output.txt");
    myfile << "Number of total seats: "<< numOfSeats << endl;
    while(!outputList.empty()){
        myfile<<"Client"<<outputList.front().clientID<<" reserves Seat"<<outputList.front().id<<endl;
        outputList.pop();
    }
    myfile << "All seats are reserved.";
    myfile.close();

    return 0;
}