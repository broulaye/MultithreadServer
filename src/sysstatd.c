#include <error.h>
#include <sys/poll.h>
#include "csapp.h"
#include "loadavg.h"
#include "meminfo.h"

#define DEFAULT_PORT "11900"
typedef int bool;
#define true 1
#define false 0

void process_request(int);
//void usage(char*);
void read_requesthdrs(rio_t *);
void parse_uri(char *, char *, char *);
void serve_static(int,char *, int, char *);
void get_filetype(char *, char *);
void serve_dynamic(int, char *, char *);
void clienterror(int, char *, char *,char *, char *);
void serve_header(int, char *, int);
static bool isCallBack;
static char version[MAXLINE];
void process_request(int client_fd) {
    //int is_static;
    struct stat sbuf;
    struct stat sbuf2;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;
    isCallBack = false;
    /** Read request line and headers */
    Rio_readinitb(&rio, client_fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;
    printf("%s\n", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET")) {
        clienterror(client_fd, method, "501", "Not Implemented",
            "Tiny does not implement this method");
        return;
    }

    read_requesthdrs(&rio);


    /** Parse URI from GET request */
    parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(client_fd, filename, "404", "Not found",
            "Tiny couldn’t find this file");
        return;
    }
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {/**Verify that file is executable and we have read permission*/
            clienterror(client_fd, filename, "403", "Forbidden",
                "Tiny couldn’t run the CGI program");
            return;
    }

    //serve_header(client_fd, filename, sbuf.st_size);


    if (!isCallBack) { /* Not CallBack */
        //serve_dynamic(client_fd, filename, cgiargs);
        printf("%s\n", filename);
	if(strstr(filename, "/proc/loadavg")) {
	    getLoadAvg();
            if (stat("loadavg.txt", &sbuf2) < 0) {
                clienterror(client_fd, filename, "404", "Not found",
                    "Tiny couldn’t find this file");
                return;
            }
	    //serve_header(client_fd, filename, sbuf.st_size);
            serve_static(client_fd, "loadavg.txt", sbuf2.st_size, cgiargs);

	}
	else if(strstr(filename, "/proc/meminfo")) {
	    getMemInfo();
            if (stat("meminfo.txt", &sbuf2) < 0) {
                clienterror(client_fd, filename, "404", "Not found",
                    "Tiny couldn’t find this file");
                return;
            }
	    //serve_header(client_fd, filename, sbuf.st_size);
            serve_static(client_fd, "meminfo.txt", sbuf2.st_size, cgiargs);
	}
	else {
	    /**Not implementing other files yet*/
	}

    }
    else {/* CallBack */
	printf("%s\n", filename);
        if(strstr(filename, "/proc/loadavg")) {
            getLoadAvg();
            if (stat("loadavg.txt", &sbuf2) < 0) {
                clienterror(client_fd, filename, "404", "Not found",
                    "Tiny couldn’t find this file");
                return;
            }

            serve_static(client_fd, "loadavg.txt", sbuf2.st_size, cgiargs);

        }
        else if(strstr(filename, "/proc/meminfo")) {
            getMemInfo();
            if (stat("meminfo.txt", &sbuf2) < 0) {
                clienterror(client_fd, filename, "404", "Not found",
                    "Tiny couldn’t find this file");
                return;
            }
            serve_static(client_fd, "meminfo.txt", sbuf2.st_size, cgiargs);
        }
        else {
            /**Not implementing other files yet*/
        }

    }
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


void parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    char *ptr2;
    char *f1 = strstr(uri, "/loadavg");
    char *f2 = strstr(uri, "/meminfo");
    if (f1 || f2) { /* Dynamic content */
	printf("got the loadavg string\n");
        ptr = index(uri, '?');
        if (ptr) {
	    ptr2 = strstr(uri, "callback");
	    if(ptr2) {
		ptr = index(ptr2, '=');
		if(ptr) {
		    ptr = index(ptr2, '&');
		    if(ptr) {
			*ptr = '\0';
		    }
		    ptr = index(ptr2, '=');
		    strcpy(cgiargs, ptr+1);
		    *ptr = '\0';
		    isCallBack = true;
		}
		else {
		    /**ERROR!!!*/
		}
	    }
	    else {
		/**Maybe error*/
	    }
        }
        else {
	    isCallBack = false;
	    strcpy(cgiargs, "");
	}
            //strcpy(cgiargs, "");
        strcpy(filename, "/proc");
        //strcat(filename, uri);
        if(f1) {
	    strcat(filename, "/loadavg");
	}
	else {
	    strcat(filename, "/meminfo");
	}
        //return 0;
    }
}



