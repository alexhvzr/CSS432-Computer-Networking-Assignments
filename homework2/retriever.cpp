#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <fstream>

using namespace std;

string CREATED_FILE= "requestResponse.txt"; // Name of the file that will contain the information from the server

// char *serverName;
string serverName;
char *fileName;

// This function parses the header information of the response
string parseHeaderInfo(int socketInfo){
    string responseHeader = "";
    char lastChar = 0;
    while (1){
        // cout << "In this loop forever" << endl;
        char currentChar = 0;
        // cout << "socketinfo=" << socketInfo << endl;
        recv(socketInfo , &currentChar , 1 , 0);
        // cout << "In this loop forever" << endl;
        if ( currentChar == '\n' || currentChar == '\r' ){
            if ( lastChar == '\r' && currentChar == '\n' ) // For each header, it is ended with a \r\n
                break;
        }
        else responseHeader += currentChar;
        lastChar = currentChar;
    }
    // cout << "JK lol" << endl;
    return responseHeader;
}

// Mehtod to set up the socket for connection to server. 
int setUpSocket(char *const *argumentValues){
    struct addrinfo hints; //define what the getaddrinfo going to do.
    struct addrinfo *serverInfo; 
    memset(&hints , 0 , sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    serverName = argumentValues[1];
    // cout << serverName << endl;
    if(serverName.substr(0,7) == "http://"){
        cout << "\nParsing off \"http://\" from " << serverName;
        serverName = serverName.substr(7);
    } else if (serverName.substr(0,8) == "https://"){
        cout << "\nParsing off \"https://\" from " << serverName;
        serverName = serverName.substr(8);
    }

    fileName = argumentValues[2];
    // cout << fileName << endl;
    // cout << argumentValues[3] << endl;
    int addrInfoStatus = getaddrinfo(serverName.c_str() , argumentValues[3] , &hints , &serverInfo);
    
    if (addrInfoStatus != 0) {            // Couldn't connect.
        cout << gai_strerror(addrInfoStatus); 
        return -1;
    }
    
    struct addrinfo *possibleConnection;
    int socketConn;
    int socketConnectionResult;
    // Go through all connection that was found and connect to the first one
    
    for ( possibleConnection = serverInfo; possibleConnection != NULL; possibleConnection = possibleConnection->ai_next ){
        socketConn = socket(possibleConnection->ai_family , possibleConnection->ai_socktype , possibleConnection->ai_protocol);
        if (socketConn == -1){
            cout << "Invalid one socket file descriptor.";
            continue;
        }
        socketConnectionResult = connect(socketConn , possibleConnection->ai_addr , possibleConnection->ai_addrlen); // Connect to socket.
        if ( socketConnectionResult == -1 ){
            cout << "Invalid socket.";
            continue;
        }
        cout << "\n\nConnected to server------------>" << serverName << endl;
        cout <<     "Attempting to get file--------->" << fileName << endl;
        break;
    }
    // If still null, then it means that we went through all possible connections but none satisfied
    if ( possibleConnection == NULL ){
        cout << "Unable to connect or empty result was given";
        return -1;
    }
    freeaddrinfo(serverInfo);
    return socketConn;
}

// Method for processing the GET request from the Socket File
int processGetRequest(int socketInfo){
    string request = string("GET " + string(fileName) + " HTTP/1.1\r\n" +
                            "Host: " + string(serverName) + "\r\n" +
                            "\r\n"); 
    cout << "\nRequest: \n" << request << endl;
    int sendResult = send(socketInfo , request.c_str() , strlen(request.c_str()) , 0);
    // Couldn't send the request. 
    if (sendResult <= 0){
        cout << "Unable to send the request";
        return -1;
    }
    
    cout << "Response:" << endl;
    int len = 0;
    while (1){
        string responseHeader = parseHeaderInfo(socketInfo);
        // cout << "ReponseHeader=" << responseHeader << endl;
        if ( responseHeader == "" ) 
            break; //end of header
        cout << responseHeader << endl;
        if ( responseHeader.substr(0 , 15) == "Content-Length:" )
            len = atoi(responseHeader.substr(16 , responseHeader.length()).c_str()); // Bytes in message
    }
    // cout << "Broke out of while loop" << endl;
    cout << "\nHTML from file " << string(fileName) << " saved in file \"" + CREATED_FILE << "\"" <<  endl;
    ofstream outputFile;
    outputFile.open(CREATED_FILE);
    char buffer[len]; // Allocate a buffer to store the content
    recv(socketInfo , &buffer , len , 0);
    
    for ( int i = 0; i < len; i++ )
        outputFile << buffer[i];
    
    close(socketInfo);
    outputFile.close();
    return 0;

}
/*
    Arguments to be passed:
    argc[1] = serverIp
        Example: www.google.com
    argc[2] = filePath
        Example: /index.html
    argc[3] = port
        Example: 80 
        Only use 80 if you are trying to connect with an actual website.
*/
int main(int argumentsPassed , char *argc[]){
    if ( argumentsPassed != 4 ){
        cout << "Incorrect number of argument provided. Expected serverIp /filePath port#. ";
    }
    int socket = setUpSocket(argc);
    if (socket == -1){
        cout << "Unable to create a socket";
        return -1;
    }
    return processGetRequest(socket);
}