#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//int main(int argc, char* argv[])
class server
{
public:
    int sockfd;
    int newsockfd;
    int portno;
    socklen_t clilen;
    int n;

    char buffer[256];

    struct sockaddr_in serv_addr, cli_addr;

/*    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided");
        exit(1);
    }*/

    server(int port)
    {
        // AF_INET / AF_UNIX : IP domain / Filesystem domain
        // SOCK_STREAM / SOCK_DGRAM
        // 0 sets STREAM to TCP and DGRAM to UDP
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            fprintf(stderr, "ERROR, opening socket");

        // Initializes serv_addr to 0
        bzero((char *) &serv_addr, sizeof(serv_addr));

        portno = port;

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(portno);
        serv_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
             fprintf(stderr, "ERROR on binding");
    }

    void acceptConn()
    {
        listen(sockfd,5);

        clilen = sizeof(cli_addr);

        // Accept client connection
        newsockfd = accept(sockfd,
                    (struct sockaddr *) &cli_addr,
                    &clilen);

        if (newsockfd < 0)
            fprintf(stderr, "ERROR on accept");
    }

    void readWrite()
    {
        bzero(buffer,256);

        n = read(newsockfd,buffer,255);

        if (n < 0) fprintf(stderr, "ERROR reading from socket");

        printf("Here is the message: %s\n",buffer);
        n = write(newsockfd,"I got your message",18);

        if (n < 0) fprintf(stderr, "ERROR writing to socket");
    }

    void closeConn()
    {
        close(newsockfd);
        close(sockfd);
    }
    //return 0;
};
