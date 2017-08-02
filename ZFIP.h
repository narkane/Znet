#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>    //close
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#include "packet/packet.h"

#define TRUE   1
#define FALSE  0
#define PORT 8888
#define VERSION 1

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

////////////////////////////////////////////////////////////////////////////
// CLIENT CLASS
////////////////////////////////////////////////////////////////////////////
class client
{
public:
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
/*    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
   }*/
   client(int p, char* host)
   {
        portno = p;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("ERROR opening socket");
        server = gethostbyname(host);
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
             (char *)&serv_addr.sin_addr.s_addr,
             server->h_length);
             serv_addr.sin_port = htons(portno);
    }

    void conn()
    {
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
            error("ERROR connecting");
    }

    void readWrite()
    {
        printf("Please enter the message: ");
        bzero(buffer,256);
        //fgets(buffer,255,stdin);
        packet rPatch;
        rPatch.packetTypeID = PATCH_REQUEST_PACKET_TYPE;
        rPatch.serializePacket(*buffer);
        printf("buffer: %s\n",buffer);
        /////////////////////////IN PROGRESS
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0)
             error("ERROR writing to socket");
        bzero(buffer,256);
        n = read(sockfd,buffer,255);
        if (n < 0)
             error("ERROR reading from socket");
    }

    void closeConn()
    {
        close(sockfd);
    }
    //return 0;
};


////////////////////////////////////////////////////////////////////////////
// SERVER CLASS
////////////////////////////////////////////////////////////////////////////
class server
{
    int opt = TRUE;
    int master_socket , addrlen , new_socket , client_socket[30] ,
          max_clients = 30 , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;

    char buffer[1025];  //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    //a message
    char* message = "ECHO Daemon v1.0 \r\n";
    char* charPacketTypeID;
    uint16_t packetTypeID;
public:
    //////////////////
    // Initialise Server
    server()
    {
        //initialise all client_socket[] to 0 so not checked
        for (i = 0; i < max_clients; i++)
        {
            client_socket[i] = 0;
        }

        //create a master socket
        if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        //set master socket to allow multiple connections ,
        //this is just a good habit, it will work without this
        if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
              sizeof(opt)) < 0 )
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        //type of socket created
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons( PORT );

        //bind the socket to localhost port 8888
        if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        printf("Listener on port %d \n", PORT);

        //try to specify maximum of 3 pending connections for the master socket
        if (listen(master_socket, 3) < 0)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        addrlen = sizeof(address);
    }

    //accept the incoming connection
    void acceptConn()
    {
        puts("Waiting for connections ...");

        //while(TRUE)
        //{
            //clear the socket set
            FD_ZERO(&readfds);

            //add master socket to set
            FD_SET(master_socket, &readfds);
            max_sd = master_socket;

            //add child sockets to set
            for ( i = 0 ; i < max_clients ; i++)
            {
                //socket descriptor
                sd = client_socket[i];

                //if valid socket descriptor then add to read list
                if(sd > 0)
                    FD_SET( sd , &readfds);

                //highest file descriptor number, need it for the select function
                if(sd > max_sd)
                    max_sd = sd;
            }

            //wait for an activity on one of the sockets , timeout is NULL ,
            //so wait indefinitely
            activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

            if ((activity < 0) && (errno!=EINTR))
            {
                printf("select error");
            }

            //If something happened on the master socket ,
            //then its an incoming connection
            if (FD_ISSET(master_socket, &readfds))
            {
                if ((new_socket = accept(master_socket,
                        (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }

                //inform user of socket number - used in send and receive commands
                printf("New connection , socket fd is %d , ip is : %s , port : %d \n",
                    new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                //send new connection greeting message
                if( send(new_socket, message, strlen(message), 0) != strlen(message) )
                {
                    perror("send");
                }

                puts("Welcome message sent successfully");

                //add new socket to array of sockets
                for (i = 0; i < max_clients; i++)
                {
                    //if position is empty
                    if( client_socket[i] == 0 )
                    {
                        client_socket[i] = new_socket;
                        printf("Adding to list of sockets as %d\n" , i);

                        break;
                    }
                }
            }

        //}
    }

    void awaitMsg()
    {
        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET( sd , &readfds))
            {
                //Check if it was for closing , and also read the
                //incoming message
                if ((valread = read( sd , buffer, 1024)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" ,
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                        //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }
                ///////////////////
                // MESSAGE HANDLER SWITCH LOGIC
                else
                {
                    memcpy(&packetTypeID, &buffer, 2);
                    packetTypeID = ntohs(packetTypeID);
                    switch(packetTypeID)
                    {
                        case PATCH_REQUEST_PACKET_TYPE :
                            send(sd , "PATCH_REQUEST Approved!\0" , strlen("PATCH_REQUEST Approved!\0") , 0 );

                        default:
                            //set the string terminating NULL byte on the end
                            //of the data read
                            buffer[valread] = '\0';
                            send(sd , buffer , strlen(buffer) , 0 );
                            break;
                    }
                }
            }
        }
    }
};

//stuff needed packet in buffer for sending
void msgHandler()
{
}
