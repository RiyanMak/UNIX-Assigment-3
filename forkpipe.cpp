#include <stdlib.h>
#include <string.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <time.h>

#include "LineInfo.h"

using namespace std;

static const char* const QUOTES_FILE_NAME = "quotes.txt"; //file for "quotes.txt"

//intializing const variables
int const READ = 0;
int const WRITE = 1;
int const PIPE_ERROR = -1;
int const FORK_ERROR = -1;
int const CHILD_PID = 0;
int const MAX_PIPE_MESSAGE_SIZE = 1000;
int const MAX_QUOTE_LINE_SIZE = 1000;

//reads lines from quotes.txt and stores them in lines array while storing the number of lines in the noLines variable
void getQuotesArray(char* lines[], unsigned& noLines) {
    FILE* inputFilePointer;
    inputFilePointer = fopen(QUOTES_FILE_NAME, "r");
    if (inputFilePointer == NULL) {
        throw domain_error(LineInfo("file open Error", __FILE__, __LINE__));
    }
    char input[MAX_QUOTE_LINE_SIZE];
    int size = sizeof(input);
    while (fgets(input, size, inputFilePointer) != NULL) {
        lines[noLines] = strdup(input);
        noLines++;
    }
}

void executeParentProcess(int pipeParentWriteChildReadfds[], int pipeParentReadChildWritefds[], int noOfParentMessages2Send) {
    if (close(pipeParentReadChildWritefds[WRITE]) == PIPE_ERROR)
        throw domain_error(LineInfo("close pipe Error", __FILE__, __LINE__));

    for (unsigned i = 0; i < noOfParentMessages2Send; i++) {
        cout << "In Parent: Write to pipe getQuoteMessage sent Message: Get Quote" << endl;
        if (write(pipeParentWriteChildReadfds[WRITE], "Get Quote", sizeof("Get Quote")) == PIPE_ERROR)
            throw domain_error(LineInfo("write pipe Error", __FILE__, __LINE__));
        char ParentReadChildMessage[MAX_PIPE_MESSAGE_SIZE] = { 0 };
        if (read(pipeParentReadChildWritefds[READ], ParentReadChildMessage, sizeof(ParentReadChildMessage)) == PIPE_ERROR) {
            throw domain_error(LineInfo("read pipe Error", __FILE__, __LINE__));
        }
        cout << "In Parent: Read from pipe pipeParentReadChildMessage read Message:" << ParentReadChildMessage << endl << "-----------------------------------" << endl;
    }
    cout << "In Parent: Write to pipe ParentWriteChildExitMessage sent Message: Exit" << endl;
    if (write(pipeParentWriteChildReadfds[WRITE], "Exit", sizeof("Exit")) == PIPE_ERROR) {
        throw domain_error(LineInfo("write pipe Error", __FILE__, __LINE__));
    }
    if (close(pipeParentWriteChildReadfds[WRITE]) == PIPE_ERROR)
        throw domain_error(LineInfo("close write pipe Error", __FILE__, __LINE__));
    if (close(pipeParentReadChildWritefds[READ]) == PIPE_ERROR) {
        throw domain_error(LineInfo("close read pipe Error", __FILE__, __LINE__));
    }
    cout << "Parent Done" << endl;
}

