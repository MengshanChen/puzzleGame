#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <unordered_map>


using namespace std;

sem_t board;
const int NAMELENGTH = 100;
const int MAX = 9999;
const int MIN = 0;
const int TEMP = 256;
const int TOP = 3;             //leader board size
const int MAXBOARD = 400;      //max size of leader board
string arr[TOP];              //store the leader board

/*
 * the function is called when a system call fails
 * it displays a message about the error on stderr
 * and then aborts the program
 */
void error(const char *msg){
    perror(msg);
    exit(1);
}

struct ThreadArgs{
    int clientSock;
};

string recvName(int);
int recvGuess(int);
int sendDiff(int, int, int);
void sendMsg(int,int);
void setleaderBoard(int, string, string arr[]);
string randomString();
void sendString(int, string);
bool permutation(string, string);
string recvStringGuess(int);
void sendTwo(int, string);
void processClient(int);
void *threadMain(void *args);

/*
 * require command line: port number
 */
int main(int argc, char *argv[]) {

    sem_init(&board, 0, 1);

    int sockfd, newsockfd;
    int portno = atoi(argv[1]);
    struct sockaddr_in serv_addr;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(-1);
    }

    //socket creation
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cout<< "ERROR opening socket"<<endl;
        exit(-1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    //bind
    if ((bind(sockfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr))) < 0)
    {
        cout << "Error binding connection" << endl;
        exit(-1);
    }

    //listen
    int test = listen(sockfd, 5);
    if(test < 0){
        printf("Could not listen on socket!");
        exit(-1);
    }

    while(true) {

        struct sockaddr_in clientAddr;

        socklen_t addrLen = sizeof(clientAddr);

        //accept
        newsockfd = accept(sockfd, (struct sockaddr *) &clientAddr, &addrLen);
        if (newsockfd < 0) {
            cout << "ERROR on accept" << endl;
            exit(-1);
        }else{
            cout << "Connection successful" << endl;
        }

        //create and initialzie argumnet struct
        ThreadArgs *threadArgs = new ThreadArgs;
        threadArgs->clientSock = newsockfd;

        //3. create client thread
        pthread_t threadID;
        int status = pthread_create(&threadID, nullptr, threadMain, (void *) threadArgs);
        if (status != 0) {
            exit(-1);
        }

    }
}

/**
 * receive name from client
 */
string recvName(int sock){
    //receive the size of name
    int nameLength = sizeof(long);
    long nameInt;
    char *bp = (char *) &nameInt;
    while(nameLength){
        int nameRecv = recv(sock, bp, nameLength, 0);
        if(nameRecv <= 0){
            exit(-1);
        }
        nameLength = nameLength - nameRecv;
        bp = bp + nameRecv;
    }
    long hostInt = ntohl(nameInt);

    //receive the name
    int nameLeft = (int) hostInt;
    char buffer[TEMP];
    bzero(buffer, TEMP);
    char *namebp = buffer;
    while(nameLeft){
        int bytesRecv = recv(sock, namebp, nameLeft, 0);
        if(bytesRecv <= 0){
            exit(-1);
        }
        nameLeft = nameLeft - bytesRecv;
        namebp = namebp + bytesRecv;
    }

    string result;
    printf("Here is your name: %s\n",buffer);
    for(int i = 0; i < (int)strlen(buffer); i++){
        result += buffer[i];
    }
    return result;
}

//************* Level One ******************
/**
 * receive guess from client
 */
int recvGuess(int sock){
    int guessLeft = sizeof(long);    //4 bytes
    long guessInt;
    char *bp = (char *) &guessInt;   //storage
    while(guessLeft){
        int guessRecv = recv(sock, bp, guessLeft, 0);
        if(guessRecv <= 0){
            exit(-1);
        }
        guessLeft = guessLeft - guessRecv;
        bp = bp + guessRecv;
    }
    long hostInt = ntohl(guessInt);
    printf("Receive your guess: %ld\n", hostInt);
    return hostInt;
}

//send difference to client
int sendDiff(int sock, int random_integer, int guess){
    long diff = random_integer - guess;
    long networkInt = htonl(diff);

    int diffSent = send(sock, (void *) &networkInt, sizeof(long), 0);
    if(diffSent != sizeof(long)) {
        exit(-1);
    }
    return (int)diff;
}

