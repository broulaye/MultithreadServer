#include "csapp.h"
#include "list.h"
#include <arpa/inet.h>

int start_server(short*);
void process_request(int);
void usage(char*);
void read_requesthdrs(rio_t *);
int parse_uri(char *, char *, char *);
void serve_static(int,char *,int);
void get_filetype(char *, char *);
void serve_dynamic(int, char *, char *);
void clienterror(int, char *, char *,char *, char *);


void process_request(int client_fd) {
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /** Read request line and headers */
    rio_readinitb(&rio, client_fd);
    rio_readlineb(&rio, buf, MAXLINE);

    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(client_fd, method, "501", "Not Implemented",
            "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);


    /** Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(client_fd, filename, "404", "Not found",
            "Tiny couldn’t find this file");
        return;
    }

    if (is_static) { /* Serve static content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {/**Verify that file is regular and we have read permission*/
            clienterror(client_fd, filename, "403", "Forbidden",
                "Tiny couldn’t read the file");
            return;
        }
        serve_static(client_fd, filename, sbuf.st_size);
    }
    else { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {/**Verify that file is executable and we have read permission*/
            clienterror(client_fd, filename, "403", "Forbidden",
                "Tiny couldn’t run the CGI program");
            return;
        }
        serve_dynamic(client_fd, filename, cgiargs);
    }


//    while((n = rio_readlineb(&rio, buf, MAXLINE)) != 0) {
//        printf("server received %d bytes\n", n);
//        rio_writen(client_fd, buf, n);
//    }
//    char buffer[256];
//    int  success;
//   /* If connection is established then start communicating */
//   bzero(buffer,256);//set buffer values to null
//   success = read( client_fd,buffer,255 );//read 255 bytes char from client into buffer, return positive number if successful
//
//   if (success < 0) {
//      perror("ERROR reading from socket");
//      exit(1);
//   }
//
//   printf("Here is the message: %s\n",buffer);
//
//   /* Write a response to the client */
//   success = write(client_fd,"I got your message",18);//write a response to client message using client file descriptor
//
//   if (success < 0) {
//      perror("ERROR writing to socket");
//      exit(1);
//   }
//
//   close(client_fd);

}



void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}


void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}


int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) { /* Static content */
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return 1;
    }
    else { /* Dynamic content */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        }
        else
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}



void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    rio_writen(fd, buf, strlen(buf));

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    rio_writen(fd, srcp, filesize);
    munmap(srcp, filesize);
}

/*
*get_filetype - derive file type from file name
*/
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    rio_writen(fd, buf, strlen(buf));

    if (fork() == 0) { /* child */
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);
        dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */
        execve(filename, emptylist, environ); /* Run CGI program */
    }
    wait(NULL); /* Parent waits for and reaps child */
}

int start_server(short *port) {
   int socket, optval=1;
   struct sockaddr_in server_addr;


   /* First call to socket() function */
   socket = Socket(AF_INET, SOCK_STREAM, 0);

   if (socket < 0) {
      perror("ERROR opening socket");
      exit(1);
   }


    /* Eliminates "Address already in use" error from bind */
     if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
     (const void *)&optval , sizeof(int)) < 0)
        return -1;


   /* Initialize socket structure */
   bzero((char *) &server_addr, sizeof(server_addr));

   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   server_addr.sin_port = htons(port);

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

   return socket;
}

void usage(char *av0) {
    fprintf(stderr, "Usage: %s [-p <n>] [-R <s>]"
                    " -p        port number \n"
                    " -R        root path\n"
                    , av0);
    abort();
}

int main(int ac, char** av)
{
    short port;
    struct sockaddr client_addr;
    int client_fd, socket;
    socklen_t client_size;
    char *path;
    int c;
    while ((c = getopt(ac, av, "p:R:")) != EOF) {
        switch (c) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'R':
            path = optarg;
        }
    }

    if(path == NULL);

    if (optind == ac)
        usage(av[0]);
    socket = start_server(&port);
    while(1) {
        client_size = sizeof(client_addr);

        /* Accept actual connection from the client and return client file descriptor if successful*/
        client_fd = accept(socket, (struct sockaddr *)&client_addr, &client_size);

        if (client_fd < 0) {
           perror("ERROR accept");
           exit(1);
        }

        process_request(client_fd);
        close(client_fd);
    }

    return(0);
}

