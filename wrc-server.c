#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

int main(int argc, char *argv[]){

    int index;
    // Default port number
    int port = 3838;
    char *pEnd;

    // If a port number is passed into the program when run
    if(argc > 1){
        // Make sure the usage is correct
        if(argc == 2){
            // Only allow ports in range 1-9999
            char portNumber[4];
            // Get the argument
            strcpy(portNumber, argv[1]);
            // Make sure they are all digits before using strtol
            for(index = 0; index < strlen(argv[1]); index++){
                if(isdigit(portNumber[index])){
                    continue;
                } else{
                    // If they are not all digits - Prompt the user that the usage is wrong and exit
                    // to avoid a failed call to strtol
                    printf("%s", "Usage - ./wrc-server portnumber(1-9999)\n");
                    exit(1);
                }
            }
            // Get the new port number
            port = (int)strtol(argv[1], &pEnd, 10);
        } else{
            // Improper argument count
            printf("%s", "Usage - ./wrc-server portnumber(1-9999)\n");
            exit(1);
        }
    }   

    // Notify of server start
    printf("Starting server on port %d\n", port);
    fflush(stdout);
    // Create server sockets and connections
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Define the server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the port
    bind(server_socket, (struct sockaddr*) & server_address, sizeof(server_address));

    // Listen for connections
    listen(server_socket, 5);

    // Client socket descriptor
    int client_socket;

    // Always run and listen for requests
    while(1){
        // Response status code
        // Large sizes to handle listing of something long like ls "-la /bin"
        // The response to the client
        char response[32768];
        // The data taken in
        char data[1024];
        // The output from the command
        char output[16384];
        // The number of characters in the number for character count
        char charCountString[6];

        // Accept the connection
        client_socket = accept(server_socket, NULL, NULL);

        // Only do this if the connection occured
        if(client_socket != -1){
                // Read the request into the array
            read(client_socket, data, sizeof(data));

            // Patterns
            char *startString = "GET /";
            char *endString = " HTTP";
            // Pointers for finding command location
            char *command = NULL;
            char *start, *end;

            // Parse the command from between GET and HTTP
            if ((start = strstr(data, startString))){
                start += strlen(startString);
                if ((end = strstr( start, endString))){
                    command = (char*)malloc(end - start + 1);
                    memcpy(command, start, end - start);
                    command[end - start] = '\0';
                }
            }

            // Index
            int i;

            // Replace special character with their inteded characters (space)
            // Disallows the use of & -- > This is for passing the correct command to system
            for(i = 0; i < strlen(command); i++){
                if(command[i] == '*'){
                    command[i] = ' ';
                } else if(command[i] == '&'){
                    command[i] = ' ';
                } 
            }

            // Error flag
            int s;
            strcat(command, " >> temp.txt");
            s = system(command);

            // Check for an error and send it back if encountered
            if(s != 0){
                // Add error to the output
                strcat(output, "BASH ERROR!");
            } else {

                // Open a temp file for reading the output of system
                FILE *runCommand;
                runCommand = fopen("temp.txt", "r");

                // The current character
                char c;
                // The number of characters in the file
                int charCount = 0;
                i = 0;

                // Parse the file
                while(1){
                    c = fgetc(runCommand);
                    if(c == EOF){
                        break;
                    }
                    // Replace newlines with <br />
                    // so that the newlines print properly in the web browser
                    if(c == '\n'){
                        output[i] = '<';
                        i++;
                        output[i] = 'b';
                        i++;
                        output[i] = 'r';
                        i++;
                        output[i] = ' ';
                        i++;
                        output[i] = '/';
                        i++;
                        output[i] = '>';
                        i++;
                        charCount++;
                    } else {
                        output[i] = c;
                        i++;
                        charCount++;
                    }
                }

                // Convert the number of characters in the file to a string
                sprintf(charCountString, "%d", charCount);

                // Close the temp file and delete it
                fclose(runCommand);
                system("rm temp.txt");
            }
            
            // Assemble the html response
            strcat(response , "HTTP/1.1 200 OK\r\n\n");
            strcat(response, "<html><body>");
            strcat(response , "HTTP/1.1 200 OK<br />");
            strcat(response, "Content-Type: text/plain <br />");
            strcat(response, "Content-Length: ");
            strcat(response, charCountString);
            strcat(response, "<br /><br />");
            strcat(response, output);
            strcat(response, "</body></html>");

            // Send the response and close the socket
            send(client_socket, response, strlen(response), 0);
            close(client_socket);

            // Print to console
            printf("%s", "Response successfully sent to client - exiting.\n");
            fflush(stdout);
            exit(0);
        }        
    }
    return 0;
}