void serve_static(int fd, char *filename, int filesize, char *cgiargs)
{
    int srcfd;
    char *srcp;
    char buf2[1] = ")";
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    int originalSize = filesize;
    if(isCallBack) {
	filesize = filesize + 2 + strlen(cgiargs);
    }
    serve_header(fd, filename, filesize);
    if(isCallBack) {
	 strcat(cgiargs, "(");
	 Rio_writen(fd, cgiargs, strlen(cgiargs));
    }
    Rio_writen(fd, srcp, originalSize);
    Munmap(srcp, originalSize);
    if(isCallBack) {
	Rio_writen(fd, buf2, strlen(buf2));
    }
}

void serve_header(int fd, char *filename, int filesize) {
    char filetype[MAXLINE], buf[MAXBUF];
    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "%s 200 OK\r\n", version);
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
}

/*
 * *get_filetype - derive file type from file name
 * */
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



/** Much of this method is heavily based on the main method for a server found on
 * www.akkadia.org/dreppper/userapi-ipv6.html
 * which was linked in the project spec */
int main(int ac, char **av) {
	char *port = DEFAULT_PORT;
	char *path;

	int c;
	while ((c = getopt(ac, av, "p:R:")) != EOF) {
		switch (c) {
		case 'p':
			port = optarg;
			break;
		case 'R':
			path = optarg;
		}
	}
    if(path == NULL);
	struct addrinfo *ai;
	struct addrinfo hints;
	memset(&hints, '\0', sizeof(hints));
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
	hints.ai_socktype = SOCK_STREAM;

	int e = getaddrinfo(NULL, port, &hints, &ai);
	if (e != 0)
		error(EXIT_FAILURE, 0, "getaddrinf0: %s", gai_strerror(e));


    printf("%s\n", port);
	struct addrinfo *runp = ai;
	int nfds = 0;
	while (runp != NULL) {
		nfds++;
		runp = runp->ai_next;
	}
	struct pollfd fds[nfds];
	for (nfds = 0, runp = ai; runp != NULL; runp = runp->ai_next) {
		fds[nfds].fd = socket(runp->ai_family, runp->ai_socktype, runp->ai_protocol);
		if (fds[nfds].fd == -1)
			error(EXIT_FAILURE, errno, "socket");

		fds[nfds].events = POLLIN;
		int opt = 1;
		setsockopt(fds[nfds].fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (bind(fds[nfds].fd, runp->ai_addr, runp->ai_addrlen) != 0) {
			if (errno != EADDRINUSE)
				error(EXIT_FAILURE, errno, "bind");

			close(fds[nfds].fd);
		} else {
			if (listen(fds[nfds].fd, SOMAXCONN) != 0)
				error(EXIT_FAILURE, errno, "listen");
			nfds++;
		}
	}
	freeaddrinfo(ai);

	while (1) {
		int n = poll(fds, nfds, -1);
		if (n > 0) {
			for (int i = 0; i < n; i++) {
				if (fds[i].revents & POLLIN) {
					struct sockaddr_storage rem;
					socklen_t remlen = sizeof(rem);
					int fd = accept(fds[i].fd, (struct sockaddr *)&rem, &remlen);
					if (fd != -1) {
						char buf1[200];
						if (getnameinfo((struct sockaddr *)&rem, remlen, buf1, sizeof(buf1), NULL, 0, 0) != 0)
							strcpy(buf1, "???");
						char buf2[100];
						(void) getnameinfo((struct sockaddr *)&rem, remlen, buf2, sizeof(buf2), NULL, 0, NI_NUMERICHOST);
						printf("connection from %s (%s)\n)", buf1, buf2);
						//char buf[1024];
						//ssize_t l = rio_readn(fd, buf, sizeof(buf));
						/* Parse and handle client requests */
						process_request(fd);
						//rio_writen(fd, buf, l);
						/* Send result to client, potentially with sendfile(1) */
						close(fd);
					}
				}
			}
		}
	}
}
