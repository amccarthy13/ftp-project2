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

    struct arg_struct *args = (struct arg_struct *) arguments;

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

    read(clientSd, (char *) &connection_buf, sizeof(connection_buf));
    cout << connection_buf;

    strcpy(cmd, "USER ");
    strcat(cmd, args->username);
    strcat(cmd, "\r\n");

    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &user_buf, sizeof(user_buf));
    cout << user_buf;

    strcpy(cmd, "PASS ");
    strcat(cmd, args->password);
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


    strcpy(cmd, args->mode);
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
        cerr << "could not create socket" << endl;
        exit(0);
    }

    if (connect(dataSd2, (struct sockaddr *) &servAddr3, sizeof(servAddr3)) < 0) {
        cerr << "could not establish connection" << endl;
        exit(0);
    }

    strcpy(cmd, "SIZE ");
    strcat(cmd, args->file);
    strcat(cmd, "\r\n");

    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &stream_feedback, sizeof(stream_feedback));
    cout << stream_feedback;

    sscanf(stream_feedback, "213 %d",
           &fileSize);

    int multiplier = fileSize / args->endPos;
    int remainder = fileSize % args->endPos;
    int start = multiplier * args->startPos;
    int end = start + multiplier;
    if (start != 0) {
        start += 1;
    }
    if (args->endPos - 1 == args->startPos) {
        end += remainder - 1;
    }

    char startString[10];
    snprintf(startString, sizeof(startString), "%d", start);

    strcpy(cmd, "REST ");
    strcat(cmd, startString);
    strcat(cmd, "\r\n");

    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &stream_feedback, sizeof(stream_feedback));
    cout << stream_feedback;

    strcpy(cmd, "RETR ");
    strcat(cmd, args->file);
    strcat(cmd, "\r\n");

    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &stream_feedback, sizeof(stream_feedback));
    cout << stream_feedback;
    if (strcmp(strtok(stream_feedback, " "), "150") != 0) {
        exit(0);
    }

    std::ofstream newFile(args->file, std::ios::out | std::ios::binary);

    newFile.seekp(start, ios::beg);
    int total_bytes = 0;
    int new_bytes = 0;
    int bytes_to_read = end - start;
    do {
        bytes_read = recv(dataSd2, receive_buf, sizeof(receive_buf), 0);
        total_bytes += bytes_read;
        if (bytes_read >= 0) {
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

    cout << start << "\n";
    cout << end << "\n";

    newFile.close();
    close(dataSd2);

    read(clientSd, (char *) &stream_feedback, sizeof(stream_feedback));
    cout << stream_feedback;

    strcpy(cmd, "QUIT");
    strcat(cmd, "\r\n");
    write(clientSd, (char *) &cmd, strlen(cmd));
    read(clientSd, (char *) &quit_verify, sizeof(quit_verify));
    cout << quit_verify;

    return nullptr;
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

    int dataSd2;
    int a1, a2, a3, a4, p1, p2;


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
        cerr << "could not create socket" << endl;
        exit(0);
    }

    if (connect(dataSd2, (struct sockaddr *) &servAddr3, sizeof(servAddr3)) < 0) {
        cerr << "could not establish connection" << endl;
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

    std::ofstream newFile(file, std::ios::out | std::ios::binary);

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
