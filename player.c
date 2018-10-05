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
#include <stdbool.h>

#define MAXDATASIZE 100 /* max number of bytes we can get at once */
#define ARRAY_SIZE 150
#define PORTNUMBER 12345

void display_board(char *board){
	//Display current gameboard

	//Create an array of letters to use
	char letters[10]; // cause there is an end line char
	strcpy(letters,"ABCDEFGHI");

	//Printing top 2 rows of board
	printf("\n  ");
	for(int i = 1;i<10;i++){
		printf(" %d",i);
	}
	printf("\n");
	for(int i = 0;i<21;i++){
		printf("-");
	}

	for(int i = 0; i<9;i++){
		printf("\n%c|",letters[i]);
		for(int j = 0;j<9;j++){
			printf(" %c", board[j+i*10]);
		}
	}
}

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

int server_connect ( void ){
	int new_fd;  /* listen on sock_fd, new connection on new_fd */
	int server_socket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = Internet Protocol v4 Addresses (Family), SOCK_STREAM = TCP, 0 = protocol default

	// Start/define network services
	struct sockaddr_in server_address; // address of server
	server_address.sin_family = AF_INET; // Define protocol
	server_address.sin_port = htons(PORTNUMBER); // Use defined port
	server_address.sin_addr.s_addr = INADDR_ANY; // Use any IP address on local machine IE 0.0.0.0 

	// Connect to the server and report if there is an error doing so
	if (connect(server_socket, (struct sockaddr *) &server_address, sizeof(struct sockaddr)) == -1) {
		perror("\nUnable to connect to server. Check if the server is online. ");
		exit(1);
	}

	bool login;

	//Loop until user says to exit
	while(1){
		//Recieve array data
		char *results = Receive_Array_Int_Data(server_socket,  ARRAY_SIZE);

		if(login == true){
			display_board(results);
			free(results);

			//Display options for user
			printf("\nChoose and option:");
			printf("\n<R> Reveal tile");
			printf("\n<P> Place flag");
			printf("\n<Q> Quit game\n");

			//Get users selection, can only be a r,p or q.
			//char input_key = getchar();
			printf("\n%c\n",getchar());
			// while(input_key != 'R' || input_key != 'P' || input_key != 'Q'){
			// 	input_key = toupper(getchar());
			// 	printf("\nhere\n");
			// }

			//Need to ensure users input is correct format
			printf("\nPlease input coordinates (A-I),(1-9)\n");

			//These 100 values need to be changed so they can be used 
			//with any size input, might also need to change server file then.
			char input[5];
			fgets(input,4,stdin);

			//Add if the user is revealing a mine or placing a flag
			//input[5] = input_key;

			//Send selection and users sqaure to server
			send(server_socket,input,40,0);

		}else{
			

			if(strcmp(results, "Login Successful") == 0){
				login = true;
				//Print out the array
				printf("\n%s",results);	
				free(results);
			}else{

				//Print out the array
				printf("\n%s",results);
			
				free(results);

				//These 100 values need to be changed so they can be used 
				//with any size input, might also need to change server file then.
				char input[100];
				fgets(input,100,stdin);

				//Send to the player that all data was recieved by the server.
				send(server_socket,input,40,0);
				//send(sockfd,"All of array data received by server\n", 40 , 0);
			}
		}
	}

}

int main(int argc, char *argv[]) {
	gen_player_welcome();
	server_connect();

	return 0;
}
