#include <sys/types.h>    // socket, bind
#include <sys/socket.h>   // socket, bind, listen, inet_ntoa
#include <netinet/in.h>   // htonl, htons, inet_ntoa
#include <arpa/inet.h>    // inet_ntoa
#include <netdb.h>        // gethostbyname
#include <fcntl.h>        // read, write, close
#include <strings.h>      // bzero
#include <netinet/tcp.h>  // SO_REUSEADDR
#include <sys/uio.h>      // writev
#include <sys/time.h>     // time_t, clock_t
#include <stdlib.h>
#include <iostream>       // cerr
#include <unistd.h>
#include <pthread.h>
using namespace std;

#define BUFSIZE     1500
#define N		10

int port, repetition, serverSd = -1;


// This method is that acts as the thread interrupt for the pthread. 
void *threadInterrupt(void *data_struct){
    // Socket from main, number of reads, databuf, time stamps
    int socket = *(int*)data_struct, count = 1;
    char* databuf[BUFSIZE];
    struct timeval startTime, stopTime;

    // Gets the start time
    gettimeofday(&startTime, NULL);

    // Gets the amount of reads
    for (int i = 0; i < repetition; i++){
        for (int nRead = 0; (nRead += read(socket, databuf, BUFSIZE - nRead) ) < BUFSIZE;++count);
        ++count;
    }

    write(socket, &count, sizeof(count));

    // Gets the stop time
    gettimeofday(&stopTime, NULL);

    // Calculates the data receiving time. 
    long dataReceivingTime = (stopTime.tv_sec - startTime.tv_sec) * 1000000 +
                             (stopTime.tv_usec - startTime.tv_usec);
    cout << "data-receiving time = " << dataReceivingTime << " usec\n";
    close(socket);
    return 0;
}


// Arguments that are passed into the command line when compiled. 
//	argv[1] = port = a server IP port
//	argv[2] = repetition = repetition of sending a set of data buffers

int main( int argc, char* argv[] ){
    if ( argc != 3 ){
        cerr << "args != 3 \n";
        return 1;
    } 
    cerr << "Starting Server... \n";


    port = atoi(argv[1]);
    repetition = atoi(argv[2]);

    // Set the socket address and zero initialize it. 
    sockaddr_in acceptSockAddr;
    bzero( (char*)&acceptSockAddr, sizeof( acceptSockAddr ) );
    acceptSockAddr.sin_family      = AF_INET;
    acceptSockAddr.sin_addr.s_addr = htonl( INADDR_ANY );
    acceptSockAddr.sin_port        = htons( port );

    // Create a socket.
    serverSd = socket( AF_INET, SOCK_STREAM, 0 );

    const int on = 1;
    setsockopt( serverSd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(int) );

    // Bind socket to its local address.
    bind ( serverSd, (sockaddr* )&acceptSockAddr, sizeof( acceptSockAddr ) );

    // Instruct the OS to listen to up to N connection requests from client, N is defined.
    listen( serverSd, N );

    // Receive a request from a client, return a new socket to specific request
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof( newSockAddr );


    // Accepts connections until the process is killed
    while(true){
        // Accepting connections
        int newSd = accept( serverSd, ( sockaddr*)&newSockAddr, &newSockAddrSize );


        
        cout << "newSd=" << newSd << endl;
        // Create the thread to run the thread_server function with the newSd
        pthread_t thread;
        if (pthread_create(&thread, NULL, threadInterrupt, (void*)&newSd) < 0 ){
            cerr << "Thread not created. \n";
            return 1;
        }
 
        // Merge threads back to avoid wasting resources
        pthread_join(thread, NULL);

    }

    return 0;
}


