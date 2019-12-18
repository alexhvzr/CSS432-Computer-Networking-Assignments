#include <iostream>
#include "UdpSocket.h"
#include "Timer.h"

using namespace std;

#define PORT 12088       // my UDP port
#define MAX 20000        // times of message transfer
#define MAXWIN 30        // the maximum window size
#define LOOP 10          // loop in test 4 and 5
#define TIMEOUT 1500

// client packet sending functions
void clientUnreliable( UdpSocket &sock, const int max, int message[] );
// You must implement the following two functions
int clientStopWait( UdpSocket &sock, const int max, int message[] );
int clientSlidingWindow( UdpSocket &sock, const int max, int message[], 
			  int windowSize );
//int clientSlowAIMD( UdpSocket &sock, const int max, int message[],
//		     int windowSize, bool rttOn );

// server packet receiving fucntions
void serverUnreliable( UdpSocket &sock, const int max, int message[] );
// You must implement the following two functions
void serverReliable( UdpSocket &sock, const int max, int message[] );
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], 
			 int windowSize );
//void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], 
//			 int windowSize, bool congestion );

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
      cerr << "Elasped time = ";                               // lap timer
      cout << timer.lap( ) << endl;
      break;
    case 2:
      timer.start( );                                          // start timer
      retransmits = clientStopWait( sock, MAX, message );      // actual test
      cerr << "Elasped time = ";                               // lap timer
      cout << timer.lap( ) << endl;
      cerr << "retransmits = " << retransmits << endl;
      break;
    case 3:
      for ( int windowSize = 1; windowSize <= MAXWIN; windowSize++ ) {
	timer.start( );                                        // start timer
	retransmits =
	clientSlidingWindow( sock, MAX, message, windowSize ); // actual test
	cerr << "Window size = ";                              // lap timer
	cout << windowSize << " ";
	cerr << "Elasped time = "; 
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
	serverEarlyRetrans( sock, MAX, message, windowSize );
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

// Test 1: client unreliable message send -------------------------------------
void clientUnreliable( UdpSocket &sock, const int max, int message[] ) {
  cerr << "client: unreliable test:" << endl;

  // transfer message[] max times
  for ( int i = 0; i < max; i++ ) {
    message[0] = i;                            // message[0] has a sequence #
    sock.sendTo( ( char * )message, MSGSIZE ); // udp message send
    cerr << "message = " << message[0] << endl;
  }
}

// Test1: server unreliable message receive -----------------------------------
void serverUnreliable( UdpSocket &sock, const int max, int message[] ) {
  cerr << "server unreliable test:" << endl;

  // receive message[] max times
  for ( int i = 0; i < max; i++ ) {
    sock.recvFrom( ( char * ) message, MSGSIZE );   // udp message receive
    cerr << message[0] << endl;                     // print out message
  }


}

// // Test3: sliding window client
// int clientSlidingWindow( UdpSocket &sock, const int max, int message[], int windowSize ) {
//   Timer *ackTimer = new Timer();
//   int numRetransmissions = 0; 
//   int ack = -1;                          // prepare a space to receive ack
//   int acknowledgementSequence = 0;                       // the ack sequence expected to receive 

//   // Send message until max
//   for ( int sequenceID = 0; sequenceID < max || acknowledgementSequence < max; ) {  
//     // Send until full window.
//     if ( acknowledgementSequence + windowSize > sequenceID && sequenceID < max ) {
//       message[0] = sequenceID;                    
//       sock.sendTo( (char *)message, MSGSIZE );  
//       // If the acknowledgement arrived and it's the same, move the window
//       if(sock.pollRecvFrom() > 0) {
//         sock.recvFrom( (char * ) &ack, sizeof(ack));
//         if(ack == acknowledgementSequence) 
//           acknowledgementSequence++;
        
//       }
//       sequenceID++;
//     } else { // The window is full 

//       //check for ack until timeout
//       ackTimer->start();
//       while(sock.pollRecvFrom() < 1) {
//         if(ackTimer->lap() >= TIMEOUT) {
//           //Timeout (ack lost)
//           numRetransmissions +=  sequenceID - acknowledgementSequence;
//           sequenceID = acknowledgementSequence;
//         }
//       }
//       //Ack recieved late
//       sock.recvFrom( (char * ) &ack, sizeof(ack));
//       if(ack >= acknowledgementSequence) {
//         acknowledgementSequence = ack + 1;
//       }
//     }
//   }
//   delete ackTimer;
//   return numRetransmissions;
// }

// // Test3: sliding window server
// void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize ) {

//  cerr << "server: early retransmit test:" << endl;
//   bool received[MAX] = {false};
//   int lastReceived = -1, lastAck = 0;

//   do {
//       sock.recvFrom((char *)message, MSGSIZE / 4);  // udp message receive
//       lastReceived = message[0];

//       if ((lastReceived - lastAck) <= windowSize) {
//         if (!received[lastReceived]) 
//           received[lastReceived] = true;
        
//         while (received[lastAck]) 
//           lastAck++;
        
//         lastAck = (lastAck < lastReceived) ? lastAck : lastReceived;
//         sock.ackTo((char *)&lastAck, sizeof(lastAck));
//         cerr << "Ack sent:\t" << lastAck << endl;
//       }
//       cerr << "Message:\t" << message[0] << endl;  // print out message

//   } while (lastAck+1 < max);
//   } 
