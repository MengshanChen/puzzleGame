>> g++ server.cpp -lpthread -o Server -w
>> ./Server 2550
Connection successful
The mystery number is 6808
Here is your name: erica
Receive your guess: 6808
The random string a is rfkqyuqfjkxyqvn
The random string b is rtysfrzrmzlygfv
false
false
Connection successful
The mystery number is 1523
Here is your name: erica
Receive your guess: 1523
The random string a is ulqfpdbhlqdqrrc
The random string b is rwdnxeuoqqeklai
false
true

**********************
client: 
>> ./Client 127.0.0.1 2550
Welcome to a Puzzle Game!
This game will have multiple puzzles for you!
Let's start!!
Enter your name: erica


Level 1: Guess a Number!!
Turn: 1
Enter a guess: 1523
Result of guess: 0

Congratulations! It took 1 turns to guess the number!


***Level 1***
Leader board:
1. erica 1

2. erica 1

Level 2: The first string is a permutation of another one: true or false?
The string is: ulqfpdbhlqdqrrc

The string is: rwdnxeuoqqeklai

Enter a guess: true


Fail!