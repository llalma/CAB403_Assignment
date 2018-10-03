#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <ctype.h>

#define MAXDATASIZE 100 /* max number of bytes we can get at once */
#define ARRAY_SIZE 150
#define PORTNUMBER 12345


//Recieve array data from client
char *Receive_Array_Int_Data(int socket_identifier, int size){
	int number_of_bytes;
	uint16_t statistics;

	char *results = malloc(sizeof(char)*size);
	for(int i = 0;i<size;i++){
		number_of_bytes = recv(socket_identifier, &statistics,sizeof(uint16_t),0);
		results[i] = ntohs(statistics);
	} 
	return results;
}

void gen_player_welcome (void){
	// Weclome MSG and prompt to log in for USER
	printf("\nWeclome to MineSweaper Online. Please Log in using your details below. \n");
}

void server_connect ( void ){
	int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
	int server_socket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = Internet Protocol v4 Addresses (Family), SOCK_STREAM = TCP, 0 = protocol default

	// Start/define network services
	struct sockaddr_in server_address; // address of server
	server_address.sin_family = AF_INET; // Define protocol
	server_address.sin_port = htons(PORTNUMBER); // Use defined port
	server_address.sin_addr.s_addr = INADDR_ANY; // Use any IP address on local machine IE 0.0.0.0 


	// Connect to the server and report if there is an error doing so
	if (connect(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
		perror("\nUnable to connect to server. Check if the server is online. ");
		exit(1);
	}

	//Loop until user says to exit
	while(1){
		//Recieve array data
		char *results = Receive_Array_Int_Data(sockfd,  ARRAY_SIZE);

		//Print out the array
		printf("\n%s",results);
	
		free(results);

		//These 100 values need to be changed so they can be used 
		//with any size input, might also need to change server file then.
		char input[100];
		fgets(input,100,stdin);

		//Send to the player that all data was recieved by the server.
		send(sockfd,input,40,0);
		//send(sockfd,"All of array data received by server\n", 40 , 0);
	}

}

int main(int argc, char *argv[]) {
	gen_player_welcome();
	server_connect();

	return 0;
}
