#include <error.h>
#include <sys/poll.h>
#include "csapp.h"
#include "loadavg.h"
#include "meminfo.h"
#include <pthread.h>
#include <regex.h>
#define DEFAULT_PORT "11900"
typedef int bool;
#define true 1
#define false 0

void process_request(int, char *);
//void usage(char*);
void read_requesthdrs(rio_t *);
bool parse_uri(char *, char *, char *, char *, int, char *, char *);
void serve_static(int,char *, int, char *, char *, bool);
void get_filetype(char *, char *);
void serve_dynamic(int, char *, char *);
void clienterror(int, char *, char *,char *, char *, char *);
void serve_header(int, char *, int, char *);
void serve_bad_method_header(int, char *);
void file_not_found(int, char *);
bool isValid(char *, char *);
void serve_file(int, char *, int, char *);
//static bool isCallBack;
//static bool badRequest;
void *do_work(void *);
pthread_mutex_t globalLock;
pthread_mutex_t badRequestLock;

struct client_struct
{
    int client_fd;
    char *path;
};


void process_request(int client_fd, char *path) {
    //int is_static;
    struct stat sbuf;
    bool isCallBack;
    struct stat sbuf2;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE], badRequest[MAXLINE];
    rio_t rio;
    //strcpy(badRequest, "false");
    isCallBack = false;
    /** Read request line and headers */
    Rio_readinitb(&rio, client_fd);
    while(true) {
        if (Rio_readlineb(&rio, buf, MAXLINE) <= 0) {
                return;
        }

        printf("%s\n", buf);
        if(sscanf(buf, "%s %s %s", method, uri, version) != 3) {
                return;
        }

        if (strcasecmp(method, "GET")) {
            serve_bad_method_header(client_fd, version);

            return;
        }

        read_requesthdrs(&rio);
        if(uri == NULL) {
            file_not_found(client_fd, version);
            return;
        }


        /** Parse URI from GET request */
        isCallBack = parse_uri(uri, filename, cgiargs, version, client_fd, path, badRequest);
        if(strcasecmp(badRequest, "true") == 0) {
            printf("Bad Request is true");
            file_not_found(client_fd, version);
            return;
        }
        printf("Filname: %s\n", filename);
        if (stat(filename, &sbuf) < 0) {
            printf("Couldn't find file: %s\n", filename);
            file_not_found(client_fd, version);

            return;
        }
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {/**Verify that file is executable and we have read permission*/
                clienterror(client_fd, filename, "403", "Forbidden",
                    "Tiny couldn’t run the CGI program", version);
                return;
        }




        if (!isCallBack) { /* Not CallBack */
            printf("%s\n", filename);
        if(strstr(filename, "/proc/loadavg")) {
            getLoadAvg();
                if (stat("loadavg.txt", &sbuf2) < 0) {
                    file_not_found(client_fd, version);

                    return;
                }

                serve_static(client_fd, "loadavg.txt", sbuf2.st_size, cgiargs, version, isCallBack);

        }
        else if(strstr(filename, "/proc/meminfo")) {
            getMemInfo();
                if (stat("meminfo.txt", &sbuf2) < 0) {
                    file_not_found(client_fd, version);
                    return;
                }
                serve_static(client_fd, "meminfo.txt", sbuf2.st_size, cgiargs, version, isCallBack);
        }
        else {
            /**Not implementing other files yet*/
            if (stat(filename, &sbuf2) < 0) {
                file_not_found(client_fd, version);
                return;
            }
            serve_file(client_fd, filename, sbuf2.st_size, version);
        }

        }
        else {/* CallBack */
            printf("%s\n", filename);
            if(strstr(filename, "/proc/loadavg")) {
                getLoadAvg();
                if (stat("loadavg.txt", &sbuf2) < 0) {
                    file_not_found(client_fd, version);
    //                clienterror(client_fd, filename, "404", "Not found",
    //                    "Tiny couldn’t find this file", version);
                    return;
                }

                serve_static(client_fd, "loadavg.txt", sbuf2.st_size, cgiargs, version, isCallBack);

            }
            else if(strstr(filename, "/proc/meminfo")) {
                getMemInfo();
                if (stat("meminfo.txt", &sbuf2) < 0) {
                    file_not_found(client_fd, version);
    //                clienterror(client_fd, filename, "404", "Not found",
    //                    "Tiny couldn’t find this file", version);
                    return;
                }
                serve_static(client_fd, "meminfo.txt", sbuf2.st_size, cgiargs, version, isCallBack);
            }
            else {
                /**Not implementing other files yet*/
                if (stat(filename, &sbuf2) < 0) {
                    file_not_found(client_fd, version);
    //                clienterror(client_fd, filename, "404", "Not found",
    //                    "Tiny couldn’t find this file", version);
                    return;
                }
                serve_file(client_fd, filename, sbuf2.st_size, version);
            }

        }
        printf("Version: %s\n",version);
        if(strcasecmp(version, "HTTP/1.0") == 0){
            return;
        }
    }
}



