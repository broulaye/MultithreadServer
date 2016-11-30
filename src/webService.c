int start_server(short *port) {
       int socket, client_fd, client_size;
       char buffer[256];
       struct sockaddr_in server_addr, client_addr;
       int  success;

       /* First call to socket() function */
       socket = socket(AF_INET, SOCK_STREAM, 0);

       if (socket < 0) {
          perror("ERROR opening socket");
          exit(1);
       }

       /* Initialize socket structure */
       bzero((char *) &server_addr, sizeof(server_addr));

       server_addr.sin_family = AF_INET;
       server_addr.sin_addr.s_addr = htlon(INADDR_ANY);
       server_addr.sin_port = htons(*port);

       /* Now bind the host address using bind() call.*/
       if (bind(socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
          perror("ERROR binding");
          exit(1);
       }

       /* Now start listening for the clients, here process will
          * go in sleep mode and will wait for the incoming connection
       */

       if(listen(socket,5) < 0) {
            perror("ERROR listening");
            exit(1);
       }
       client_size = sizeof(client_addr);

       /* Accept actual connection from the client and return client file descriptor if successful*/
       client_fd = accept(socket, (struct sockaddr *)&client_addr, &client_size);

       if (client_fd < 0) {
          perror("ERROR accept");
          exit(1);
       }

       /* If connection is established then start communicating */
       bzero(buffer,256);//set buffer values to null
       success = read( client_fd,buffer,255 );//read 255 bytes char from client into buffer, return positive number if successful

       if (success < 0) {
          perror("ERROR reading from socket");
          exit(1);
       }

       printf("Here is the message: %s\n",buffer);

       /* Write a response to the client */
       success = write(client_fd,"I got your message",18);//write a response to client message using client file descriptor

       if (success < 0) {
          perror("ERROR writing to socket");
          exit(1);
       }

       close(socket);
       close(client_fd);
       return 0;
}

static void usage(char *av0) {
    fprintf(stderr, "Usage: %s [-p <n>] [-R <s>]"
                    " -p        port number \n"
                    " -R        root path\n"
                    , av0);
    abort();
}

int main(int ac, char** av)
{
    unsigned short port;
    char *path
    int c;
    while ((c = getopt(ac, av, "p:R:")) != EOF) {
        switch (c) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'R':
            path = optarg;
            break;

        }
    }
    if (optind == ac)
        usage(av[0]);
    start_server(*port);

    return(0);
}

