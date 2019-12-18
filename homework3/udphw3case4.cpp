#include <iostream>
#include "UdpSocket.h"
#include "Timer.h"
#include <stdio.h> 
#include <stdlib.h> 
#include<time.h>
#include <vector>

using namespace std;

#define PORT 12088       // my UDP port
#define MAX 20000        // times of message transfer
#define MAXWIN 30        // the maximum window size
#define LOOP 10          // loop in test 4 and 5
#define TIMEOUT 1500
#define PERCENT 10

// client packet sending functions
void clientUnreliable( UdpSocket &sock, const int max, int message[] );
// You must implement the following two functions
int clientStopWait( UdpSocket &sock, const int max, int message[] );
int clientSlidingWindow( UdpSocket &sock, const int max, int message[], 
			  int windowSize );
//int clientSlowAIMD( UdpSocket &sock, const int max, int message[],
//		     int windowSize, bool rttOn );

// server packet receiving functions
void serverUnreliable( UdpSocket &sock, const int max, int message[] );
// You must implement the following two functions
void serverReliable( UdpSocket &sock, const int max, int message[] );
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize, int percent );

enum myPartType { CLIENT, SERVER, ERROR } myPart;

int main( int argc, char *argv[] ) {

  int message[MSGSIZE/4]; // prepare a 1460-byte message: 1460/4 = 365 ints;
  UdpSocket sock( PORT );  // define a UDP socket

  myPart = ( argc == 1 ) ? SERVER : CLIENT;

  if ( argc != 1 && argc != 2 ) {
    cerr << "usage: " << argv[0] << " [serverIpName]" << endl;
    return -1;
  }

  if ( myPart == CLIENT ) // I am a client and thus set my server address
    if ( sock.setDestAddress( argv[1] ) == false ) {
      cerr << "cannot find the destination IP name: " << argv[1] << endl;
      return -1;
    }

  int testNumber;
  cerr << "Choose a testcase" << endl;
  cerr << "   1: unreliable test" << endl;
  cerr << "   2: stop-and-wait test" << endl;
  cerr << "   3: sliding windows" << endl;
  cerr << "--> ";
  cin >> testNumber;

  if ( myPart == CLIENT ) {

    Timer timer;           // define a timer
    int retransmits = 0;   // # retransmissions

    switch( testNumber ) {
    case 1:
      timer.start( );                                          // start timer
      clientUnreliable( sock, MAX, message );                  // actual test
      cerr << "Elapsed time = ";                               // lap timer
      cout << timer.lap( ) << endl;
      break;
    case 2:
      timer.start( );                                          // start timer
      retransmits = clientStopWait( sock, MAX, message );      // actual test
      cerr << "Elapsed time = ";                               // lap timer
      cout << timer.lap( ) << endl;
      cerr << "retransmits = " << retransmits << endl;
      break;
    case 3:
      for ( int windowSize = 1; windowSize <= MAXWIN; windowSize++ ) {
	timer.start( );                                        // start timer
	retransmits =
	clientSlidingWindow( sock, MAX, message, windowSize ); // actual test
	cerr << "Drop Percentage = ";                              // lap timer
	cout << PERCENT << " " << endl;
	cerr << "\nElapsed time = "; 
	cout << timer.lap( ) << endl;
	cerr << "retransmits = " << retransmits << endl;

      }
      break;
    default:
      cerr << "no such test case" << endl;
      break;
    }
  }
  if ( myPart == SERVER ) {
    switch( testNumber ) {
    case 1:
      serverUnreliable( sock, MAX, message );
      break;
    case 2:
      serverReliable( sock, MAX, message );
      break;
    case 3:
      for ( int windowSize = 1; windowSize <= MAXWIN; windowSize++ )
	serverEarlyRetrans( sock, MAX, message, windowSize , PERCENT);
      break;
    default:
      cerr << "no such test case" << endl;
      break;
    }

    // The server should make sure that the last ack has been delivered to
    // the client. Send it three time in three seconds
    cerr << "server ending..." << endl;
    for ( int i = 0; i < 10; i++ ) {
      sleep( 1 );
      int ack = MAX - 1;
      sock.ackTo( (char *)&ack, sizeof( ack ) );
    }
  }

  cerr << "finished" << endl;

  return 0;
}

// Test 1: client unreliable -- SEND
void clientUnreliable( UdpSocket &sock, const int max, int message[] ) {
  cerr << "client: unreliable test:" << endl;

  // Transfer message until max hit
  for ( int i = 0; i < max; i++ ) {
    message[0] = i;                            // message[0] has a sequence #
    sock.sendTo( ( char * )message, MSGSIZE ); // udp message send
    cerr << "message = " << message[0] << endl;
  }
}

