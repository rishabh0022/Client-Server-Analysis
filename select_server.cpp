#include <iostream> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/select.h>
#include <dirent.h>
#include <stdio.h>
#include <cerrno>
#include <fstream>
typedef struct ProcessNode{
    char name[256];
    int pid;
    long user_time;
    long kernel_time;
    long total_time;
} ProcessNode;

void iterate_and_fetch_from_file( DIR* dir, char *res,dirent* entry , ProcessNode top_processes []){
      while ((entry = readdir(dir)) != NULL) {
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            char stat_path[2048];
            sprintf(stat_path, "/proc/%d/stat", pid);

            // Open the /proc/[pid]/stat file
            std::ifstream stat_file(stat_path);
            if (stat_file) {
                ProcessNode process;
                process.pid = pid;

                std::string buffer;
                getline(stat_file, buffer);
                stat_file.close();
                long user_time =0;
                long kernel_time=0;
                // Parse the required fields from the stat file (pid, name, user time, kernel time)
                sscanf(buffer.c_str(), "%d (%[^)]) %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %ld %ld",
                       &process.pid, process.name, &user_time, &kernel_time);
                process.user_time= user_time;
                process.kernel_time= kernel_time;
                process.total_time = process.user_time + process.kernel_time;
                // Check if this process should be in the top 2
                if (process.total_time > top_processes[0].total_time) {
                    top_processes[1] = top_processes[0];
                    top_processes[0] = process;
                } else if (process.total_time > top_processes[1].total_time) {
                    top_processes[1] = process;
                }
            }
        }
    }

}
void get_top_cpu_consumers(char* res) {
    // so we need to store the result in result array 
    ProcessNode top_processes[2] = {{.total_time = 0}, {.total_time = 0}};
    DIR *dir;
    struct dirent *entry;

    // open the /proc directory 
    if ((dir = opendir("/proc")) == NULL) {
        perror("opendir failed");
        strcpy(res, "Failed to read /proc directory.\n");
        return;
    }

    // Iterate over each entry in the /proc directory
    iterate_and_fetch_from_file(dir, res, entry , top_processes);

    closedir(dir);
    snprintf(res, 2048,
             "Top CPU consumers:\n"
             "1. Process: %s (PID: %d) - Total CPU time: %ld ticks (User: %ld, Kernel: %ld)\n"
             "2. Process: %s (PID: %d) - Total CPU time: %ld ticks (User: %ld, Kernel: %ld)\n",
             top_processes[0].name, top_processes[0].pid, top_processes[0].total_time,
             top_processes[0].user_time, top_processes[0].kernel_time,
             top_processes[1].name, top_processes[1].pid, top_processes[1].total_time,
             top_processes[1].user_time, top_processes[1].kernel_time);
}

int main() {
    int server_fd, new_socket, client_socket[20];
    int max_clients = 20, activity, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[2048];

    // Initialize client sockets to 0 to ensure not connected
    for (int i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to a port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is listening on port 8080" <<  std::endl;

    fd_set readfds;

    while (true) {
        // empty the socket set at the start
        FD_ZERO(&readfds);

        // Add server socket to set of sockets
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add client sockets to set
        for (int i = 0; i < max_clients; i++) {
            sd = client_socket[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            // Keep track of the maximum socket descriptor
            max_sd = std::max(max_sd, sd);
        }

        // waits till for any activity on any socket 
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("Select function error");
        }

        // An incoming request 
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            std::cout << "New connection: socket fd is " << new_socket << ", ip is: "
                      << inet_ntoa(address.sin_addr) << ", port: " << ntohs(address.sin_port) << std::endl;

            // Add new socket to client array
            for (int i = 0; i < max_clients; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    std::cout << "Adding new client to list at index " << i << std::endl;
                    break;
                }
            }
        }

        // handle io on each socket 
        for (int i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            // If the fd__isset then it means the socket is ready , and file io is done
            if (FD_ISSET(sd, &readfds)) {
                valread = read(sd, buffer, 2048); 

                if (valread == 0) {
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    std::cout << "Client disconnected: ip " << inet_ntoa(address.sin_addr)
                              << ", port " << ntohs(address.sin_port) << std::endl;

                    close(sd);
                    client_socket[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    std::cout << "Received message: " << buffer << std::endl;

                    // Send the response to the client
                    char response[2048];
                    get_top_cpu_consumers(response);
                    send(sd, response, strlen(response), 0);
                }
            }
        }
    }

    return 0;
}
