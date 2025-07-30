#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int initializeSocket(){
	return socket(AF_INET, SOCK_STREAM, 0);
}

int handleReuse(int server_fd) {
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return -1;
	}
	return 0;
}

int bindSocketToPort(int server_fd, int port) {
	struct sockaddr_in serv_addr = { .sin_family = AF_INET, // Address family (IPv4)
									  .sin_port = htons(port), // Port number in network byte order
									  .sin_addr = { htonl(INADDR_ANY) }, // Bind to any available interface
									};
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return -1;
	}
	return 0;
}

int socketListen(int server_fd, int connection_backlog) {
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return -1;
	}
	return 0;
}

int isGetRequest(const char *buffer) {
	// Check if the request starts with "GET /"
	return strncmp(buffer, "GET /", 5) == 0;
}

char* extractUrl(const char *buffer, char *url) {
	// Extract the URL from the GET request
	sscanf(buffer, "GET /%1023s", url);
	return url;
}

char* basicResponse(char *response, int status_code) {
	if ( status_code != 200 && status_code != 404) {
		printf("Unsupported status code: %d\n", status_code);
		status_code = 404; // Default to 404 if an unsupported status code is provided
	}

	const char *status_text = (status_code == 200) ? "OK" : "Not Found";
	snprintf(response, 2048,
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: 0\r\n"
		"\r\n",
		status_code, status_text);
	return response;
}

char* basic200ResponseWithBody(char *response, const char *body) {
	size_t body_length = strlen(body);
	snprintf(response, 2048,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: %zu\r\n"
		"\r\n"
		"%s",
		body_length, body);
	return response;
}

char* basicFileResponse(char *response, const char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        // File not found, return 404 response
        return basicResponse(response, 404);
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

	printf("entree en fonction");
    
    // Check if file is too large for our buffer
    // Reserve space for HTTP headers (estimate ~100 bytes)
    if (file_size > 6000) {
        fclose(file);
        printf("File too large: %ld bytes\n", file_size);
        return basicResponse(response, 404);
    }
    
    // Read file content
    char file_content[6000] = {0};
    size_t bytes_read = fread(file_content, 1, file_size, file);
	printf("File read: %s, size: %zu bytes\n", file_content, bytes_read);
    fclose(file);
    
    if (bytes_read != file_size) {
        printf("Error reading file: %s\n", file_name);
        return basicResponse(response, 404);
    }
    
    // Determine content type based on file extension
    const char *content_type = "text/plain";
    const char *ext = strrchr(file_name, '.');
    if (ext != NULL) {
        if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
            content_type = "text/html";
        } else if (strcmp(ext, ".css") == 0) {
            content_type = "text/css";
        } else if (strcmp(ext, ".js") == 0) {
            content_type = "application/javascript";
        } else if (strcmp(ext, ".json") == 0) {
            content_type = "application/json";
        }
    }
    
    // Create HTTP response
    snprintf(response, 8048,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        content_type, bytes_read, file_content);
    
    return response;
}

int serveClient(int client_fd){
		int bytes_received;
		char buffer[4096] = {0};

		if (client_fd < 0) {
			printf("Accept failed: %s\n", strerror(errno));
			return 1;
		}

		// ----------------------------------- RECEIVING DATA FROM CLIENT -----------------------------


		bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
		if (bytes_received < 0) {
			printf("Recv failed: %s\n", strerror(errno));
			close(client_fd);
			return 1;
		}

		// ----------------------------------- PROCESSING REQUEST -----------------------------------
    char response[2048];
    if (isGetRequest(buffer)) {
        char url[4096] = {0};
        extractUrl(buffer, url);
        printf("%s\n", url);

        // Build full filepath: <cwd>/WebFiles/<url>
        char cwd[2048];
        if (!getcwd(cwd, sizeof(cwd))) {
            perror("getcwd");
            exit(1);
        }

        size_t len = strlen(cwd)
                   + strlen("/WebFiles/")
                   + strlen(url)
                   + 5;
        char* fileName = malloc(len);
        if (!fileName) {
            perror("malloc");
            exit(1);
        }
		printf("%s\n", url);
        snprintf(fileName, len, "%s/src/WebFiles/%s", cwd, url);

		printf("%s\n", fileName );

        basicFileResponse(response, fileName);
        free(fileName);

		} else {
			basicResponse(response, 404);
		}
		send(client_fd, response, strlen(response), 0);

		close(client_fd);
		return 0;
}


int main() {

	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	// ------------------------------- SOCKET CREATION -------------------------------------
	server_fd = initializeSocket();

	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return -1;
	}

	// ------------------------------- RESTARTING MANAGER -----------------------------------
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuseState = handleReuse(server_fd);

	if (reuseState != 0) {
		printf("Failed to set SO_REUSEADDR: %s \n", strerror(errno));
		return 1;
	}	

	// ------------------------------- BINDING TO PORT --------------------------------------
	int port = 8080; // Port to bind the server to
	if (
		bindSocketToPort(server_fd, port) != 0
	) {
		printf("Failed to bind socket to port %d: %s \n", port, strerror(errno));
		return 1;
	}
	// ------------------------------- LISTENING FOR CONNECTIONS -----------------------------
	
	int connection_backlog = 5; // Maximum number of pending connections
	if (socketListen(server_fd, connection_backlog) != 0) {
		printf("Failed to listen on socket: %s \n", strerror(errno));
		return 1;
	}
	printf("Server is listening on port %d...\n", port);

	
	// ------------------------------- ACCEPTING CONNECTIONS --------------------------------

	int client_fd;
	int handleStatus;
	while (1) {

		client_addr_len = sizeof(client_addr);
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

		handleStatus = serveClient(client_fd);
		if (handleStatus != 0) {
			printf("Error handling client: %s\n", strerror(errno));
			continue;
		} else {
			printf("Client served successfully.\n");
		}

	}
	close(server_fd);

	return 0;
}