// Test1: server unreliable -- RECEIVE 
void serverUnreliable( UdpSocket &sock, const int max, int message[] ) {
  cerr << "server unreliable test:" << endl;

  // Receive message until max hit
  for ( int i = 0; i < max; i++ ) {
    sock.recvFrom( ( char * ) message, MSGSIZE );   // udp message receive
    cerr << message[0] << endl;                     // print out message
  }


}

// Test2: stop and wait client send
int clientStopWait( UdpSocket &sock, const int max, int message[] ) {
  cerr << "client stop-and-wait test" << endl;

  Timer *ackTimer = new Timer();
  int numRetransmissions = 0;
  
  for(int i = 0; i < max; i++) {
    message[0] = i;                             //message[0] has a sequence #
    sock.sendTo( ( char * )message, MSGSIZE ); // udp message send
    ackTimer->start();
    //Check for ack from server
    while(sock.pollRecvFrom() < 1) {
        // Check for a time out 
        if(ackTimer->lap() >= TIMEOUT) {
          numRetransmissions++;
          // Resend the message if there is a timeout
          sock.sendTo( ( char * )message, MSGSIZE ); 
          //Restart timer
          ackTimer->start();
        }
    }

    //Read ack
    sock.recvFrom( ( char * ) message, MSGSIZE );   // udp message receive
  }
  //Delete pointer
  delete ackTimer;
  return numRetransmissions;
}
// Test2: Server stop and wait recieve
void serverReliable( UdpSocket &sock, const int max, int message[] ) {
  cerr << "server reliable test" << endl;
    
  for(int sequenceNumber = 0; sequenceNumber < max; sequenceNumber++) {
    //Keep receiving messages and sending acks until same sequence number 
    do {
      sock.recvFrom( ( char * ) message, MSGSIZE );   // udp message receive
      if(message[0] == sequenceNumber) {
        //send ack
        sock.ackTo( (char *) &sequenceNumber, sizeof(sequenceNumber));   
      }
    // Check for wrong sequence number.
    }while(message[0] != sequenceNumber);
  }
}

// Test 4
int clientSlidingWindow(UdpSocket &sock, const int max, int message[], int windowSize) {
    cerr << "client: sliding window test:" << endl;
    Timer timer;
    vector<bool> sent(max, false);
    int retransmitted = 0;
    int nextInSequence = 0;
    int windowBase = 0;

    // Loop till full transmission of each packet, ACK for all. 
    while (nextInSequence < max || windowBase < max) {
        if ((nextInSequence < (windowBase + windowSize)) &&
            (nextInSequence < max)) {
            message[0] = nextInSequence;
            sock.sendTo((char *)message, MSGSIZE);  // udp message send
            // Check if packet was already sent
            if (sent[nextInSequence]) {
                retransmitted++;
            }
            // Sent 
            sent[nextInSequence] = true;
            nextInSequence++;
        } else {
            timer.start();
            // Wait for an ACK or Timeout 
            while ((timer.lap() < TIMEOUT) && (sock.pollRecvFrom() <= 0));
            // Check if an ack was received. 
            if (sock.pollRecvFrom() > 0) {
                int ack;
                sock.recvFrom((char *)&ack, sizeof(int));
                // Check if you received the ACK
                if (ack > windowBase) 
                    windowBase = ack;
            }
            else { // Expects the base to be retransmitted. 
                nextInSequence = windowBase;
            }
        }
    }
    return retransmitted;
}


void serverEarlyRetrans(UdpSocket &sock, const int max, int message[], int windowSize, int lossInterval) {
    srand(time(0)); // Seed the random number so it changes each time ran
    cerr << "server early retransmit test:" << endl;
    vector<bool> received(max, false);
    int acksSent = 0;
    int nextExpectedSequenceNum = 0;
    while (nextExpectedSequenceNum < max) {
        int sequenceNumber;
        sock.recvFrom((char *)message, MSGSIZE);  // udp message receive
        sequenceNumber = message[0];
        // Check if the percentage matches 
        if ((rand() % 100) > lossInterval) 
            received[sequenceNumber] = true;  
        // Increase count for expected packet
        while (nextExpectedSequenceNum < max && received[nextExpectedSequenceNum]) 
            nextExpectedSequenceNum++;
        // ACK
        if (nextExpectedSequenceNum <= max) {  //&& startedReceiving) {
            sock.ackTo((char *)&nextExpectedSequenceNum, sizeof(int));
            acksSent++;
        }
    }
    cout << "Percent Dropped: " << lossInterval << endl;
    cout << "Total ACKS: " << acksSent << " Acks" << endl;
    cout << "Window size: " << windowSize << endl;
}
