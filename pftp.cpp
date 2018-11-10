#include <iostream>
#include <string>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <fstream>

using namespace std;

int dataSd2;
int a1, a2, a3, a4, p1, p2;

int socketPoll(int sd) {
    struct pollfd pfd[1];
    pfd[0].fd = sd;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;

    return poll(pfd, 1, 1000);
}

int main(int argc, char *argv[]) {

    char hostname[64];
    char file[64];
    int port = 21;
    char username[32] = "anonymous";
    char password[32] = "user@localhost.localnet";
    char mode[20] = "binary";
    char log[32] = "";
    if (argc < 5) {
        cerr << "not enough arguments provided" << endl;
        exit(0);
    }

    if (strcmp(argv[1], "-s") == 0) {
        strcpy(hostname, argv[2]);
    } else {
        cerr << "host name not provided" << endl;
        exit(0);
    };
    if (strcmp(argv[3], "-f") == 0) {
        strcpy(file, argv[4]);
    } else {
        cerr << "file name not provided" << endl;
        exit(0);
    }

    if (argc % 2 == 0) {
        cerr << "incorrect number of arguments" << endl;
        exit(0);
    }
    if (argc > 5) {
        for (int i = 5; i < argc - 1; i += 2) {
            if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "-port") == 0) {
                port = atoi(argv[i + 1]);
            } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "-username") == 0) {
                strcpy(username, argv[i + 1]);
            } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "-password") == 0) {
                strcpy(password, argv[i + 1]);
            } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "-mode") == 0) {
                strcpy(mode, argv[i + 1]);
            } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "-logfile") == 0) {
                strcpy(log, argv[i + 1]);
            } else {
                cerr << "invalid option" << endl;
                exit(0);
            }
        }
    }

    if (strcmp(mode, "binary") == 0) {
        strcpy(mode, "Type I");
    } else if (strcmp(mode, "ASCII") == 0) {
        strcpy(mode, "Type A");
    } else {
        cerr << "invalid mode" << endl;
        exit(0);
    }

    char *ftpUrl = argv[2];
    char connection_buf[64];
    char user_buf[64];
    char pass_buf[64];
    char socket_buf[64];
    char cmd[11];

    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSd < 0) {
        cerr << "Cannot create socket" << endl;
        exit(0);
    }
    struct hostent *host = gethostbyname(ftpUrl);
    struct sockaddr_in addr;
    bzero((char *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr =
            inet_addr(inet_ntoa(*(struct in_addr *) *host->h_addr_list));
    addr.sin_port = htons(port);
    if (connect(clientSd, (sockaddr *) &addr, sizeof(addr)) < 0) {
        cerr << "Cannot connect to ftp server" << endl;
        exit(0);
    }

    read(clientSd, (char *) &connection_buf, sizeof(connection_buf));
    cout << connection_buf;

    strcpy(cmd, "USER ");
    strcat(cmd, username);
    strcat(cmd, "\r\n");

    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &user_buf, sizeof(user_buf));
    cout << user_buf;

    strcpy(cmd, "PASS ");
    strcat(cmd, password);
    strcat(cmd, "\r\n");

    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &pass_buf, sizeof(pass_buf));
    cout << pass_buf;

    if (strcmp(strtok(pass_buf, " "), "230") != 0) {
        exit(0);
    }

    while (socketPoll(clientSd) == 1) {
        read(clientSd, (char *) &socket_buf, sizeof(socket_buf));
        cout << socket_buf << endl;
    }
    if (socketPoll(clientSd) == -1) {
        cerr << "Error polling the socket" << endl;
    }


    char temp[200];
    char verify[100];
    char stream_feedback[100];
    char feedback[100];
    char quit_verify[50];
    char receive_buf[4096];
    int bytes_read;

    strcpy(cmd, mode);
    strcat(cmd, "\r\n");
    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &verify, sizeof(verify));
    cout << verify;


    if (strcmp(strtok(verify, " "), "200") != 0) {
        cerr << "Failed to switch modes" << endl;
        exit(0);
    }
    strcpy(cmd, "PASV");
    strcat(cmd, "\r\n");
    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &temp, sizeof(temp));
    cout << temp;

    sscanf(temp, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
           &a1, &a2, &a3, &a4, &p1, &p2);

    int servPort = (p1 * 256) + p2;
    struct sockaddr_in servAddr3;

    bzero((char *) &servAddr3, sizeof(servAddr3));
    servAddr3.sin_family = AF_INET;
    servAddr3.sin_port = htons(servPort);
    memcpy(&servAddr3.sin_addr, host->h_addr, static_cast<size_t>(host->h_length));

    dataSd2 = socket(AF_INET, SOCK_STREAM, 0);
    if (dataSd2 < 0) {
        cout << "could not create socket" << endl;
        exit(0);
    }

    if (connect(dataSd2, (struct sockaddr *) &servAddr3, sizeof(servAddr3)) < 0) {
        cout << "could not establish connection" << endl;
    }

    strcpy(cmd, "RETR ");
    strcat(cmd, file);
    strcat(cmd, "\r\n");

    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &stream_feedback, sizeof(stream_feedback));
    cout << stream_feedback;
    if (strcmp(strtok(stream_feedback, " "), "150") != 0) {
        exit(0);
    }

    std::ofstream newFile(file, std::ios::out|std::ios::binary);

    do {
        bytes_read = recv(dataSd2, receive_buf, sizeof(receive_buf), 0);
        if (bytes_read > 0) {
            newFile.write(receive_buf, bytes_read);
        }
    } while (bytes_read > 0);


    read(clientSd, (char *) &feedback, sizeof(feedback));
    cout << feedback;

    newFile.close();
    close(dataSd2);

    strcpy(cmd, "QUIT");
    strcat(cmd, "\r\n");
    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &quit_verify, sizeof(quit_verify));
    cout << quit_verify;

    return 0;
}
