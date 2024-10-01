#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <asm-generic/socket.h>
#include <dirent.h>
typedef struct ProcessNode{
    char name[256];
    int pid;
    long user_time;
    long kernel_time;
    long total_time;
} ProcessNode;
void* client_handler(void* arg);
int main(int argc, char *argv[]){
    int server_fd, new_socket; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen  = sizeof(address);
    if((server_fd= socket(AF_INET , SOCK_STREAM, 0))==0){
        perror("Socket connection failed");
        exit(EXIT_FAILURE);
    }
    // now the connection is established
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port number %d .. \n" , 8080);
    while(1){
        // infinite loop for listening 
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        // Create a new thread to handle the client
        pthread_t client_thread;
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        pthread_create(&client_thread, NULL, client_handler, pclient);
        pthread_detach(client_thread);  // clear the thread resources 
    }
}
void* client_handler(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    char buffer[2048] = {0};
     char result[2048]={0};
    // Read the client re0uest
    read(client_socket, buffer, sizeof(buffer));
    printf("Received request from client: %s\n", buffer);

    // Get top two CPU consuming processes
    //char result[2048] = {0};
    //top_two_cpu_consumers(result);
     ProcessNode top_processes[2] = {{.total_time = 0}, {.total_time = 0}};
    DIR *dir;
    struct dirent *entry;
    
    // Open the /proc directory
    dir = opendir("/proc");
    if ((dir==NULL)) {
        perror("opendir failed");
        strcpy(result, "Failed to read /proc directory.\n");
        return NULL; 
    }
    
    // Iterate over each entry in the /proc directory
    while ((entry = readdir(dir)) != NULL) {
        // Check if the directory name is a PID (numeric value)
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            char stat_path[2048];
            sprintf(stat_path, "/proc/%d/stat", pid);
            
            // Open the /proc/[pid]/stat file
            FILE *stat_file = fopen(stat_path, "r");
            if (stat_file) {
                ProcessNode process;
                process.pid = pid;
                
                // Read the stat file
                char stat_file_data[2048];
                fgets(stat_file_data ,sizeof(stat_file_data), stat_file);
                fclose(stat_file);
                long user_time =0;
                long kernel_time=0;
                // Parse the required fields from the stat file (pid, name, user time, kernel time)
                sscanf(stat_file_data, "%d (%[^)]) %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %ld %ld", 
                       &process.pid, process.name, &user_time, &kernel_time);
                
                // Calculate total CPU time (user time + kernel time)
                process.total_time = user_time + kernel_time;
                process.user_time= user_time;
                process.kernel_time = kernel_time;
                // Check if this process should be in the top 2
                if (process.total_time > top_processes[0].total_time) {
                    top_processes[1] = top_processes[0];  // update current top 1 to top 2
                    top_processes[0] = process;  
                } else if (process.total_time > top_processes[1].total_time) {
                    top_processes[1] = process;  
                }
            }
        }
    }
    
    closedir(dir);
    
    // Format the result string with the top two processes
    snprintf(result, 2048,
             "Top 2 CPU consumers are :\n"
             "1. Process: %s (PID: %d) - Total CPU time: %ld ticks (User: %ld, Kernel: %ld)\n"
             "2. Process: %s (PID: %d) - Total CPU time: %ld ticks (User: %ld, Kernel: %ld)\n",
             top_processes[0].name, top_processes[0].pid, top_processes[0].total_time,
             top_processes[0].user_time, top_processes[0].kernel_time,
             top_processes[1].name, top_processes[1].pid, top_processes[1].total_time,
             top_processes[1].user_time, top_processes[1].kernel_time);

    // Send the result back to the client
    send(client_socket, result, strlen(result), 0);
    close(client_socket);
    return NULL;
}
