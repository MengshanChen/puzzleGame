#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<limits>

using namespace std;

const int NAMELENGTH = 100;
const int MAX = 9999;
const int MIN = 0;
const int NUMLENGTH = 4;
const int TEMP = 256;

/*
 * the function is called when a system call fails
 * it displays a message about the error on stderr
 * and then aborts the program
 */
void error(const char *msg){
    perror(msg);
    exit(0);
}

void sendName(int);
void sendGuess(int, int);
int recvDiff(int);
void recvMsg(int);
void recvBoard(int);
void recvString(int);
void sendTwo(int);




//main function, receive host and port
int main(int argc, char *argv[]) {

    int sock;
    int portno;        //port number
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if(argc < 3)
    {
        cerr<<"Syntax : ./client <host name> <port>"<<endl;
        exit(-1);
    }

    portno = atoi(argv[2]);

    sock = socket(AF_INET, SOCK_STREAM, 0);        //file descriptors
    if (sock < 0) {
        cout<< "ERROR opening socket"<<endl;
        exit(-1);
    }

    server = gethostbyname(argv[1]);
    if (server == nullptr) {
        cerr << "ERROR, no such host" <<endl;
        exit(-1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server -> h_addr, (char *) &serv_addr.sin_addr.s_addr, server -> h_length);
    serv_addr.sin_port = htons(portno);

    //successfully connect to the server if no error message
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cout << "ERROR connecting" <<endl;
        exit(-1);
    }

    /*--------------start to play the game----------------*/
    //welcome message
    cout<<"Welcome to a Puzzle Game!"<<endl;
    cout<<"This game will have multiple puzzles for you!"<<endl;
    cout<<"Let's start!!"<<endl;
    sendName(sock);

    //Level 1: Guess a number
    cout<<"Level 1: Guess a Number!!"<<endl;
    int count = 1;
    sendGuess(sock,count);
    int diff = recvDiff(sock);
    printf("Result of guess: %d",diff);
    cout<<"\n"<<endl;

    //check whether the guess is correct or not
    while(diff != 0){
        count++;
        sendGuess(sock,count);
        diff = recvDiff(sock);
        printf("Result of guess: %d",diff);
        cout<<"\n"<<endl;
    }

    //message
    recvMsg(sock);
    cout<<"\n"<<endl;

    //board
    recvBoard(sock);


    //Level 2: determine whether one string is a permutation of another one 
    cout<<"Level 2: The first string is a permutation of another one: true or false?"<<endl;
    recvString(sock);
    cout<<"\n"<<endl;
    recvString(sock);
    cout<<"\n"<<endl;

    //send the true or false
    sendTwo(sock);
    cout<<"\n"<<endl;
    recvMsg(sock);

    //14. close the connection with the server
    close(sock);
    return 0;
}

/**
 * ask user for the name
 * send the name to the server
*/
void sendName(int sock){
    //store string name into char array: msg
    char msg[NAMELENGTH];
    string name;

    cout<<"Enter your name: ";
    cin >> name;
    cout<<"\n" <<endl;

    //check the user's name is not longer than 100 characters
    if(name.length() >=  NAMELENGTH){
        cout<<"Enter your name: ";
        cin >> name;
        cout<<"\n" <<endl;
    }

    bzero(msg, NAMELENGTH);
    strcpy(msg, name.c_str());  //to char array

    //4. send the name to the server
    long length = name.length();

    //send the length
    long networkInt = htonl(length);

    int lengthSent = send(sock, (void *) &networkInt, sizeof(long), 0);
    if(lengthSent != sizeof(long)) {
        exit(-1);
    }

    //send the name
    int bytesSent = send(sock, (void *) msg, length, 0);
    if(bytesSent != length){
        cout<<"Send failed."<<endl;
        exit(-1);
    }
}

//*************Level One******************
/**
 * ask user for the guess
*/
void sendGuess(int sock, int count) {
    long guess;

    printf("Turn: %d\n", count);
    cout << "Enter a guess: ";

    //check whether the guess is integer
    do{
        while(!(cin >> guess)){
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "ERROR integer" << endl;
            //printf("Turn: %d\n", count);
            cout << "Enter a guess: ";
        }
        if(guess < MIN || guess > MAX){
            cout << "ERROR integer" << endl;
            //printf("Turn: %d\n", count);
            cout << "Enter a guess: ";
        }

    }while(guess < MIN || guess > MAX);

    //send the guess to the server
    long guessInt = htonl(guess);
    int guessSent = send(sock, (void *) &guessInt, sizeof(long), 0);
    if (guessSent != sizeof(long)) {
        exit(-1);
    }
}