void file_not_found(int fd, char *version) {
    char buf[MAXBUF];
    /* Send response headers to client */
    sprintf(buf, "%s 404 Not Found\r\n", version);
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, 0);
    Rio_writen(fd, buf, strlen(buf));
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg, char *version) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "%s %s %s\r\n", errnum, shortmsg, version);
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


bool parse_uri(char *uri, char *filename, char *cgiargs, char *version, int client_fd, char *path, char *badRequest)
{

    strcpy(badRequest, "false");

    bool isCallBack = false;
    char *ptr;
    char *ptr2;
    char *f1 = strstr(uri, "..");
    if(f1) {
        strcpy(badRequest, "true");
        return isCallBack;
    }

    f1 = strstr(uri, "/loadavg");
    char *f2 = strstr(uri, "/meminfo");
    printf("Uri: %s\n", uri);
    if (f1 || f2) { /* Dynamic content */
        printf("Maybe Dynamic\n");
        if(f1){/**get pointer to character after loadavg*/
            ptr = f1 + 8;
            ptr2 = f1 - 1;
        }
        else {
            ptr = f2 + 8;
            ptr2 = f2 - 1;
        }
        printf("Pointer 1: %s\n", ptr);
        if(*ptr2) {/**Bad request*/
            strcpy(badRequest, "true");
            printf("Pointer 2: %s\n", ptr2);
            return isCallBack;
        }
        if(*ptr == '/') {/**Bad request*/
            strcpy(badRequest, "true");
            return isCallBack;
        }
        else if(*ptr == '?') {/**CallBack*/
            ptr = ptr + 1;
            if(!*ptr) {/**no CallBack value*/
                printf("No CallBack Value\n");
                ptr = ptr - 1;
                *ptr = '\0';
                strcpy(cgiargs, "");
                isCallBack = true;
                strcpy(filename, "/proc");
                strcat(filename, uri);
                return isCallBack;
            }
            else {
                bool isvalid = isValid(ptr, cgiargs);
                printf("callback value: %s\n", cgiargs);
                if(isvalid) {
                    strcpy(filename, "/proc");
                    if(f1) {
                        strcat(filename, "/loadavg");
                    }
                    else {
                        strcat(filename, "/meminfo");
                    }
                    isCallBack = true;
                    return isCallBack;
                }
                else {
                    if(strstr(cgiargs, "validReq")) {
                        strcpy(filename, "/proc");
                        if(f1) {
                            strcat(filename, "/loadavg");
                        }
                        else {
                            strcat(filename, "/meminfo");
                        }
                        isCallBack = false;
                        return isCallBack;
                    }
                    strcpy(badRequest, "true");
                    return isCallBack;
                }
            }
        }
        else if(!*ptr) {/**Not callback return dynamic value*/
            strcpy(filename, "/proc");
            strcat(filename, uri);
            return isCallBack;
        }
        else {/**append path and look for filename*/
//            strcpy(filename, "~/");
//            strcat(filename, path);
//            strcat(filename, uri);
            strcpy(filename, path);
            strcat(filename, uri);
            return isCallBack;
        }

    }

    else {/**append path and look for filename*/
//        strcpy(filename, "~/");
//        strcat(filename, path);
//        strcat(filename, uri);
        strcpy(filename, path);
        strcat(filename, uri);
        return isCallBack;
    }
}

