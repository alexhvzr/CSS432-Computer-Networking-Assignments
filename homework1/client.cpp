#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <iostream>
#include <errno.h>

using namespace std;

/*
 *  Arguments passed to the listening server:
 *  argv[1] = port
 *      Port should equal the last four of SID = 1208. If not port += 1024
 *  argv[2] = repetition
 *  argv[3] = nbufs
 *  argv[4] = bufsize
 *  argv[5] = server (Either localhost or UW linuz machine)
 *  argv[6] = type
 */

int main(int argc, char *argv[]) {

    // Arguments passed when compiled.
    int port = 0;
    int repetition = 0;
    int nbufs = 0;
    int bufsize = 0;
    int type = 0;

    const char *serverName; // Also known as the server name

    // Time variables used.
    struct timeval startTime;
    struct timeval lapTime;
    struct timeval endTime;

    // Check if there are enough arguments passed. 
    if (argc == 7) {
        port = atoi(argv[1]);
        repetition = atoi(argv[2]);
        nbufs = atoi(argv[3]);
        bufsize = atoi(argv[4]);
        serverName = (argv[5] + '\0');
        type = atoi(argv[6]);
    } else {
        cout << "Not enough arguments passed. \n";
        return -1;
    }

    // The server will create a TCP socket that listens on a port
    // (the last 4 digits of your ID number unless it is < 1024, in which case, add 1024 to your ID number).
    if (port < 1024)
        port += 1024;

    // Must be an IP address.
    struct hostent *host = gethostbyname(serverName);
    if (host == NULL) {
        cerr << "Cannot find host. \n";
        return -1;
    }

    // Declare a sockaddr_in structure, zero-initialize it by calling bzero, and set its data members
    sockaddr_in sendSockAddr;
    bzero((char *) &sendSockAddr, sizeof(sendSockAddr));
    sendSockAddr.sin_family = AF_INET; // Address Family Internet
    sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr *) *host->h_addr_list));
    sendSockAddr.sin_port = htons(port);

    // cout << inet_ntoa(sendSockAddr.sin_addr) << "/" << port << endl;

    // Open a stream-oriented socket with the Internet address family.
    int clientSd = socket(AF_INET, SOCK_STREAM, 0);

    // Check to see if the socket is open.
    if (clientSd < 0) {
        cerr << "Client can't open socket. \n";
        return -1;
    }

    // Connect this socket to the server by calling connect as passing the following arguments:
    // the socket descriptor, the sockaddr_in structure defined above, and its data size.
    connect(clientSd, (sockaddr *) &sendSockAddr, sizeof(sendSockAddr));

    char databuf[nbufs][bufsize]; // where nbufs * bufsize = 1500

    gettimeofday(&startTime, NULL);


    // Chooses the specified write type
    // Then writes it as many times as repetition
    int itr = 0;
    while (itr < repetition) {
        // Ensure databuf size is correct
        if (nbufs * bufsize == 1500) {
            if (type == 1) { // Multiple Writes
                for (int j = 0; j < nbufs; j++) {
                    write(clientSd, databuf[j], bufsize);
                }
            } else if (type == 2) { // writev
                struct iovec vector[nbufs];
                for (int j = 0; j < nbufs; j++) {
                    vector[j].iov_base = databuf[j];
                    vector[j].iov_len = bufsize;
                }
                writev(clientSd, vector, nbufs); // allocates an array of iovec data structures
            } else { // single write, type == 3
                write(clientSd, databuf, nbufs * bufsize);
            }
        } else {
            cerr << "(nbufs*bufsize) doesn't equal 1500. \n";
        }
        itr++;
    }

    // Finds the total lap time. 
    gettimeofday(&lapTime, NULL);

    long dataSendingTime = (lapTime.tv_sec - startTime.tv_sec) * 1000000 + (lapTime.tv_usec - startTime.tv_usec);


    int numOfReads = 0;

    // Receive server acknowledgements
    while (true) {
        // Read from server
        int numBytes = read(clientSd, &numOfReads, sizeof(numOfReads));

        // End program if read not successful
        if (numBytes == -1) {
            cerr << "Read Error! \n";
            return -1;
        }
            // Break when finished reading
        else if (numBytes == 0)
            break;
    }

    gettimeofday(&endTime, NULL);

    long roundTripTime = (endTime.tv_sec - startTime.tv_sec) * 1000000 + (endTime.tv_usec - startTime.tv_usec);

    cout << "Test " << type << ": data-sending time = " << dataSendingTime
         << " usec, round-trip time = " << roundTripTime << " usec, #read = "
         << numOfReads << endl;

    close(clientSd);
    return 0;
}