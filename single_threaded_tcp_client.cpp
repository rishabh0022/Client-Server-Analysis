#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fstream>

typedef struct ProcessInfo {
    char name[256];
    int pid;
    long kernel_time;
    long user_time;
    long total_time;
} ProcessInfo;

void iterate_and_fetch_from_file( DIR* dir, char *res,dirent* entry , ProcessInfo top_processes []){
      while ((entry = readdir(dir)) != NULL) {
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            char stat_path[2048];
            sprintf(stat_path, "/proc/%d/stat", pid);

            // Open the /proc/[pid]/stat file
            std::ifstream stat_file(stat_path);
            if (stat_file) {
                struct ProcessInfo process;
                process.pid = pid;

                std::string buffer;
                getline(stat_file, buffer);
                stat_file.close();
                 long user_time=0;
                 long kernel_time=0;
                // Parse the required fields from the stat file (pid, name, user time, kernel time)
                sscanf(buffer.c_str(), "%d (%[^)]) %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %ld %ld",
                       &process.pid, process.name, &user_time, &kernel_time);
                process.user_time= user_time;
                process.kernel_time = kernel_time;
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
//  get top two CPU consuming processes
void top_two_cpu_consumers(char* res) {
    ProcessInfo top_processes[2] = {{.total_time = -1}, {.total_time = -1}};
    DIR *dir;
    struct dirent *entry;

    // try to open the /proc directory 
    if ((dir = opendir("/proc")) == NULL) {
        perror("opendir failed");
        strcpy(res ,"Failed to read /proc directory.\n");
        return;
    }
    // iterate over each file in the / proc directory 
    iterate_and_fetch_from_file(dir, res, entry , top_processes);

    closedir(dir);

    //copy the result into the res memory 
    snprintf(res, 2048,
             "Top 2 CPU consumers are :\n"
             "1. Process: %s (PID: %d) - Total CPU time: %ld ticks (User: %ld, Kernel: %ld)\n"
             "2. Process: %s (PID: %d) - Total CPU time: %ld ticks (User: %ld, Kernel: %ld)\n",
             top_processes[0].name, top_processes[0].pid, top_processes[0].total_time,
             top_processes[0].user_time, top_processes[0].kernel_time,
             top_processes[1].name, top_processes[1].pid, top_processes[1].total_time,
             top_processes[1].user_time, top_processes[1].kernel_time);
    return ;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[2048]={0};
    int addrlen = sizeof(address);
    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port 8080" <<std::endl;

    while (true) {
        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "Connection accepted from client" << std::endl;

        // Read the request from the client
        int bytes_read = read(new_socket, buffer, 2048);
        if (bytes_read > 0) {
            std::cout << "Received request: " << buffer << std::endl;

            // Prepare the response with top CPU consumers
            char result[2048] = {0};
            top_two_cpu_consumers(result);

            // Send the response back to the client
            send(new_socket, result, strlen(result), 0);
            std::cout << "Sent result to client" << std::endl;
        }

        // Close the client connection after serving one request
        close(new_socket);
    }

    return 0;
}
