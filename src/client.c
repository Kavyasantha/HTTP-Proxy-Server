#include "common.h"

int main(int argc, char * argv[]){
    char buffer[10000];
    unsigned int port_number;
    struct sockaddr_in curr_addr_ip4, remote_addr_ip4;
    int client_fd;
    char request[200];
    char *hostname;
    char *file_path;
    char *file_path_copy;
    char *file_itr;
    int file_count = 0, flag = 0;
    if(argc < 4){
        printf("Run: ./client <server_ip> <port> <url>");
        exit(1);
    }
    
    port_number = atoi(argv[2]);
    bzero(&curr_addr_ip4, sizeof(curr_addr_ip4));
    bzero(&remote_addr_ip4, sizeof(remote_addr_ip4));

    curr_addr_ip4.sin_family = AF_INET;          
    curr_addr_ip4.sin_addr.s_addr = INADDR_ANY;

    remote_addr_ip4.sin_family = AF_INET;
    remote_addr_ip4.sin_port = htons(port_number);
    if(inet_aton(argv[1], (struct in_add *)&remote_addr_ip4.sin_addr.s_addr) == 0){
        perror(argv[1]);
        exit(-1);
    }
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0){
        perror("Socket Creating Error:");
        exit(-1);
    }
    if(connect(client_fd, (struct sockaddr *)(&remote_addr_ip4), sizeof(struct sockaddr)) < 0){
        perror("Connection Error:");
        exit(-1);
    }

    memset(request, 0, 100);
    hostname = strtok(argv[3],"/");
    file_path = strtok(NULL, "");
    sprintf(request,"GET /%s HTTP/1.0\r\nHost:%s\r\n\r\n", file_path, hostname);
    file_itr = file_path;
    while(*file_itr != '\0'){
        if(*file_itr == '/'){
            file_count++;
        }
        file_itr++;
    }
    file_path_copy = file_path;
    if(file_count > 0){
        while(file_count >= 0){
            if(flag == 0){
                strtok(file_path_copy, "/");
                flag = 1;
            }
            else
            {
                file_itr = strtok(NULL, "/");
            }
            file_count--;
        }
    }
    else{
        file_itr = file_path_copy;
    }

    if((send(client_fd, request, strlen(request), 0)) == -1){
        perror("Send Error:");
        exit(-1);
    }

    FILE *file_ptr;
    file_ptr = fopen(file_itr, "w");
    int recv_msg_length;
    memset(buffer,0,10000);
    if((recv_msg_length = recv(client_fd, buffer, 10000,0)) <= 0){
        perror("Receive Error:");
    }
    else if((*(buffer+9) == '4') && (*(buffer+10) == '0') && (*(buffer+11) == '4')){
        printf("%s", buffer);
        remove(file_itr);
    }
    else
    {
        char * temp = strstr(buffer, "\r\n\r\n");
        fwrite(temp+4, 1, strlen(temp)-4, file_ptr);
        memset(buffer, 0, 10000);
        while((recv_msg_length = recv(client_fd, buffer, 10000,0)) > 0){
            fwrite(buffer, 1, recv_msg_length,file_ptr);
            memset(buffer, 0, 10000);
        }
    }
    fclose(file_ptr);
    close(client_fd);
    return 0;
}
