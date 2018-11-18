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
#include <thread>
#include <vector>
#include <csignal>

using namespace std;

struct arg_struct {
    char hostname[32];
    char file[64];
    int port;
    char username[32];
    char password[32];
    char mode[20];
    char log[32];
    int startPos;
    int endPos;
};


pthread_t threads[15];


int socketPoll(int sd) {
    struct pollfd pfd[1];
    pfd[0].fd = sd;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;

    return poll(pfd, 1, 1000);
}

void *downloadHandler(void *arguments) {
    int dataSd2;
    int a1, a2, a3, a4, p1, p2;
    int fileSize;
    bool logFlag;

    struct arg_struct *args = (struct arg_struct *) arguments;

    char user[11];
    char pass[16];
    char buffer[1500];
    char server_log[512];
    char client_log[512];

    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSd < 0) {
        cerr << "Cannot create socket" << endl;
        exit(0);
    }
    struct hostent *host = gethostbyname(args->hostname);
    struct sockaddr_in addr;
    bzero((char *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr =
            inet_addr(inet_ntoa(*(struct in_addr *) *host->h_addr_list));
    addr.sin_port = htons(args->port);
    if (connect(clientSd, (sockaddr *) &addr, sizeof(addr)) < 0) {
        cerr << "Cannot connect to ftp server" << endl;
        exit(0);
    }

    std::ofstream log_file(args->log, std::ios::out | std::ios::binary);

    logFlag = strcmp(args->log, "-") != 0;

    read(clientSd, (char *) &buffer, sizeof(buffer));
    strcpy(server_log, "S->C ");
    strcat(server_log, buffer);
    if (logFlag) {
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << server_log;
    }

    if (strcmp(strtok(buffer, " "), "220") != 0) {
        cerr << "Failed to connect to server" << endl;
        exit(0);
    }

    strcpy(user, "USER ");
    strcat(user, args->username);
    strcat(user, "\r\n");

    write(clientSd, (char *) &user, strlen(user));
    read(clientSd, (char *) &buffer, sizeof(buffer));
    strcpy(server_log, "S->C ");
    strcat(server_log, buffer);
    strcpy(client_log, "C->S ");
    strcat(client_log, user);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }
    if (strcmp(strtok(buffer, " "), "331") != 0) {
        cerr << "Failed to login" << endl;
        exit(0);
    }


    strcpy(pass, "PASS ");
    strcat(pass, args->password);
    strcat(pass, "\r\n");

    write(clientSd, (char *) &pass, strlen(pass));
    read(clientSd, (char *) &buffer, sizeof(buffer));
    strcpy(server_log, "S->C ");
    strcat(server_log, buffer);
    strcpy(client_log, "C->S ");
    strcat(client_log, pass);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }
    if (strcmp(strtok(buffer, " "), "230") != 0) {
        cerr << "Failed to login" << endl;
        exit(0);
    }

    while (socketPoll(clientSd) == 1) {
        read(clientSd, (char *) &buffer, sizeof(buffer));
    }
    if (socketPoll(clientSd) == -1) {
        cerr << "Error polling the socket" << endl;
    }
    
    char type[12];
    strcpy(type, args->mode);
    strcat(type, "\r\n");
    char verify[100];
    write(clientSd, (char *) &type, strlen(type));
    read(clientSd, (char *) &verify, sizeof(verify));
    strcpy(server_log, "S->C ");
    strcat(server_log, verify);
    strcpy(client_log, "C->S ");
    strcat(client_log, type);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }

    if (strcmp(strtok(verify, " "), "200") != 0) {
        cerr << "Failed to switch modes" << endl;
        exit(0);
    }

    char passive[30];
    char temp[100];
    strcpy(passive, "PASV");
    strcat(passive, "\r\n");
    write(clientSd, (char *) &passive, strlen(passive));
    read(clientSd, (char *) &temp, sizeof(temp));
    strcpy(server_log, "S->C ");
    strcat(server_log, temp);
    strcpy(client_log, "C->S ");
    strcat(client_log, passive);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }

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
        cerr << "could not create socket" << endl;
        exit(0);
    }

    if (connect(dataSd2, (struct sockaddr *) &servAddr3, sizeof(servAddr3)) < 0) {
        cerr << "could not establish connection" << endl;
        exit(0);
    }

    char size[30];
    char temp2[100];
    strcpy(size, "SIZE ");
    strcat(size, args->file);
    strcat(size, "\r\n");

    write(clientSd, (char *) &size, strlen(size));
    read(clientSd, (char *) &temp2, sizeof(temp2));
    strcpy(server_log, "S->C ");
    strcat(server_log, temp2);
    strcpy(client_log, "C->S ");
    strcat(client_log, size);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }

    sscanf(temp2, "213 %d",
           &fileSize);

    int multiplier = fileSize / args->endPos;
    int remainder = fileSize % args->endPos;
    int start = multiplier * args->startPos;
    int end = start + multiplier - 1;
    if (args->endPos - 1 == args->startPos) {
        end += remainder;
    }

    char startString[10];
    snprintf(startString, sizeof(startString), "%d", start);

    char rest[30];
    char temp3[100];
    strcpy(rest, "REST ");
    strcat(rest, startString);
    strcat(rest, "\r\n");

    write(clientSd, (char *) &rest, strlen(rest));
    read(clientSd, (char *) &temp3, sizeof(temp3));
    strcpy(server_log, "S->C ");
    strcat(server_log, temp3);
    strcpy(client_log, "C->S ");
    strcat(client_log, rest);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }
    if (strcmp(strtok(temp3, " "), "350") != 0) {
        cerr << "Could not set restart position" << endl;
        exit(0);
    }

    char retr[10];
    char receive_buf[1024];
    char feedback[200];
    strcpy(retr, "RETR ");
    strcat(retr, args->file);
    strcat(retr, "\r\n");

    write(clientSd, (char *) &retr, strlen(retr));
    read(clientSd, (char *) &feedback, sizeof(feedback));
    strcpy(server_log, "S->C ");
    strcat(server_log, feedback);
    strcpy(client_log, "C->S ");
    strcat(client_log, retr);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }
    if (strcmp(strtok(feedback, " "), "150") != 0) {
        cerr << "Could not initiate file retrieval" << endl;
        exit(0);
    }

    std::ofstream newFile(args->file, std::ios::out | std::ios::binary);

    newFile.seekp(start);
    int total_bytes = 0;
    int new_bytes = 0;
    int bytes_read = 0;
    int bytes_to_read = end - start + 1;
    do {
        bytes_read = recv(dataSd2, receive_buf, sizeof(receive_buf), 0);
        total_bytes += bytes_read;
        if (bytes_read > 0) {
            if (total_bytes > bytes_to_read) {
                new_bytes = total_bytes - bytes_to_read;
                new_bytes = bytes_read - new_bytes;
                newFile.write(receive_buf, new_bytes);
                break;
            } else {
                newFile.write(receive_buf, bytes_read);
            }
        }
    } while (bytes_read > 0);

    newFile.close();
    close(dataSd2);

    char quit[10];
    strcpy(quit, "QUIT");
    strcat(quit, "\r\n");
    write(clientSd, (char *) &quit, strlen(quit));

    if (!logFlag) {
        remove(args->log);
    }
    return nullptr;
}