/**
 * send victory message to client
 */
void sendMsg(int sock, int count){
    string msg1 = "Congratulations! It took ";

    stringstream ss;
    ss << count;
    string msg2 = ss.str();

    string msg3 = " turns to guess the number!";
    string msg = msg1 + msg2 + msg3;

    long length = msg.length();

    //send the length
    long networkInt = htonl(length);

    int lengthSent = send(sock, (void *) &networkInt, sizeof(long), 0);
    if(lengthSent != sizeof(long)) {
        exit(-1);
    }

    //send the message
    char msgArr[TEMP];
    bzero(msgArr, TEMP);
    strcpy(msgArr, msg.c_str());
    int bytesSent = send(sock, (void *) msgArr, length, 0);
    if(bytesSent != length){
        cout<<"Send failed."<<endl;
        exit(-1);
    }
}

/**
 * send leader board to client
*/
void setleaderBoard(int sock, string input, string arr[]){
    sem_wait(&board);
    if(arr[0] == ""){
        arr[0] = input;
    }else{
        //compare with the first one
        string temp = input.substr(input.find(":") + 1);
        stringstream ss(temp);
        int tempCount;
        ss >> tempCount;

        string first = arr[0].substr(arr[0].find(":") + 1);
        stringstream ss1(first);
        int firstCount;
        ss1 >> firstCount;

        if(tempCount < firstCount){
            if(arr[1] == ""){
                arr[1] = arr[0];
                arr[0] = input;
            }else{
                arr[2] = arr[1];
                arr[1] = arr[0];
                arr[0] = input;
            }
        }else{
            //compare with the second one
            if(arr[1] == ""){
                arr[1] = input;
            }else{
                string second = arr[1].substr(arr[1].find(":") + 1);
                stringstream ss2(second);
                int secondCount;
                ss2 >> secondCount;

                if(tempCount < secondCount){
                    arr[2] = arr[1];
                    arr[1] = input;
                }else{
                    //compare with the third one
                    if(arr[2] == ""){
                        arr[2] = input;
                    }else{
                        string third = arr[2].substr(arr[2].find(":") + 1);
                        stringstream ss3(third);
                        int thirdCount;
                        ss3 >> thirdCount;

                        if(tempCount < thirdCount){
                            arr[2] = input;
                        }
                    }
                }
            }
        }
    }
    string msg;
    for(int i = 0; i < TOP; i++){
        msg += arr[i];
        msg += ";";
    }

    long length = msg.length();

    //send the length
    long networkInt = htonl(length);

    int lengthSent = send(sock, (void *) &networkInt, sizeof(long), 0);
    if(lengthSent != sizeof(long)) {
        exit(-1);
    }

    //send the message
    char msgArr[MAXBOARD];
    bzero(msgArr, MAXBOARD);
    strcpy(msgArr, msg.c_str());
    int bytesSent = send(sock, (void *) msgArr, length, 0);
    if(bytesSent != length){
        cout<<"Send failed."<<endl;
        exit(-1);
    }
    sem_post(&board);

}

//************* Level Two ******************
string randomString(){
    const int ALPHA = 26;
    char alphabet[ALPHA] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 
                          'h', 'i', 'j', 'k', 'l', 'm', 'n',  
                          'o', 'p', 'q', 'r', 's', 't', 'u', 
                          'v', 'w', 'x', 'y', 'z' }; 
    int n = 15;
    string res = "";
    for(int i = 0; i < n; ++i){
        res = res + alphabet[rand() % ALPHA];
    }
    return res;
}

/**
 * send two strings to client
*/
void sendString(int sock, string word){
    long length = word.length();

    //send the length
    long networkInt = htonl(length);

    int lengthSent = send(sock, (void *) &networkInt, sizeof(long), 0);
    if(lengthSent != sizeof(long)) {
        exit(-1);
    }

    //send the random string
    char wordArr[TEMP];
    bzero(wordArr, TEMP);
    strcpy(wordArr, word.c_str());
    int bytesSent = send(sock, (void *) wordArr, length, 0);
    if(bytesSent != length){
        cout<<"Send failed."<<endl;
        exit(-1);
    }
}

