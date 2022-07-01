#include "common.h"
char *pcDay[7]=
{
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

char *pcMonth[12]=
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

int create_socket(bool isIPv4)
{
    int socket_fd = -1;
    if (isIPv4 ==  true)
    {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    else
    {
        socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
    }
    if(socket_fd == -1)
    {
        perror("Socket Create Failed");
        exit(-1);
    }
    else{
        printf("Socket is Created Successfully....\n");
    }
    return socket_fd;
}

void set_server_address(struct sockaddr_in *server_address, char * ip, int port)
{
    bzero(server_address, sizeof(*server_address));
    (*server_address).sin_family = AF_INET;
    (*server_address).sin_addr.s_addr = inet_addr(ip);
    (*server_address).sin_port = htons(port);
}

void set_server_address_ipv6(struct sockaddr_in6 *server_address, char * ip, int port)
{
    bzero(server_address, sizeof(*server_address));
    (*server_address).sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, ip, &(*server_address).sin6_addr) <= 0)
        printf("inet_pton error for %s", ip);
    (*server_address).sin6_port = htons(port);
}

void bind_server(int socket_fd, struct sockaddr_in server_address)
{
    int val = bind(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
    if(val != 0)
    {
        perror("Socket Bind Failed");
        exit(-1);
    }
    else
    {
        printf("Socket is binded Successfully....\n");
    }
}

void bind_server_ipv6(int socket_fd, struct sockaddr_in6 server_address)
{
    int val = bind(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
    if(val != 0)
    {
        perror("Socket Bind Failed");
        exit(-1);
    }
    else
    {
        printf("Socket is binded Successfully....\n");
    }
}

void start_listening(int socket_fd)
{
    int val = listen(socket_fd, QUEUE_SIZE);
    if(val != 0)
    {
        perror("Listening Failed");
        exit(-1);
    }
    else
    {
        printf("Server Listening Started....\n");
    }
}

int accept_connection(struct sockaddr_in * client_addresses, int client_count, int socket_fd)
{
    socklen_t len = (socklen_t)sizeof(client_addresses[client_count]);
    int new_socket_fd = accept(socket_fd, (struct sockaddr *)&client_addresses[client_count], &len);
    if(new_socket_fd < 0)
    {
        perror("Accept connection error: ");
        exit(-1);
    }
    return new_socket_fd;
}

// Function for handling zombie process
void zombie_handler_func(int sigtemp_fd)
{
    wait(NULL);
}

int iFormat_Read_Request(char *pcRequest, char *pcHost, int *piPort,char *pcUrl, char *pcName)
{
    char acMethod[MAX_NAME_LENGTH];
    char acProtocol[MAX_NAME_LENGTH];
    char *pcUri;
    int  iNum_Bytes_Ret;
    iNum_Bytes_Ret = sscanf(pcRequest, "%s %s %s %s", acMethod, pcName, acProtocol, pcUrl);

    if(strcmp(acMethod, "GET")!=0)
        return -1;

    if(strcmp(acProtocol, "HTTP/1.0")!=0)
        return -1;

    pcUri = pcUrl;
    pcUri = pcUri + 5;
    strcpy (pcUrl,pcUri);
    strcpy (pcHost,pcUrl);
    strcat (pcUrl, pcName);

    *piPort = 80;
    return iNum_Bytes_Ret;
}

int check_cache_entry (char *url)
{
    int index = -1;
    int i = 0;
    for (i = 0 ; i < 10 ; i++)
    {
        if (!strcmp (sCache_table[i].acUrl, url))
        {
            index = i;
            break;
        }
    }
    return index;
}

int iCheck_Cache_Entry_Hit(char *pcUrl)
{

    int j = 0;
    for (j = 0; j < 10; j++)
    {
        if (strcmp(sCache_table[j].acUrl, pcUrl) == 0)
        {
            return 0;
        }
    }
    return -1;
}

int iMonthCoverter(char *pcMonth)
{
    int iMonthNumber;
    switch (*pcMonth)
    {
        case 'J':
            if (*(pcMonth + 1) == 'a')
            {
                iMonthNumber = 1;
            }
            else if (*(pcMonth + 2) =='n')
            {
                iMonthNumber = 6;
            }
            else
            {
                iMonthNumber = 7;
            }
            break;

        case 'F':
            iMonthNumber = 2;
            break;

        case 'M':
            if (*(pcMonth + 2) =='r')
            {
                iMonthNumber = 3;
            }
            else
            {
                iMonthNumber = 5;
            }
            break;

        case 'A':
            if (*(pcMonth + 2) == 'r')
            {
                iMonthNumber = 4;
            }
            else
            {
                iMonthNumber = 8;
            }
            break;

        case 'S':
            iMonthNumber = 9;
            break;

        case 'O':
            iMonthNumber = 10;
            break;

        case 'N':
            iMonthNumber = 11;
            break;

        case 'D':
            iMonthNumber = 12;
            break;

        default:
            break;
    }
    return iMonthNumber;
}

int iCheck_Cache_Entry_Expire(char *pcUrl,struct tm *timenow)
{
    int i = 0, iRet;
    char acUpdatedTime[MAX_TIME_LENGTH];
    for (i = 0; i < 10; i++)
    {
        if (strcmp(sCache_table[i].acUrl, pcUrl)==0)
        {
            break;
        }
    }
    memset(acUpdatedTime, 0, MAX_TIME_LENGTH);

    sprintf(acUpdatedTime, "%s, %2d %s %4d %2d:%2d:%2d GMT", pcDay[timenow->tm_wday],timenow->tm_mday, pcMonth[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);

    iRet = iTime_Comparison_Func(sCache_table[i].acExpiry, acUpdatedTime);
    return (iRet < 0) ? -1 : 0;
}

int iTime_Comparison_Func(char *pcOldTime, char *pcNewTime)
{
    int old_year, old_month, old_hour, old_minute, old_second,old_day;
    int new_year, new_month, new_hour, new_minute, new_second,new_day;

    char acOld_month[4];
    char acNew_month[4];

    memset(acOld_month, 0, 4);
    memset(acNew_month, 0, 4);

    sscanf(pcOldTime + 5, "%d %3s %d %d:%d:%d ",&old_day,acOld_month,&old_year,&old_hour,&old_minute,&old_second);
    sscanf(pcNewTime + 5, "%d %3s %d %d:%d:%d ",&new_day,acNew_month,&new_year,&new_hour,&new_minute,&new_second);
    old_month = iMonthCoverter(acOld_month);
    new_month = iMonthCoverter(acNew_month);

    if (old_year < new_year) return -1;
    if (old_year > new_year) return 1;
    if (old_month < new_month) return -1;
    if (old_month > new_month) return 1;
    if (old_day < new_day) return -1;
    if (old_day > new_day) return 1;
    if (old_hour < new_hour) return -1;
    if (old_hour > new_hour) return 1;
    if (old_minute < new_minute) return -1;
    if (old_minute > new_minute) return 1;
    if (old_second < new_second) return -1;
    if (old_second > new_second) return 1;
    return 0;
}

void vSend_Error_Message(int iStatus, int iSocket_fd)
{
    char acErr_Msg[1024];
    static char* acBad_Request =
        "****************************************************************"
        "HTTP/1.0 400 Bad Request"
        "****************************************************************";

    static char* acNot_Found =
        "****************************************************************"
        "HTTP/1.0 404 Not Found"
        "****************************************************************";

    memset(acErr_Msg, 0, 1024);
    switch (iStatus)
    {
        case 400:
            sprintf(acErr_Msg, "%s", acBad_Request);
            send(iSocket_fd, acErr_Msg, strlen(acErr_Msg), 0);
            break;

        case 404:
            sprintf(acErr_Msg, "%s", acNot_Found);
            send(iSocket_fd, acErr_Msg, strlen(acErr_Msg), 0);
            break;

        default:
            break;
    }
}