void executeChildProcess(int pipeParentWriteChildReadfds[], int pipeParentReadChildWritefds[], char* lines[], unsigned noLines) {
    if (close(pipeParentWriteChildReadfds[WRITE]) == PIPE_ERROR)
        throw domain_error(LineInfo("close write pipe Error", __FILE__, __LINE__));
    if (close(pipeParentReadChildWritefds[READ]) == PIPE_ERROR) {
        throw domain_error(LineInfo("close read pipe Error", __FILE__, __LINE__));
    }
    time_t t;
    srand((unsigned)time(&t));
    do {
        char recievedMesssage[MAX_PIPE_MESSAGE_SIZE] = { 0 };
        if (read(pipeParentWriteChildReadfds[READ], recievedMesssage, sizeof(recievedMesssage)) == PIPE_ERROR)
            throw domain_error(LineInfo("read pipe Error", __FILE__, __LINE__));
        cout << "In Child : Read from pipe pipeParentWriteChildMessage read Message: " << recievedMesssage << endl;
        //check message
        char* indexPointer;
        indexPointer = strstr(recievedMesssage, "Exit");
        if (indexPointer != NULL) {
            break;
        }
        indexPointer = strstr(recievedMesssage, "Get Quote");
        if (indexPointer != NULL) {
            int randomLineChoice = (rand() % noLines);
            char quoteMessage[1000] = { 0 };
            unsigned size = strlen(lines[randomLineChoice]);
            for (unsigned i = 0; i < size; i++) {
                quoteMessage[i] = *(lines[randomLineChoice] + i);
            }
            // Write to the pipe before closing the write end
            cout << "In Child : Write to pipe pipeParentReadChildMessage sent Message:" << quoteMessage;
            if (write(pipeParentReadChildWritefds[WRITE], quoteMessage, sizeof(quoteMessage)) == PIPE_ERROR) {
                throw domain_error(LineInfo("write pipe Error", __FILE__, __LINE__));
            }
        }
        else {
            cout << "In Child: Invalid message recieved: " << endl << recievedMesssage << endl;
            cout << "In Child: Write to pipe pipeParentReadChildMessage sent" << endl << "Message: " << recievedMesssage << endl;
            if (write(pipeParentReadChildWritefds[WRITE], recievedMesssage, sizeof(recievedMesssage)) == PIPE_ERROR) {
                throw domain_error(LineInfo("write pipe Error", __FILE__, __LINE__));
            }
        }
    } while (true);
    if (close(pipeParentWriteChildReadfds[READ]) == PIPE_ERROR) {
        throw domain_error(LineInfo("close read pipe Error", __FILE__, __LINE__));
    }
    if (close(pipeParentReadChildWritefds[WRITE]) == PIPE_ERROR) {
        throw domain_error(LineInfo("close write pipe Error", __FILE__, __LINE__));
    }
    cout << "Child Done" << endl;
}

int main(int argc, char* argv[]) {

    //Try statement to specify the usage of forkpipe.cpp. We need exactly 2 command-line arguments.
    try {
        if (argc != 2)
            throw domain_error(LineInfo("Usage: ./forkpipe <number>", __FILE__, __LINE__));

        //argv[0] is the program name, argv[1] is the number, atoi = ascii to int
        int      noOfParentMessages2Send = atoi(argv[1]);
        char* lines[1000];
        unsigned noLines = 0;  // Initialize noLines to 0

        //retrieve quotes and store them in lines array while storing the number of lines in noLines
        getQuotesArray(lines, noLines);

        int pipeParentWriteChildReadfds[2],
            pipeParentReadChildWritefds[2];
        int pid;

        // create pipes    
        if (pipe(pipeParentWriteChildReadfds) == PIPE_ERROR)
            throw domain_error(LineInfo("Unable to create pipe pipeParentWriteChildReadfds", __FILE__, __LINE__));

        if (pipe(pipeParentReadChildWritefds) == PIPE_ERROR)
            throw domain_error(LineInfo("Unable to create pipe pipeParentReadChildWritefds", __FILE__, __LINE__));

        pid = fork();

        if (pid == FORK_ERROR)
            throw domain_error(LineInfo("Fork Error", __FILE__, __LINE__));

        else if (pid != CHILD_PID) {
            // Parent process
            executeParentProcess(pipeParentWriteChildReadfds, pipeParentReadChildWritefds, noOfParentMessages2Send);
        }
        else {
            //child process
            executeChildProcess(pipeParentWriteChildReadfds, pipeParentReadChildWritefds, lines, noLines);
        }
    }//try
    catch (exception& e) {
        cout << e.what() << endl;
        cout << endl << "Press the enter key once or twice to leave..." << endl;
        cin.ignore(); cin.get();
        exit(EXIT_FAILURE);
    }//catch

    exit(EXIT_SUCCESS);
}