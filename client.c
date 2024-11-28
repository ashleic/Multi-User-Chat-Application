#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h> // For socket programming
#include <pthread.h> // For creating thread

#define PORT 8080 // Port number to connect to
#define BUFFER_SIZE 1024 // Buffer size for messages

// Function to handle receiving messages from the server
void *receive_messages(void *socket_desc) {
    int socket = *(int *)socket_desc; // Get the socket descriptor
    char buffer[BUFFER_SIZE]; // Buffer to store received messages

    while (1) {
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer
        int bytes_received = recv(socket, buffer, BUFFER_SIZE, 0); // Receive a message
        if (bytes_received <= 0) { // If the server disconnects
            printf("Disconnected from server.\n");
            exit(EXIT_SUCCESS); // Exit the program
        }
        printf("%s", buffer); // Print the received message
    }
    return NULL;
}

int main() {
    int client_socket; // Client socket descriptor
    struct sockaddr_in server_addr; // Server address structure
    char buffer[BUFFER_SIZE]; // Buffer for sending messages

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) { // Check for errors
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address and port
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(PORT); // Port number
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost IP

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Prompt the user for a username
    printf("Enter your username: ");
    fgets(buffer, sizeof(buffer), stdin); // Get the username
    send(client_socket, buffer, strlen(buffer), 0); // Send the username to the server

    // Create a thread to handle incoming messages
    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, &client_socket);

    // Handle sending messages to the server
    while (1) {
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer
        fgets(buffer, BUFFER_SIZE, stdin); // Get the message from the user
        send(client_socket, buffer, strlen(buffer), 0); // Send the message to the server
    }

    close(client_socket); // Close the client socket
    return 0;
}
