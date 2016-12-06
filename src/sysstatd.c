#include <error.h>
#include <sys/poll.h>
#include "csapp.h"
#include "loadavg.h"
#include "meminfo.h"

#define DEFAULT_PORT "13011"

/** Much of this method is heavily based on the main method for a server found on 
 * www.akkadia.org/dreppper/userapi-ipv6.html
 * which was linked in the project spec */
int main(int ac, char **av) {
	char *port = DEFAULT_PORT;
//	char *path;

	int c;
	while ((c = getopt(ac, av, "p:R:")) != EOF) {
		switch (c) {
		case 'p':
			port = optarg;
			break;
//		case 'R':
//			path = optarg;
		}
	}

	struct addrinfo *ai;
	struct addrinfo hints;
	memset(&hints, '\0', sizeof(hints));
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
	hints.ai_socktype = SOCK_STREAM;
	
	int e = getaddrinfo(NULL, port, &hints, &ai);
	if (e != 0)
		error(EXIT_FAILURE, 0, "getaddrinf0: %s", gai_strerror(e));

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
						char buf[1024];
						ssize_t l = rio_readn(fd, buf, sizeof(buf));
						/* Parse and handle client requests */
						rio_writen(fd, buf, l);
						/* Send result to client, potentially with sendfile(1) */
						close(fd);
					}
				}
			}
		}
	}
}