/**
 * receive difference from server
*/
int recvDiff(int sock){
    int diffLeft = sizeof(long);    //4 bytes
    long diffInt;
    char *bp = (char *) &diffInt;   //storage
    while(diffLeft){
        int diffRecv = recv(sock, bp, diffLeft, 0);
        if(diffRecv <= 0){
            exit(-1);
        }
        diffLeft = diffLeft - diffRecv;
        bp = bp + diffRecv;
    }
    long hostInt = ntohl(diffInt);
    return hostInt;
}

/**
 * receive victory message from server
*/
void recvMsg(int sock){
    //receive the length of message
    int msgLength = sizeof(long);
    long msgInt;
    char *bp = (char *) &msgInt;
    while(msgLength){
        int msgRecv = recv(sock, bp, msgLength, 0);
        if(msgRecv <= 0){
            exit(-1);
        }
        msgLength = msgLength - msgRecv;
        bp = bp + msgRecv;
    }
    long hostInt = ntohl(msgInt);

    //receive the message
    int msgLeft = (int) hostInt;
    char buffer[TEMP];
    char *msgbp = buffer;
    while(msgLeft){
        int bytesRecv = recv(sock, msgbp, msgLeft, 0);
        if(bytesRecv <= 0){
            exit(-1);
        }
        msgLeft = msgLeft - bytesRecv;
        msgbp = msgbp + bytesRecv;
    }
    cout<<buffer<<endl;
}

/**
 * receive leader board from server
*/
void recvBoard(int sock){
    //receive the length of message
    int msgLength = sizeof(long);
    long msgInt;
    char *bp = (char *) &msgInt;
    while(msgLength){
        int msgRecv = recv(sock, bp, msgLength, 0);
        if(msgRecv <= 0){
            exit(-1);
        }
        msgLength = msgLength - msgRecv;
        bp = bp + msgRecv;
    }
    long hostInt = ntohl(msgInt);

    int msgLeft = (int) hostInt;
    char buffer[TEMP];
    char *msgbp = buffer;
    while(msgLeft){
        int bytesRecv = recv(sock, msgbp, msgLeft, 0);
        if(bytesRecv <= 0){
            exit(-1);
        }
        msgLeft = msgLeft - bytesRecv;
        msgbp = msgbp + bytesRecv;
    }

    //delete unrelated message if possible
    int check = 0;
    for(int i = 0; i < (int)strlen(buffer); i++){
        if(buffer[i] == ':'){
            buffer[i] = ' ';
        }
        if(buffer[i] == ';'){
            check++;
        }
        if(check == 3){
            buffer[i] = '\0';
        }
    }

    //print the leader board by using delimeter 
    cout<<"***Level 1***"<<endl;
    cout<<"Leader board: "<<endl;
    char *pch;
    pch = strtok (buffer,";");
    int num = 1;
    while(pch != NULL){
        printf("%d. %s",num,pch);
        cout<<"\n"<<endl;
        pch = strtok(NULL, ";");
        num++;
    }
}

//*************Level Two******************
/**
 * receive two strings from server
*/
void recvString(int sock){
    //receive the length of string
    int wordLength = sizeof(long);
    long wordInt;
    char *bp = (char *) &wordInt;
    while(wordLength){
        int wordRecv = recv(sock, bp, wordLength, 0);
        if(wordRecv <= 0){
            exit(-1);
        }
        wordLength = wordLength - wordRecv;
        bp = bp + wordRecv;
    }
    long hostInt = ntohl(wordInt);

    //receive the string
    int wordLeft = (int) hostInt;
    char buffer[TEMP];
    char *wordbp = buffer;
    while(wordLeft){
        int bytesRecv = recv(sock, wordbp, wordLeft, 0);
        if(bytesRecv <= 0){
            exit(-1);
        }
        wordLeft = wordLeft - bytesRecv;
        wordbp = wordbp + bytesRecv;
    }
    
    //print the string
    //cout<<buffer<<endl;
    printf("The string is: %s",buffer);
}

/**
 * send the level two guess
 */
void sendTwo(int sock) {
    string guess;
    cout << "Enter a guess: ";

    //check whether the guess is integer
    cin >> guess;

    //store string name into char array: msg
    char msg[TEMP];

    //send the level two guess to the server
    bzero(msg, TEMP);
    strcpy(msg, guess.c_str());  //to char array

    //4. send the guess to the server
    long length = guess.length();

    //send the length
    long networkInt = htonl(length);

    int lengthSent = send(sock, (void *) &networkInt, sizeof(long), 0);
    if(lengthSent != sizeof(long)) {
        exit(-1);
    }

    //send the guess
    int bytesSent = send(sock, (void *) msg, length, 0);
    if(bytesSent != length){
        cout<<"Send failed."<<endl;
        exit(-1);
    }
}


