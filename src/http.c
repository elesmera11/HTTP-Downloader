#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "http.h"

#define BUF_SIZE 1024


#define FORMAT "GET %s%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: getter\r\n\r\n"
#define FORMAT_LEN 80

/*
 * create socket and connect to server
 */
int init_sock(char* hostname, int port) {
	//local vars
	char port_no[6];
	struct addrinfo their_addrinfo;
	struct addrinfo *their_addr = NULL;
	//create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Couldn't create socket.\n");
		exit(1);
	}
	//get server address info
	memset(&their_addrinfo, 0, sizeof(struct addrinfo));
	their_addrinfo.ai_family = AF_INET;
	their_addrinfo.ai_socktype = SOCK_STREAM;
	sprintf(port_no, "%d", port);
	getaddrinfo(hostname, port_no, &their_addrinfo, &their_addr);
	//connect
	int rc = connect(sockfd, their_addr->ai_addr, their_addr->ai_addrlen);
	freeaddrinfo(their_addr); //done with addrinfo, can free
	if(rc == -1) {
		perror("Couldn't connect to server.\n");
		close(sockfd);
		exit(1);
	}
	return sockfd;
}

/*
 * Initiate the buffer
 */
Buffer* init_buff(size_t size) {
	Buffer* buff = malloc(sizeof(Buffer));
	buff->data = calloc(size, sizeof(char));
	buff->length = size;
	return buff;
}

/*
 * Formats data into a transmittable query string
 */
char* formatter(int length, char* page, char* host) {
	char* str = malloc(length);
	//format query string correctly, accounting for presence of '/'
	if (page[0] == '/') sprintf(str, FORMAT, "", page, host);
	else sprintf(str, FORMAT, "/", page, host);
	return str;
}

/*
 * Reads the response from the server into the buffer
 */
void process_response(Buffer* buffer, int sockfd) {
	int recvd, recvd_total = 0;
	do { //receive and error check
		recvd = recv(sockfd, buffer->data + recvd_total, BUF_SIZE, 0);
		if(recvd == -1) {
			perror("Couldn't receive.\n");
			buffer_free(buffer);
			close(sockfd);
			exit(1);
		} //if too large, reallocate memory to continue receiving
		recvd_total += recvd;
		if (recvd_total + BUF_SIZE > buffer->length) {
			buffer->length = buffer->length + BUF_SIZE;
			buffer->data = realloc(buffer->data, buffer->length);
		}
	} while(recvd > 0);
	buffer->length = recvd_total;
	return;
}


Buffer* http_query(char *host, char *page, int port) {
	Buffer* response = init_buff(BUF_SIZE);
	int sockfd = init_sock(host, port);
	//space for null terminator
	int query_len = FORMAT_LEN + strlen(host) + strlen(page) + 1; 
	char* query = formatter(query_len, page, host);
	int sent = send(sockfd, query, strlen(query), 0);
	if (!sent) {
		perror("Couldn't send query.\n");
		close(sockfd);
		free(query);
		exit(1);
	}
	process_response(response, sockfd);
	close(sockfd);
	free(query);
	return response;
}


// split http content from the response string
char* http_get_content(Buffer *response) {

	char* header_end = strstr(response->data, "\r\n\r\n");

	if (header_end) {
		return header_end + 4;
	}
	else {
		return response->data;
	}
}


Buffer *http_url(const char *url) {
	char host[BUF_SIZE];
	strncpy(host, url, BUF_SIZE);

	char *page = strstr(host, "/");
	if (page) {
		page[0] = '\0';

		++page;
		return http_query(host, page, 80);
	}
	else {

		fprintf(stderr, "could not split url into host/page %s\n", url);
		return NULL;
	}
}
