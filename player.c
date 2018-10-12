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

#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10

typedef struct{
	char tiles[NUM_TILES_X][NUM_TILES_Y];

	//0 = loss, 1 = play on, 2 = win.
	int game_state;
} Board;

//Globals
Board previous_board;	//saves previous state of board, for use in placing flags
int remaining_mines;

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

void display_board(Board board){
	//Display current gameboard

	//Display number of mines remining
	printf("\nRemaining mines  = %d\n",remaining_mines);

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
			if(board.tiles[j][i] != 0 && board.tiles[j][i] != 'X'){
				printf(" %c", board.tiles[j][i]);
			}else{
				printf("  ");
			}	
		}
	}
}

Board load_board(int server_socket){
	int number_of_bytes;
	uint16_t statistics;
	remaining_mines = NUM_MINES;
	Board board;
	board.game_state = 1;
	bool flags_check = true;	//false = tried to place flag where there was no mine

	for (int i = 0; i < 9; i++) {
		for(int j =0;j<9;j++){
			number_of_bytes = recv(server_socket, &statistics,sizeof(uint16_t),0);
			board.tiles[j][i] = ntohs(statistics);
			if(board.tiles[j][i] == '*'){
				board.game_state = 0;
			}else if(board.tiles[j][i] == '+'){
				remaining_mines--;
			}
		}
	}

	//PLayer wins
	if(remaining_mines <= 0){
		board.game_state = 2;
	}

	//Player tried to place a flag where there was no mine.
	for(int j =0;j<NUM_TILES_Y;j++){
		for(int i = 0;i<NUM_TILES_X;i++){
			if(previous_board.tiles[i][j] != board.tiles[i][j] && board.tiles[i][j] == 'X'){
				flags_check = false;
			}
		}
	}

	previous_board = board;

	display_board(board);

	if(flags_check == false){
		printf("\n\n No mines here\n");
	}

	return board;
}

void gen_player_welcome (void){
	// Weclome MSG and prompt to log in for USER
	printf("\nWeclome to MineSweaper Online. Please Log in using your details below. \n");
}

bool check_input(char* input,Board board){
	//Checks the users input is a correct function and within the limits of the game board
	bool input_check = true;

	//Return immidetly as there are no other inputs when quitting the game
	if(toupper(input[0]) == 'Q'){
		return true;
	}

	//Input check for task
	if(toupper(input[0]) != 'R' && toupper(input[0]) != 'P'){
		input_check = false;
	}

	//Letter coordinates
	if((toupper(input[1]) < 'A' || toupper(input[1]) >'I') && input_check == true){
		input_check = false;
	}

	//Digit coordinates
	if((toupper(input[2]) < '1' || toupper(input[2]) > '9') && input_check == true){
		input_check = false;
	}

	//If tile is already revealed

	//Covnvert users coordinates to integers
	int Y_Coords = (int) toupper(input[1]) - (int)'A';
	int X_Coords = (int) input[2] - (int)'0'-1;

	if(input_check == true){
		if(board.tiles[X_Coords][Y_Coords] != 0 && board.tiles[X_Coords][Y_Coords] != 'X' ){
			input_check = false;
		}
	}

	return input_check;
}

char main_menu(void){

	printf("\n\nMain Menu");
	printf("\nPlease enter a selection:	\n<1> Play MineSweeper \n<2> Show leaderboard \n<3> Quit\n");

	//needs to auto change with any size input, might also need to change server file then.
	char input[100];
				
	while(1){

		//Loop until a valid input
		input[0] = getchar();

		if(input[0] == '3'){
			//User has quit the game
			return '3';
		}else if(toupper(input[0]) == '1'){
			//User wishes to play minesweeper
			return '1';
		}else if(toupper(input[0]) == '2'){
			//User wishes to see leaderboard
			return '2';
		}else{
			printf("\nInvalid input\n");
		}
	}
}

