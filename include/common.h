#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

#define HTTP_PORT_NUMBER 80


#define QUEUE_SIZE 50
#define BUFFER_SIZE 10000
#define FILE_SIZE 512

#define MAX_RETRIES 10
#define MAX_CACHE_ENTRIES 10
#define MAX_NAME_LENGTH 256
#define MAX_TIME_LENGTH 50
#define MAX_FILE_NAME_LENGTH 10

// cache structure definition
typedef struct
{
    char acUrl[MAX_NAME_LENGTH];
    char acLast_Modified_Time[MAX_TIME_LENGTH];
    char acExpiry[MAX_TIME_LENGTH];
    char acFilename[MAX_FILE_NAME_LENGTH];
    int  iIs_filled;
}sCache;

sCache sCache_table[MAX_CACHE_ENTRIES];
// day structure definition
extern char *pcDay[7];

// month structure definition
extern char *pcMonth[12];

pthread_mutex_t mFile_Lock[MAX_CACHE_ENTRIES];


int create_socket(bool isIPv4);
void set_server_address(struct sockaddr_in *server_address, char * ip, int port);
void set_server_address_ipv6(struct sockaddr_in6 *server_address, char * ip, int port);
void bind_server(int socket_fd, struct sockaddr_in server_address);
void bind_server_ipv6(int socket_fd, struct sockaddr_in6 server_address);
void start_listening(int socket_fd);
int accept_connection(struct sockaddr_in * client_addresses, int client_count, int socket_fd);
int get_mode (char *mode_checker);
void zombie_handler_func(int signum);
int iFormat_Read_Request(char *pcRequest, char *pcHost, int *piPort,char *pcUrl, char *pcName);
int iCheck_Cache_Entry_Hit(char *pcUrl);
int iMonthCoverter(char *pcMonth);
int iCheck_Cache_Entry_Expire(char *pcUrl,struct tm *timenow);
int iTime_Comparison_Func(char *pcOldTime, char *pcNewTime);
void vSend_Error_Message(int iStatus, int iSocket_fd);
int iHandle_Client_Message (int client_fd);
int check_cache_entry (char *url);
#endif // COMMON_INCLUDED