int main(int argc, char *argv[]) {
    char hostname[64];
    char file[64];
    int port = 21;
    char username[32] = "anonymous";
    char password[32] = "user@localhost.localnet";
    char mode[20] = "binary";
    char log[32] = "-";
    bool logFlag;
    if (argc < 5) {
        cerr << "not enough arguments provided" << endl;
        exit(0);
    }

    if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "-thread") == 0) {
        if (argc > 3) {
            for (int i = 3; i < argc - 1; i += 2) {
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

        char input[100];
        int streamCount = 0;
        struct arg_struct stream_args[15];
        ifstream myFile;
        myFile.open(argv[2]);
        if (myFile.is_open()) {
            while (!myFile.eof()) {
                myFile >> input;
                char *token = strtok(input, ":/@");
                if (strcmp(token, "ftp") != 0) {
                    cerr << "invalid config file" << endl;
                    exit(0);
                }

                token = strtok(nullptr, ":/@");
                if (token == nullptr) {
                    cerr << "invalid config file" << endl;
                    exit(0);
                }
                strcpy(username, token);

                token = strtok(nullptr, ":/@");
                if (token == nullptr) {
                    cerr << "invalid config file" << endl;
                    exit(0);
                }
                strcpy(password, token);

                token = strtok(nullptr, ":/@");
                if (token == nullptr) {
                    cerr << "invalid config file" << endl;
                    exit(0);
                }
                strcpy(hostname, token);

                token = strtok(nullptr, ":/@");
                if (token == nullptr) {
                    cerr << "invalid config file" << endl;
                    exit(0);
                }
                strcpy(file, token);

                struct arg_struct args;
                strcpy(args.hostname, hostname);
                strcpy(args.file, file);
                args.port = port;
                strcpy(args.username, username);
                strcpy(args.password, password);
                strcpy(args.mode, mode);
                strcpy(args.log, log);

                stream_args[streamCount] = args;
                streamCount++;
            }
        }


        pthread_t thread_id;
        for (int i = 0; i < streamCount; i++) {
            stream_args[i].startPos = i;
            stream_args[i].endPos = streamCount;
            pthread_create(&thread_id, nullptr, downloadHandler, (void *) &stream_args[i]);
            threads[i] = thread_id;
        }

        for (int i = 0; i < streamCount; i++) {
            pthread_join(threads[i], nullptr);
            cout << "thread joined\n";
        }
        return 0;
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

    logFlag = strcmp(log, "-") != 0;

    char user_buf[256];
    char pass_buf[512];
    char connect_buf[256];
    char poll_buf[256];
    char user[11];
    char pass[16];
    char server_log[512];
    char client_log[512];

    std::ofstream log_file(log, std::ios::out | std::ios::binary);

    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSd < 0) {
        cerr << "Cannot create socket" << endl;
        exit(0);
    }
    struct hostent *host = gethostbyname(hostname);
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

    read(clientSd, (char *) &connect_buf, sizeof(connect_buf));
    strcpy(server_log, "S->C ");
    strcat(server_log, connect_buf);
    if (logFlag) {
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << server_log;
    }
    if (strcmp(strtok(connect_buf, " "), "220") != 0) {
        exit(0);
    }

    strcpy(user, "USER ");
    strcat(user, username);
    strcat(user, "\r\n");

    write(clientSd, (char *) &user, strlen(user));
    read(clientSd, (char *) &user_buf, sizeof(user_buf));
    strcpy(server_log, "S->C ");
    strcat(server_log, user_buf);
    strcpy(client_log, "C->S ");
    strcat(client_log, user);
    if (logFlag) {
        log_file.write(server_log, strlen(server_log));
        log_file.write(client_log, strlen(client_log));
    } else {
        cout << server_log;
        cout << client_log;
    }

    if (strcmp(strtok(user_buf, " "), "331") != 0) {
        exit(0);
    }

    strcpy(pass, "PASS ");
    strcat(pass, password);
    strcat(pass, "\r\n");

    write(clientSd, (char *) &pass, strlen(pass));
    read(clientSd, (char *) &pass_buf, sizeof(pass_buf));
    strcpy(server_log, "S->C ");
    strcat(server_log, pass_buf);
    strcpy(client_log, "C->S ");
    strcat(client_log, pass);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }

    if (strcmp(strtok(pass_buf, " "), "230") != 0) {
        exit(0);
    }

    while (socketPoll(clientSd) == 1) {
        read(clientSd, (char *) &poll_buf, sizeof(poll_buf));
    }
    if (socketPoll(clientSd) == -1) {
        cerr << "Error polling the socket" << endl;
    }

    int dataSd2;
    int a1, a2, a3, a4, p1, p2;

    char type[11];
    strcpy(type, mode);
    strcat(type, "\r\n");
    char verify[256];
    write(clientSd, (char *) &type, strlen(type));
    read(clientSd, (char *) &verify, sizeof(verify));
    strcpy(server_log, "S->C ");
    strcat(server_log, verify);
    strcpy(client_log, "C->S ");
    strcat(client_log, type);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }

    if (strcmp(strtok(verify, " "), "200") != 0) {
        cerr << "Failed to switch modes" << endl;
        exit(0);
    }

    char passive[30];
    char temp[256];
    strcpy(passive, "PASV");
    strcat(passive, "\r\n");
    write(clientSd, (char *) &passive, strlen(passive));
    read(clientSd, (char *) &temp, sizeof(temp));
    strcpy(server_log, "S->C ");
    strcat(server_log, temp);
    strcpy(client_log, "C->S ");
    strcat(client_log, passive);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }

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
        cerr << "could not create socket" << endl;
        exit(0);
    }

    if (connect(dataSd2, (struct sockaddr *) &servAddr3, sizeof(servAddr3)) < 0) {
        cerr << "could not establish connection" << endl;
        exit(0);
    }

    char retr[10];
    char receive_buf[1024];
    char retrive_feedback[256];
    char transfer_feedback[256];
    char quit_feedback[256];
    strcpy(retr, "RETR ");
    strcat(retr, file);
    strcat(retr, "\r\n");

    write(clientSd, (char *) &retr, strlen(retr));
    read(clientSd, (char *) &retrive_feedback, sizeof(retrive_feedback));
    strcpy(server_log, "S->C ");
    strcat(server_log, retrive_feedback);
    strcpy(client_log, "C->S ");
    strcat(client_log, retr);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout <<client_log;
        cout << server_log;
    }
    if (strcmp(strtok(retrive_feedback, " "), "150") != 0) {
        cerr << "could not retrieve file" << endl;
        exit(0);
    }

    std::ofstream newFile(file, std::ios::out | std::ios::binary);

    int bytes_read = 0;
    do {
        bytes_read = recv(dataSd2, receive_buf, sizeof(receive_buf), 0);
        if (bytes_read > 0) {
            newFile.write(receive_buf, bytes_read);
        }
    } while (bytes_read > 0);
    read(clientSd, (char *) &transfer_feedback, sizeof(transfer_feedback));
    strcpy(server_log, "S->C ");
    strcat(server_log, transfer_feedback);
    if (logFlag) {
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << server_log;
    }

    newFile.close();
    close(dataSd2);

    char quit[10];
    strcpy(quit, "QUIT");
    strcat(quit, "\r\n");
    write(clientSd, (char *) &quit, strlen(quit));
    read(clientSd, (char *) &quit_feedback, sizeof(quit_feedback));
    strcpy(server_log, "S->C ");
    strcat(server_log, quit_feedback);
    strcpy(client_log, "C->S ");
    strcat(client_log, quit);
    if (logFlag) {
        log_file.write(client_log, strlen(client_log));
        log_file.write(server_log, strlen(server_log));
    } else {
        cout << client_log;
        cout << server_log;
    }
    if (!logFlag) {
        remove(log);
    }

    return 0;
}
