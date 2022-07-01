#include "common.h"

int main(int argc, char *argv[])
{
    char acRequest[1024];
    time_t now;
    struct tm *timenow;
    struct addrinfo hints, *result;
    int iSendLen;
    FILE *fp;
    int recvlen = -1;
    int iListener_fd,client_fd;
    struct sockaddr_storage remoteaddr;
    struct sockaddr_in addr;
    socklen_t sin_size;
    fd_set set1;
    fd_set set2;
    int max_fd;
    int socket_itr;
    char acBuffer[BUFFER_SIZE], acHost[MAX_NAME_LENGTH], acUrl[MAX_NAME_LENGTH], acName[MAX_NAME_LENGTH];
    int cacheindex = -1;
    int iPort = HTTP_PORT_NUMBER;
    int porxy_fd;
    int i, iOldest_Entry;
    FILE *readfp;
    char* expires=NULL;
    char *pcToken = NULL;
    int readlen = 0;
    char modified[100];
    char modified_request[BUFFER_SIZE];

    memset(sCache_table, 0, 10 * sizeof(sCache));
    if (argc<3)
    {
        printf("\nUsage: server host port\nExiting\n");
        exit(-1);
    }
    memset(&remoteaddr, 0, sizeof remoteaddr);
    memset(&addr, 0, sizeof addr);

    iPort = atoi(argv[2]);
    addr.sin_port = htons(iPort);
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_family = AF_INET;

    if((iListener_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Cannot create socket");
        exit(-1);
    }
    if(bind(iListener_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0)
    {
        perror("Cannot bind socket");
        exit(-1);
    }
    if(listen(iListener_fd, 10) < 0)
    {
        perror("listen");
        exit(-1);
    }

    FD_SET(iListener_fd, &set1);
    max_fd = iListener_fd;

    memset(sCache_table, 0, 10 * sizeof(sCache));

    printf("Waiting for request!\n");
    sin_size = sizeof(remoteaddr);

    while(1)
    {
        set2 = set1;
        if(select(max_fd+1, &set2, NULL, NULL, NULL) == -1)
        {
            perror("Select Error: ");
            exit(-1);
        }
        for(socket_itr=0; socket_itr <=max_fd; socket_itr++)
        {
            if(FD_ISSET(socket_itr, &set2))
            {
                if(socket_itr == iListener_fd)
                {
                    client_fd = accept(iListener_fd, (struct sockaddr *)&remoteaddr, &sin_size);
                    max_fd = client_fd > max_fd ? client_fd : max_fd;
                    FD_SET(client_fd, &set1);
                    if (client_fd == -1)
                    {
                        perror("accept:");
                        continue;
                    }
                }
                else
                {
                    memset(acBuffer, 0, BUFFER_SIZE);
                    memset(acHost, 0, MAX_NAME_LENGTH);
                    memset(acUrl, 0, MAX_NAME_LENGTH);
                    memset(acName, 0, MAX_NAME_LENGTH);
                    client_fd = socket_itr;
                    if(recv(client_fd, acBuffer, sizeof(acBuffer), 0) < 0)
                    {
                        perror("recv");
                        close(client_fd);
                        return 1;
                    }
                    printf ("The client sent messages is\n%s \n", acBuffer);
                    if (iFormat_Read_Request(acBuffer, acHost, &iPort, acUrl, acName) != 4)
                    {
                        vSend_Error_Message(400,client_fd);
                        close(client_fd);
                        return 1;
                    }
                    if((porxy_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        perror("proxysocket create");
                        close(client_fd);
                        return 1;
                    }

                    memset(&hints, 0, sizeof(hints));
                    hints.ai_family = AF_INET;
                    hints.ai_socktype = SOCK_STREAM;

                    if(getaddrinfo(acHost, "80", &hints, &result)!=0)
                    {
                        printf("getaddrinfo error\n");
                        vSend_Error_Message(404, client_fd);
                        close(client_fd);
                        return 1;
                    }

                    if (connect(porxy_fd, result->ai_addr, result->ai_addrlen) < 0)
                    {
                        close(porxy_fd);
                        perror("connect error:");
                        vSend_Error_Message(404, client_fd);
                        close(client_fd);
                        return 1;
                    }

                    time(&now);
                    timenow = gmtime(&now);

                    cacheindex = check_cache_entry (acUrl);
                    if (cacheindex == -1)
                    {
                        // New entry in cache. Send request to HTTP server
                        memset(acRequest, 0, 1024);
                        printf("File not found in cache..\nDownloading from %s:%d\n",acHost,iPort);
                        printf ("\nSending HTTP query to web server\n");
                        sprintf(acRequest, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", acName,acHost);
                        puts(acRequest);
                        if((iSendLen = send(porxy_fd, acRequest, strlen(acRequest), 0)) < 0)
                        {
                            perror("send acRequest:");
                            close(porxy_fd);
                            return 1;
                        }
                        printf("Request sent to server.\n");
                        memset(acBuffer, 0, BUFFER_SIZE);
                        for (i = 0; i < 10; i++)
                        {
                            if(sCache_table[i].iIs_filled == 0)
                            {
                                iOldest_Entry = i;
                                break;
                            }
                            else
                            {
                                if (iTime_Comparison_Func(sCache_table[i].acLast_Modified_Time,sCache_table[iOldest_Entry].acLast_Modified_Time) <= 0)
                                {
                                    iOldest_Entry = i;
                                }
                            }
                        }

                        memset(&sCache_table[iOldest_Entry], 0, sizeof(sCache));
                        sCache_table[iOldest_Entry].iIs_filled = 1;

                        pcToken = strtok (acName, "/");

                        while (pcToken != NULL)
                        {
                            strcpy(sCache_table[iOldest_Entry].acFilename, pcToken);
                            pcToken = strtok(NULL, "/");
                        }

                        memcpy(sCache_table[iOldest_Entry].acUrl, acUrl, MAX_NAME_LENGTH);
                        sprintf(sCache_table[iOldest_Entry].acLast_Modified_Time, "%s, %02d %s %d %02d:%02d:%02d GMT", pcDay[timenow->tm_wday],timenow->tm_mday, pcMonth[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);

                        remove(sCache_table[iOldest_Entry].acFilename);
                        fp=fopen(sCache_table[iOldest_Entry].acFilename, "w");
                        if (fp==NULL)
                        {
                            printf("failed to create cache.\n");
                            return 1;
                        }
                        while ((recvlen=recv(porxy_fd, acBuffer, BUFFER_SIZE, 0)) > 0)
                        {
                            if(send(client_fd, acBuffer, recvlen, 0) < 0)
                            {
                                perror("Client send:");
                                return 1;
                            }
                            fwrite(acBuffer, 1, recvlen, fp);
                            memset(acBuffer, 0, BUFFER_SIZE);
                        }
                        send(client_fd, acBuffer, 0, 0);
                        printf("Received successfull response from server.\n");
                        printf("Sent file to the client \n*******************************************************\n");
                        fclose(fp);
                        readfp=fopen(sCache_table[iOldest_Entry].acFilename,"r");
                        fread(acBuffer, 1, 2048, readfp);
                        fclose(readfp);

                        expires=strstr(acBuffer, "Expires: ");
                        if (expires!=NULL)
                        {
                            memcpy(sCache_table[iOldest_Entry].acExpiry, expires + 9, 29);
                        }
                        else
                        {
                            sprintf(sCache_table[iOldest_Entry].acExpiry, "%s, %02d %s %4d %02d:%02d:%02d GMT", pcDay[timenow->tm_wday],timenow->tm_mday, pcMonth[timenow->tm_mon], timenow->tm_year+1900,(timenow->tm_hour),(timenow->tm_min)+2,timenow->tm_sec);
                        }
                    }
                    else
                    {
                        if(iCheck_Cache_Entry_Expire(acUrl,timenow)>=0)
                        {
                            printf("File found in cache and its not expired.\nSending file to the client...\n");
                            sprintf(sCache_table[cacheindex].acLast_Modified_Time, "%s, %02d %s %4d %02d:%02d:%02d GMT", pcDay[timenow->tm_wday],timenow->tm_mday, pcMonth[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
                            readfp=fopen(sCache_table[cacheindex].acFilename,"r");
                            memset(acBuffer, 0, BUFFER_SIZE);
                            while ((readlen=fread(acBuffer, 1, BUFFER_SIZE, readfp)) > 0)
                            {
                                send(client_fd, acBuffer, readlen, 0);
                            }
                            printf("Sent file to client successfully.\n*******************************************************\n");
                            fclose(readfp);
                        }
                        else
                        {
                            printf("File has expired, Requesting updated file from server.\n");
                            memset(modified, 0, 100);
                            sprintf(modified, "If-Modified-Since: %s\r\n\r\n",sCache_table[cacheindex].acLast_Modified_Time);
                            memset(modified_request, 0, BUFFER_SIZE);
                            memcpy(modified_request, acBuffer, strlen(acBuffer)-2);

                            strcat(modified_request, modified);
                            printf ("Sending HTTP query to web server\n");
                            printf("%s\n",modified_request);
                            send(porxy_fd, modified_request, strlen(modified_request), 0);
                            memset(acBuffer, 0, BUFFER_SIZE);
                            recvlen = recv(porxy_fd, acBuffer, BUFFER_SIZE, 0);
                            expires=strstr(acBuffer, "Expires: ");
                            if (expires!=NULL)
                            {
                                memcpy(sCache_table[cacheindex].acExpiry, expires+9, 29);
                            }
                            else
                            {
                                sprintf(sCache_table[cacheindex].acExpiry, "%s, %02d %s %4d %02d:%02d:%02d GMT", pcDay[timenow->tm_wday],timenow->tm_mday, pcMonth[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
                            }

                            if (recvlen > 0)
                            {
                                printf("Printing HTTP response \n %s\n",acBuffer);
                                if ((*(acBuffer + 9) == '3') && (*(acBuffer + 10) == '0') && (*(acBuffer + 11) == '4'))
                                {
                                    printf("File is still up to date. Sending file in cache.\n*******************************************************\n");

                                    readfp = fopen(sCache_table[cacheindex].acFilename,"r");
                                    memset(acBuffer, 0, BUFFER_SIZE);
                                    while ((readlen=fread(acBuffer, 1, BUFFER_SIZE, readfp))>0)
                                    {
                                        send(client_fd, acBuffer, readlen, 0);
                                    }
                                    fclose(readfp);

                                }
                                else if((*(acBuffer + 9) == '4') && (*(acBuffer + 10) == '0') && (*(acBuffer + 11) == '4'))
                                {
                                    vSend_Error_Message( 404, client_fd);
                                }
                                else if((*(acBuffer + 9) == '2') && (*(acBuffer + 10) == '0') && (*(acBuffer + 11) == '0'))
                                {
                                    printf("New file received from server.\nUpdating cache and sending file to client.\n*******************************************************\n");
                                    send(client_fd, acBuffer, recvlen, 0);
                                    remove(sCache_table[cacheindex].acFilename);

                                    expires = NULL;

                                    expires = strstr(acBuffer, "Expires: ");
                                    if (expires != NULL)
                                    {
                                        memcpy(sCache_table[cacheindex].acExpiry, expires+9, 29);
                                    }
                                    else
                                    {
                                        sprintf(sCache_table[cacheindex].acExpiry, "%s, %02d %s %4d %02d:%02d:%02d GMT", pcDay[timenow->tm_wday],timenow->tm_mday, pcMonth[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
                                    }

                                    sprintf(sCache_table[cacheindex].acLast_Modified_Time, "%s, %02d %s %4d %02d:%02d:%02d GMT", pcDay[timenow->tm_wday],timenow->tm_mday, pcMonth[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
                                    fp=fopen(sCache_table[cacheindex].acFilename, "w");
                                    fwrite(acBuffer, 1, recvlen, fp);

                                    memset(acBuffer, 0, BUFFER_SIZE);
                                    while ((recvlen = recv(porxy_fd, acBuffer, BUFFER_SIZE, 0)) > 0)
                                    {
                                        send(client_fd, acBuffer, recvlen, 0);
                                        fwrite(acBuffer, 1, recvlen, fp);
                                    }
                                    fclose(fp);

                                }
                            }
                            else
                                perror("receive:");


                        }
                    }
                    FD_CLR(client_fd,&set1);
                    close(client_fd);
                    close(porxy_fd);
                    printf ("               Printing Cache Table\n");
                    for (i = 0 ; i < 10 ; i++)
                    {
                        if (sCache_table[i].iIs_filled)
                        {
                            printf ("*******************************************************\n");
                            printf ("               Cache Entry Number %d \n", i + 1);
                            printf ("URL                : %s\n", sCache_table[i].acUrl);
                            printf ("Last Access Time   : %s\n", sCache_table[i].acLast_Modified_Time);
                            printf ("Expiry             : %s\n", sCache_table[i].acExpiry);
                            printf ("File Name          : %s\n", sCache_table[i].acFilename);
                            printf ("*******************************************************\n\n");
                        }
                    }
                }
            }
        }
    }
}
