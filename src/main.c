/*-
 * Copyright (c) 2016, Jason Waataja
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* The file descriptor for the incoming socket.  Being something other than -1
 * means that it hasn't been set yet and doesn't need to be closed.  */
int sockfd = -1;

/* Whether or not the main loop should continue.  */
int should_continue = 1;

/* If this gives an error, used -std=gnu99 */
#define 	SERVER_PORT "8110"

/* The amount of connections to keep in the backlog.  */
#define 	BACKLOG 20

static void	close_socket();
static void	handle_connection(int socket);
static void	init_networking();
static void	main_loop();
static void	print_address(FILE *stream, struct sockaddr *sockaddr);
static void	signal_handler(int signum);

/* 
 * Set up the networking.  Creates the port and sets sockfd to the value.
 * Exits the program if there's an error.
 */
static void
init_networking()
{
		struct addrinfo *host_addr, hints;
		int status, sockfd;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		status = getaddrinfo(NULL, SERVER_PORT, &hints, &host_addr);

		if (status == -1)
			err(1, "Failed to create address");

		sockfd = socket(host_addr->ai_family, host_addr->ai_socktype,
			host_addr->ai_protocol);

		if (sockfd == -1)
			err(1, "Failed to create socket");

		status = bind(sockfd, host_addr->ai_addr, host_addr->ai_addrlen);

		if (status == -1)
			err(1, "Failed to bind port %s", SERVER_PORT);

		freeaddrinfo(host_addr);

		signal(SIGINT, signal_handler);
		signal(SIGTERM, signal_handler);

		listen(sockfd, BACKLOG);
}

/*
 * This was mainly intended to handle termination signals so that it can exit
 * the infinite loop gracefully.
 */
static void
signal_handler(int signum)
{

		close_socket();
		/* Do a bunch of stuff to terminate the worker threads here.  */

		/* Set the signal back to its original value and use that.  */
		signal (signum, SIG_DFL);
		raise (signum);
}

/* Start accepting connections.  Exists the program if there's an error.  */
static void
main_loop()
{
		struct sockaddr_storage their_sock;
		socklen_t their_socklen;
		int newfd;

		/* Loop forever until should_continue is set to false.  */
		while (should_continue) {
			printf("Accepting connections.\n");

			/* Accept a connection.  */
			newfd = accept (sockfd, (struct sockaddr *)&their_sock,
				&their_socklen);

			if (newfd == -1)
				warn ("Failed to accept connection");

			printf("Got a connection from ");
			print_address(stdout, (struct sockaddr *)&their_sock);
			printf("\n");

			handle_connection(newfd);
		}

		close_socket();
}

/* Closes the socket, supposed to be on exit.  */
static void
close_socket()
{

		if (sockfd > -1)
			close (sockfd);

		sockfd = -1;
}

/*
 * Prints the address (inet or inet6) to the given stream.  I decided to it this
 * way because returning a char array would mean that it would need to be freed
 * with free() which is annoying and ties it to malloc () (as opposed to c++'s
 * delete.
 */
static void
print_address(FILE *stream, struct sockaddr *sockaddr)
{

		if (sockaddr->sa_family == AF_INET)
		{
			struct sockaddr_in *as_inet = (struct sockaddr_in *)sockaddr;
			char ip[INET_ADDRSTRLEN];

			inet_ntop (AF_INET, &(as_inet->sin_addr), ip, INET_ADDRSTRLEN);

			fprintf (stream, "%s", ip);
		}
		else if (sockaddr->sa_family == AF_INET6)
		{
			struct sockaddr_in6 *as_inet6 = (struct sockaddr_in6 *)sockaddr;
			char ip[INET6_ADDRSTRLEN];

			inet_ntop(AF_INET6, &(as_inet6->sin6_addr), ip, INET6_ADDRSTRLEN);

			fprintf(stream, "%s", ip);
		}
}

/*
 * Handle the conection already accepted with socket. This function is
 * responsible for closing the socket.
 */
static void
handle_connection(int socket)
{
		/* TODO: Handle the connection here. */
		close(socket);
}

/*
 * TODO: Put the program description here.
 */
int
main(int argc, char *argv[])
{
		init_networking ();
		main_loop ();
}