void leaderboard(int server_socket){

	//Recieve leaderboard data
	char *results = Receive_Array_Int_Data(server_socket,  ARRAY_SIZE);

	while(strcmp(results, "NULL\0") != 0){
		printf("\n%s",results);

		results = Receive_Array_Int_Data(server_socket,ARRAY_SIZE);
	}

	// printf("\n\n%s\n","Play again? (Y/N)");

	// //needs to auto change with any size input, might also need to change server file then.
	// char input[2];
	// fgets(input,2,stdin);

	// while(toupper(input[0]) != 'Y' && toupper(input[0]) != 'N'){
	// 	printf("Invalid input");
	// 	printf("\n\n%s\n","Play again? (Y/N)");

	// 	fgets(input,2,stdin);
	// }

	// send(server_socket,input,40,0);

	// //Quit game
	// if(toupper(input[0]) == 'N'){
	// 	//Disconnect from server
	// 	printf("\nThanks for playing MineSweaper!\n");
	// 	return true;
	// }

	// //Clear input buffer
	// while(getchar() != '\n');

	// //Play new game.
	// return false;	
}


int play_game(int server_socket, bool login){

	//Users selection on the main menu
	char selection = '0';

	//Loop until user says to exit
	while(1){

		if(login == true){

			if(selection == '0'){
				selection = main_menu();
				char Selection[2];
				Selection[0] = (char)selection;

				//Send to server the users selection
				send(server_socket,Selection ,2,0);
			}

			if(selection == '3'){
				//User has quit the game

				//Clear input buffer
				while(getchar() != '\n');
				
				printf("\nConnection to Server closed\n");
				return 1;
			}else if(selection == '2'){
				//User wishes to see leaderboard

				//Clear input buffer
				while(getchar() != '\n');

				return 0;

			}else{
				//User wishes to play game

				Board board = load_board(server_socket);

				if(board.game_state == 0){
					//Player has selected a mine and lost
					printf("\n\nYou Lose\n");
					selection = 0;
					return 0;
				}else if(board.game_state == 2){
					//Player has placed all the flags and won
					printf("\n\nYou Win\n");
					selection = 0;
					break;
				}else{

					//Display options for user
					printf("\nOptions:");
					printf("\n<R> Reveal tile");
					printf("\n<P> Place flag");
					printf("\n<Q> Quit game\n");
					
					//Need to ensure users input is correct format
					printf("\nPlease input coordinates (Option)(A-I)(1-9)\n");	

					//Clear input buffer
					while(getchar() != '\n');	

					//needs to auto change with any size input, might also need to change server file then.
					char input[5];
					fgets(input,4,stdin);

					//Input verifications - True == input was allowed
					bool input_check = check_input(input,board);

					while(input_check == false){
						display_board(board);
						printf("\nInvalid input.");
						printf("\nPlease input coordinates (Option)(A-I)(1-9)\n");

						//Get users input
						fgets(input,4,stdin);

						input_check = check_input(input,board);
					}
					
					//Send selection and users sqaure to server
					send(server_socket,input,40,0);
					
					//Quit game
					if(toupper(input[0]) == 'Q'){
						selection = 0;;
					}
				}
			}

			//Clear input buffer
			while(getchar() != '\n');	

		}else{
			
			//Recieve array data
			char *results = Receive_Array_Int_Data(server_socket,  ARRAY_SIZE);
			
			if(strcmp(results, "Login Successful") == 0){
				login = true;
				//Print out the array
				printf("\n%s",results);	
				free(results);

			}else if(strcmp(results, "You entered either an incorrect username or password. Disconnecting.\0") == 0){
				//Invalid login details

				//Print out the array
				printf("\n%s\n",results);
			
				free(results);

				close(server_socket);
				break;

			}else{

				//Print out the array
				printf("\n%s",results);
			
				free(results);

				//These 100 values need to be changed so they can be used 
				//with any size input, might also need to change server file then.
				char input[100];
				fgets(input,100,stdin);

				//////////////input error checking


				//Send to the player that all data was recieved by the server.
				send(server_socket,input,40,0);
				//send(sockfd,"All of array data received by server\n", 40 , 0);
			}
		}
	}
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

	bool login = false;
	bool quit_game = false;

	//Display main menu
	while(1){

			if(play_game(server_socket,login) == 1){
				//User has quit game
				break;
			}

			//Clear input buffer
			//while(getchar() != '\n');

			printf("\nhere\n");

			leaderboard(server_socket);
			printf("\n");

			login = true;
		}
	

	//Close connection with server
	close(server_socket);
}

int main(int argc, char *argv[]) {
	gen_player_welcome();
	server_connect();

	return 0;
}