/**
 * determine whether one string is a permutation of another one 
 */
bool permutation(string s, string t){
    unordered_map<char, int> letter;
    for(int i = 0; i < s.length(); i++){
        letter[s[i]]++;
    }
    for(int i = 0; i < t.length(); i++){
        letter[t[i]]--;
        if(letter[t[i]] < 0){
            return false;
        }
    }
    return true;
}

/**
 * receive string guess from client
 */
string recvStringGuess(int sock){
    //receive the size of string guess
    int stringLength = sizeof(long);
    long stringInt;
    char *bp = (char *) &stringInt;
    while(stringLength){
        int stringRecv = recv(sock, bp, stringLength, 0);
        if(stringRecv <= 0){
            exit(-1);
        }
        stringLength = stringLength - stringRecv;
        bp = bp + stringRecv;
    }
    long hostInt = ntohl(stringInt);

    //receive the guess string
    int stringLeft = (int) hostInt;
    char buffer[TEMP];
    bzero(buffer, TEMP);
    char *stringbp = buffer;
    while(stringLeft){
        int bytesRecv = recv(sock, stringbp, stringLeft, 0);
        if(bytesRecv <= 0){
            exit(-1);
        }
        stringLeft = stringLeft - bytesRecv;
        stringbp = stringbp + bytesRecv;
    }

    string result;
    for(int i = 0; i < (int)strlen(buffer); i++){
        result += buffer[i];
    }
    return result;
}

/**
 * send messsage
 */
void sendTwo(int sock, string word){
    long length = word.length();

    //send the length
    long networkInt = htonl(length);

    int lengthSent = send(sock, (void *) &networkInt, sizeof(long), 0);
    if(lengthSent != sizeof(long)) {
        exit(-1);
    }

    //send the random string
    char wordArr[TEMP];
    bzero(wordArr, TEMP);
    strcpy(wordArr, word.c_str());
    int bytesSent = send(sock, (void *) wordArr, length, 0);
    if(bytesSent != length){
        cout<<"Send failed."<<endl;
        exit(-1);
    }
}

//************* Thread Function ******************
/**
 * run the program
*/
void processClient(int sock) {
    //Level One
    //1. generate random number
    int random_integer = (rand() % MAX);
    printf("The mystery number is %d\n",random_integer);

    //2.receive name
    string name = recvName(sock);

    //3.receive guess
    int count = 1;
    int value = recvGuess(sock);

    //4.send the difference
    int diff = sendDiff(sock, random_integer, value);
    while(diff != 0){
        count++;
        value = recvGuess(sock);
        diff = sendDiff(sock, random_integer, value);
    }

    //5.send the message
    sendMsg(sock, count);
    string mark1 = ":";
    stringstream sstr;
    sstr << count;
    string strCount = sstr.str();
    string input = name + mark1 + strCount;

    //6. send board
    setleaderBoard(sock,input, arr);

    //Level Two
    //1. generate a random string
    string astring = randomString();
    string bstring = randomString();
    printf("The random string a is %s\n",astring.c_str());
    printf("The random string b is %s\n",bstring.c_str());
    sendString(sock, astring);
    sendString(sock, bstring);

    //2. determine true or false
    bool twoboolean = permutation(astring, bstring);
    string twoResult;
    if(twoboolean){
        twoResult = "true";
    }else{
        twoResult = "false";
    }
    cout<<twoResult<<endl;

    //3.receive guess
    string recvString = recvStringGuess(sock);
    cout<<recvString<<endl;

    //4. determine true or false
    if(recvString.compare(twoResult) != 0){
        string wrong = "Fail!";
        sendTwo(sock,wrong);
    } else {
        string word = "Congratulations!";
        sendTwo(sock,word);
    }
}


/**
 * thread main function
*/
void *threadMain(void *args){
    //extract socket file descriptor from argument
    auto *threadArgs = (struct ThreadArgs *)args;
    int clientSock = threadArgs -> clientSock;
    delete threadArgs;

    //communicate with client
    processClient(clientSock);

    //reclaim resources before finishing
    pthread_detach(pthread_self());

    // 9. close the connection with the client
    close(clientSock);
    return nullptr;
}


