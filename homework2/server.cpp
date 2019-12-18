#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <functional>
#include <errno.h>
#include <string.h>
#include <stdio.h>

using namespace std;

extern int errno;

const int CONN = 10;


struct thread_data {
    int thread_id;
    int clientFileDescriptor;
};


//This method is used to parse the header info received from the request
string parseHeaderInfo(int socketFileDescriptor){
    string responseHeader = "";
    char lastChar = 0;
    while (1){
        char currentChar = 0;
        recv(socketFileDescriptor , &currentChar , 1 , 0);
        if ( currentChar == '\n' || currentChar == '\r' ){
            if ( lastChar == '\r' && currentChar == '\n' ) // For each header, it is ended with a \r\n
                break;
        }
        else responseHeader += currentChar;
        lastChar = currentChar;
    }
    return responseHeader;
}


// Method to prepare what the response should be for the GET response thrown from the client / site.
void prepareResponseData(string &filePath , bool getPassed , string &statusCode , string &fileContent){
    fileContent = "";
    if (getPassed){
        // Trying to access a file that is above the directory the server is running
        if ( filePath.substr(0 , 2) == ".." )
            statusCode = fileContent = "HTTP/1.1 403 Forbidden\r\n";
            /// Check the last 15 characters to see if trying to access SecretFile... Fix this method. 
        else if (filePath.length() >= 15){
            if (filePath.substr(filePath.length() - 15 , filePath.length()) == "SecretFile.html" )
                statusCode = fileContent = "HTTP/1.1 401 Unauthorized\r\n";
            
        }
        else{   
            filePath = "." + filePath;
            cout << "Looking for this file " + filePath << endl;
            FILE *file = fopen(filePath.c_str() , "r");
            // Could not open the file: it doesn't exist or no permission. 
            if ( file == nullptr ){
                cout << "Unable to open the file for reading";
                if ( errno == EACCES ){
                    statusCode = fileContent = "HTTP/1.1 401 Unauthorized\r\n";
                } else {
                    statusCode = fileContent = "HTTP/1.1 404 Not Found\r\n";
                }
            } else{   // Found the file
                while ( !feof(file)){
                    string line;
                    char c = fgetc(file);
                    if ( c < 0 )
                        continue;   
                    if ( c == '\n' ){ // Append string
                        fileContent += '\n';
                        continue;
                    } else if ( c == '\r' ) {
                        fileContent += "\r";
                        continue;
                    }
                    fileContent += c;
                }
                fclose(file);
                statusCode = "HTTP/1.1 200 OK\r\n";
            }
        }
    }
    else {
        // Could not recognize the get request
        statusCode = fileContent = "HTTP/1.1 400 Bad Request\r\n";
    }
}
 // This method is used to process the GET Request from client, threadData that data that the thread is gonna use (client file descriptor and id)
void *processGETRequest(void *threadData){
    struct thread_data *data;
    data = (struct thread_data *) threadData;
    string filePath = "";
    bool isGET = false;
    while (1){
        string header = parseHeaderInfo(data->clientFileDescriptor);
        if (header == "") break;
        std::cout << "	Header: " << header << "\n";
        if ( header.substr(0 , 3) == "GET" ){
            filePath = header.substr(4 , header.length() - 13);
            cout << "Found the file.  " << filePath << "\n";
            isGET = true;
            break;
        }
    }
    string statusCode;
    string fileContent;
    prepareResponseData(filePath , isGET , statusCode , fileContent);
    string length = to_string(fileContent.size());
    string response = statusCode +"Content-Length: " + length + "\r\nContent-Type: text/plain\r\n\r\n" + fileContent;
    cout << "Printing out the response " + response << endl;
    send(data->clientFileDescriptor , &response[ 0 ] , response.size() , 0);
    close(data->clientFileDescriptor);
}


// Pass in the port number as the argument[1] for the port you want to listen to. 
int main(int argumentNumber , char *argc[]){
    int serverSd;
    if ( argumentNumber != 2 ){
        cout << "Invalid number of argument. The program does not accept any argument at all";
        return -1;
    }
    cout << "Starting Server... " << endl;
	
   struct addrinfo hints; // define how the server will be configure
    struct addrinfo *serverInfo; // used to store all the connections that the server can use
    
    std::memset(&hints , 0 , sizeof(hints));
    hints.ai_family = AF_INET; // IPv4 or v6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int addressInfoStatus = getaddrinfo(nullptr , argc[1] , &hints , &serverInfo);
    if ( addressInfoStatus != 0 ){
        cout << "Unable to connect";
        cout << gai_strerror(addressInfoStatus); // print out error message
        return -1;
    }
    int socketFileDescriptor;
    
    
    struct addrinfo *connectionSocket;
    for (connectionSocket = serverInfo; connectionSocket != nullptr; connectionSocket = connectionSocket->ai_next){
        socketFileDescriptor = socket(connectionSocket->ai_family , connectionSocket->ai_socktype , connectionSocket->ai_protocol);
        if (socketFileDescriptor == -1 ){
            cout << "Invalid one socket file descriptor detected.";
            continue;
        }
        int optionValue = 1;
        setsockopt(socketFileDescriptor , SOL_SOCKET , SO_REUSEADDR , &optionValue , sizeof(optionValue));
        auto serverBindResult = bind(socketFileDescriptor , connectionSocket->ai_addr , connectionSocket->ai_addrlen);
        break; 
    }
    if (connectionSocket == NULL){
        cout << "Unable to connect.";
        return -1;
    }
    freeaddrinfo(serverInfo);


    int socketResult = listen(socketFileDescriptor , CONN);
    if ( socketResult != 0 ){
        cout << "Unable to listen using the socket given.";
        return -1;
    }
    int count = 1;
    // Server listens continuously. 
    while (1){
        struct sockaddr_storage clientSocket;
        socklen_t clientSocketSize = sizeof(clientSocket);
        cout << "Listening on port " << argc[1] << endl;
        int clientConnection= accept(socketFileDescriptor , (struct sockaddr *) &clientSocket , &clientSocketSize);
       
        if ( clientConnection == -1 ){
            cout << "Unable to connect to client." << endl;
            continue;
        }
        
        pthread_t new_thread;
        struct thread_data data;
        data.thread_id = count;
        data.clientFileDescriptor = clientConnection;
        cout << "Creating new thread with count: " + to_string(count) << endl;
        // Spawn a thread to do the work
        int threadResult = pthread_create(&new_thread , nullptr , processGETRequest , (void *) &data);
        if ( threadResult != 0 ){
            cout << "Unable to create thread. Trying again" << endl;
            continue;
        }
        count++;
    }
}