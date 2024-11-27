#include <stdio.h> // 
#include <stdlib.h> // 
#include <string.h> // 
#include <unistd.h> // 
#include <arpa/inet.h> // For socket programming (inet_addr, sockaddr_in)
#include <pthread.h> // For creating threads

#define PORT 8080 // Port number for the server
#define MAX_CLIENTS 10 // Maximum number of clients
#define BUFFER_SIZE 1024 // Buffer size for messages

// Structure to store client information
typedef struct {
    int socket; // Socket descriptor for the client
    char username[32]; // Username of the client
} Client;

// Global variables for client management
Client clients[MAX_CLIENTS]; // Array to hold connected clients
int client_count = 0; // Current number of connected clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread-safe client list

// Function to broadcast a message to all clients except the sender
void broadcast_message(const char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex); // Lock the client list to ensure thread safety
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket != sender_socket) { // Skip the sender
            send(clients[i].socket, message, strlen(message), 0); // Send the message to other clients
        }
    }
    pthread_mutex_unlock(&clients_mutex); // Unlock the client list
}

// Function to handle communication with a specific client
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE]; // Buffer to store received messages
    char username[32]; // Buffer to store the client's username

    // Receive the username from the client
    recv(client_socket, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = '\0'; // Remove newline character from the username

    // Add the client to the global client list
    pthread_mutex_lock(&clients_mutex); // Lock the client list
    strcpy(clients[client_count].username, username); // Store the username
    clients[client_count].socket = client_socket; // Store the socket descriptor
    client_count++; // Increment the client count
    pthread_mutex_unlock(&clients_mutex); // Unlock the client list

    // Notify all clients that a new user has joined
    sprintf(buffer, "%s joined the chat.\n", username);
    broadcast_message(buffer, client_socket);

    // Handle client messages in a loop
    while (1) {
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0); // Receive a message from the client

        if (bytes_received <= 0) { // If the client disconnects or an error occurs
            // Notify other clients that the user has left
            sprintf(buffer, "%s left the chat.\n", username);
            broadcast_message(buffer, client_socket);

            // Remove the client from the global client list
            pthread_mutex_lock(&clients_mutex); // Lock the client list
            for (int i = 0; i < client_count; i++) {
                if (clients[i].socket == client_socket) { // Find the client
                    for (int j = i; j < client_count - 1; j++) {
                        clients[j] = clients[j + 1]; // Shift remaining clients
                    }
                    client_count--; // Decrease the client count
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex); // Unlock the client list
            close(client_socket); // Close the client socket
            return; // Exit the thread
        }

        // Format the message to include the username
        buffer[bytes_received] = '\0'; // Null-terminate the message
        char message[BUFFER_SIZE + 32]; // Buffer to store the formatted message
        sprintf(message, "%s: %s", username, buffer); // Add the username to the message

        // Broadcast the message to all clients
        broadcast_message(message, client_socket);
    }
}

int main() {
    int server_socket, client_socket; // Server and client socket descriptors
    struct sockaddr_in server_addr, client_addr; // Server and client address structures
    socklen_t addr_len = sizeof(client_addr); // Length of the client address structure

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) { // Check for errors
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address and port
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
    server_addr.sin_port = htons(PORT); // Port number

    // Bind the socket to the address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);

    // Accept incoming connections in a loop
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len); // Accept a new connection
        if (client_socket < 0) { // Check for errors
            perror("Accept failed");
            continue;
        }

        // Create a new thread for the client
        pthread_t thread;
        if (pthread_create(&thread, NULL, (void *)handle_client, (void *)(intptr_t)client_socket) != 0) {
            perror("Thread creation failed");
            close(client_socket);
        }
    }

    close(server_socket); // Close the server socket
    return 0;
}