bool isValid(char *ptr, char *cgiargs) {
    bool valid = false;
    char *charPtr1;
    char *charPtr2;
    char *strPtr;
    const char *s = "&";
    char *token;
    /* get the first token */
   token = strtok(ptr, s);

   /* walk through other tokens */
   while( token != NULL )
   {
      printf( "Token: %s\n", token );
      strPtr = strstr(token, "callback");
      printf( "strPtr: %s\n", strPtr );
      if(strPtr) {
        charPtr1 = strPtr - 1;
        printf( "value before callback: %s\n", charPtr1 );
        if(!*charPtr1 || *charPtr1 == '?') {
            charPtr2 = strPtr + 8;
            printf( "value after callback: %s\n", charPtr2 );
            if(*charPtr2 == '=') {
                valid = true;
                charPtr2 = charPtr2 + 1;
                printf( "callback value!: %s\n", charPtr2 );
                //cgiargs = charPtr2;
                strcpy(cgiargs, charPtr2);
                break;
            }
        }
        else{
            strPtr = strstr(token, "=");
            if(!strPtr) {
                printf( "callback value!: %s\n", token );
                valid = true;
                strcpy(cgiargs, token);
                break;
            }
            printf( "callback value!: %s\n", token );
            valid = false;
            strcpy(cgiargs, "validReq");
        }
      }
      else{
        strPtr = strstr(token, "=");
        if(!strPtr) {
            printf( "callback value!: %s\n", token );
            valid = true;
            strcpy(cgiargs, token);
            break;
        }
         printf( "value!: %s\n", token );
         valid = false;
         strcpy(cgiargs, "validReq");

      }
      token = strtok(NULL, s);
   }
   printf("callback value: %s\n", cgiargs);
   return valid;
}

void serve_static(int fd, char *filename, int filesize, char *cgiargs, char *version, bool isCallBack)
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
    serve_header(fd, filename, filesize, version);
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


void serve_file(int fd, char *filename, int filesize, char *version) {
    int srcfd;
    char *srcp;
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    serve_header(fd, filename, filesize, version);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

void serve_header(int fd, char *filename, int filesize, char *version) {
    char filetype[MAXLINE], buf[MAXBUF];
    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "%s 200 OK\r\n", version);
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
}

void serve_bad_method_header(int fd, char *version) {
    char buf[MAXBUF];
    /* Send response headers to client */
    sprintf(buf, "%s 501 NOT IMPLEMENTED\r\n", version);
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, 0);
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

void *do_work(void *value)
{

    //detach the thread so that when terminated the kernel reaps automatically
    pthread_detach(pthread_self());
    //retrieve arguments
    struct client_struct *args = (struct client_struct *) value;

    process_request(args->client_fd, args->path);

    close(args->client_fd);
    free(value);

    return NULL;
}


int main(int ac, char **av) {

	char *port = DEFAULT_PORT;
	char *path = "./";
	pthread_t newthread;
    pthread_mutex_init(&globalLock, NULL);
    pthread_mutex_init(&badRequestLock, NULL);
    pthread_attr_t atr;
    pthread_attr_init(&atr);

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
    hints.ai_family = AF_INET6;
    hints.ai_protocol = 0;
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
			if (listen(fds[nfds].fd, 8) != 0)
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

						struct client_struct *client = Malloc(sizeof(struct client_struct));
						client->client_fd = fd;
						client->path = path;

						if (pthread_create(&newthread , &atr, do_work, client) != 0){
                           perror("pthread_create");
                         }
						//rio_writen(fd, buf, l);
						/* Send result to client, potentially with sendfile(1) */
						//close(fd);
					}
					else {
                        exit(EXIT_FAILURE);
					}
				}
			}
		}
		pthread_mutex_destroy(&globalLock);
		pthread_mutex_destroy(&badRequestLock);
	}
}

