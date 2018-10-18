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

int PORTNUMBER = 12345;    // The port users will be connecting to 

char *Receive_Array_Int_Data(int socket_identifier, int size){
	//Recieve array data from client, returns message from server
	int number_of_bytes;
	uint16_t statistics;

	//Recieves from the socket and saves in results 1 bit of data per loop
	char *results = malloc(sizeof(char)*size);
	for(int i = 0;i<size;i++){
		number_of_bytes = recv(socket_identifier, &statistics,sizeof(uint16_t),0);
		if(number_of_bytes == 0){
			//Socket is close, so close client connection to socket

			printf("\nServer is close\n");
			close(socket_identifier);
		}

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

	//Print by row, row letter then gamebaord row.
	for(int i = 0; i<9;i++){
		printf("\n%c|",letters[i]);
		for(int j = 0;j<9;j++){
			if(board.tiles[j][i] != 0 && board.tiles[j][i] != 'X'){
				printf(" %c", board.tiles[j][i]);	//Print the char (mine/number) on the board
			}else{
				printf("  ");	//Tile is not revealed therefore print blank space.
			}	
		}
	}
}

Board load_board(int server_socket){
	int number_of_bytes;
	uint16_t statistics;
	remaining_mines = NUM_MINES;	//Each time board is loaded, reset number of mines to intial value.
	Board board;
	board.game_state = 1;	//gamestate, 1 = netural, 0 = lose, 2 = win.
	bool flags_check = true;	//false = tried to place flag where there was no mine

	//Each cell of board is recieved one at a time. A version of the gameboard is then created on the client side. 
	//Only the revealed tiles are saved though to prevent givining the location of the mines to the client
	for (int i = 0; i < 9; i++) {
		for(int j =0;j<9;j++){
			number_of_bytes = recv(server_socket, &statistics,sizeof(uint16_t),0);
			if(number_of_bytes == 0){
				//Socket is close, so close client connection to socket

				printf("\nServer is close\n");
				close(server_socket);
			}

			board.tiles[j][i] = ntohs(statistics);

			if(board.tiles[j][i] == '*'){
				//If a player revealed a mine, a mine to recieved by the client and therefore the client lost
				board.game_state = 0;
			}else if(board.tiles[j][i] == '+'){
				//Else the player has successfully placed a flag, since flags can only be placed on mines, the number of mines is decreased by 1.
				remaining_mines--;
			}
		}
	}

	//Player wins - 10 flags have been placed on the board, that means all mines are tagged and the player has won the game.
	if(remaining_mines <= 0){
		board.game_state = 2;
	}

	//Player tried to place a flag where there was no mine. The test was previously sent to the client to confirm, these is the result of the test.
	// 'X' is just a flag to store the test, this is never revealed on the gameboard.
	for(int j =0;j<NUM_TILES_Y;j++){
		for(int i = 0;i<NUM_TILES_X;i++){
			if(previous_board.tiles[i][j] != board.tiles[i][j] && board.tiles[i][j] == 'X'){
				flags_check = false;
			}
		}
	}

	//Update the previous state of the board, for comparison purposes.
	previous_board = board;

	//Display the new game board.
	display_board(board);

	//If the player attempted to place a flag there was no mine return the error/
	if(flags_check == false){
		printf("\n\n No mines here\n");
	}

	return board;
}

bool check_input(char* input,Board board){
	//Checks the users input is a correct value and within the limits of the game board
	bool input_check = true;

	//Return immedietly as there are no inputs following Q when quitting the game
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

	//Covnvert users coordinates to integers
	int Y_Coords = (int) toupper(input[1]) - (int)'A';
	int X_Coords = (int) input[2] - (int)'0'-1;

	//If tile is already revealed
	if(input_check == true){
		if(board.tiles[X_Coords][Y_Coords] != 0 && board.tiles[X_Coords][Y_Coords] != 'X' ){
			input_check = false;
		}
	}

	return input_check;
}

char main_menu(void){
	//Display the main menu and recieve the users selection.

	printf("\n\n\t\tMain Menu");
	printf("\nPlease enter a selection:	\n<1> Play MineSweeper \n<2> Show leaderboard \n<3> Quit\n");

	//Variable for users inpit
	char input[2];
				
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

	//Print the leaderboard results.
	while(strcmp(results, "NULL\0") != 0){

		//Seperate into seperate cells
		char* pch = strtok(results,"\n");
		while(pch != NULL){
			printf("\n%s",pch);
			pch = strtok(NULL,"\t");
		}

		//printf("\n%s",strtok(results,"\t"));

		//printf("\n%s",results);

		results = Receive_Array_Int_Data(server_socket,ARRAY_SIZE);
	}
}


int play_game(int server_socket, bool login){
	//Main game loop

	//Users selection on the main menu
	char selection = '0';

	//Loop until user wishes to exit
	while(1){

		//Player has successfully logged in at some stage.
		if(login == true){

			//Display the main menu if the user has not seen the main menu yet.
			if(selection == '0'){
				selection = main_menu();
				char Selection[2];
				Selection[0] = (char)selection;

				//Send to server the users selection
				if(send(server_socket,Selection,2,0) == -1){
					//Socket is closed, so disconnect

					printf("\nSocket is closed\n");
					close(server_socket);
				}

					//Clear buffer
					while(getchar() != '\n');
			}

			//User has selected a value on the main menu.
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

				//Get the most recent gameboard
				Board board = load_board(server_socket);

				//Check the game state of the board.
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
					//Game is still going.

					//Display options for user
					printf("\nOptions:");
					printf("\n<R> Reveal tile");
					printf("\n<P> Place flag");
					printf("\n<Q> Quit game\n");
					
					//Specify input format for user.
					printf("\nPlease input coordinates (Option)(A-I)(1-9)\n");	

					//Recieve users input
					char input[5];
					fgets(input,4,stdin);

					

					//Input verifications - True == input was allowed
					bool input_check = check_input(input,board);

					//Loop until input valid.
					while(input_check == false){
						display_board(board);
						printf("\nInvalid input.");
						printf("\nPlease input coordinates (Option)(A-I)(1-9)\n");

						//Get users input
						fgets(input,4,stdin);

						input_check = check_input(input,board);
					}
					
					//Send selection to server
					if(send(server_socket,input,40,0) == -1){
						//Socket is closed, so disconnect

						printf("\nSocket is closed\n");
						close(server_socket);
					}

					
					//Quit game
					if(toupper(input[0]) == 'Q'){
						selection = '0';
					}
				}
			}

			//Clear input buffer
			while(getchar() != '\n');	

		}else{	//Player has not logged in yet.
			
			//Recieve login response
			char *results = Receive_Array_Int_Data(server_socket,  ARRAY_SIZE);
			
			if(strcmp(results, "Login Successful") == 0){
				// Login was successsfuly

				login = true;
				//Print out the array
				printf("\n%s",results);	
				free(results);

			}else if(strcmp(results, "You entered either an incorrect username or password. Disconnecting.\0") == 0){
				//Invalid login details

				//Print out the array
				printf("\n%s\n",results);

				close(server_socket);
			
				free(results);

				return 1;

			}else{
				//User has not attempted a login yet.

				//Print out login requests from server.
				printf("\n%s",results);
				free(results);

				//Get users input
				char input[100];
				fgets(input,100,stdin);

				//Send users input to server
				if(send(server_socket,input,40,0) == -1){
					//Socket is closed, so disconnect

					printf("\nSocket is closed\n");
					close(server_socket);
				}
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

	while(1){

			//Check if user has quit game, this state will be impossible before the first iteration.
			//The play game function is called as the input, the play game function is the game loop/
			if(play_game(server_socket,login) == 1){
				//User has quit game
				break;
			}

			//Print the leaderboard
			leaderboard(server_socket);
			printf("\n");

			//If the user got to this stage, they have succelly logged in.
			login = true;
		}
	

	//Close connection with server
	close(server_socket);
}

int main(int argc, char *argv[]) {

	//If specified portm use that instead of the defualt.
	if(argc >= 2){
		PORTNUMBER = htons(atoi(argv[1]));
	}


	//Connect to the server.
	server_connect();

	return 0;
}
