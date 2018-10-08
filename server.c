// Server Client

#include <arpa/inet.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>


#define RANDOM_NUMBER_SEED 42 // Sead for randomisation

#define MYPORT 12345    // the port users will be connecting to 
#define BACKLOG 10     // how many pending connections queue will hold
#define MAXDATASIZE 1000 

// Defining size of play area
#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10

//Define max transmit sizes
#define MAXLOGINDATA 100

// Global void pointer for pthreads
void *server_handle;
int num_requests;

//////////Structures//////////

// Define log in structure
typedef struct login login_t;
struct login {
    char *username;
    char *password;
};

// Define log in node structure
typedef struct node_login node_login_t;
struct node_login {
    login_t *login;
    node_login_t *next;
};

// Define leaderboard player structure
typedef struct player player_t;
struct player {
    char *username;
    time_t playtime;
    int won;
    int played;
};

// Define leaderboard node structure
typedef struct node_leaderboard node_leaderboard_t;
struct node_leaderboard {
    player_t *player;
    node_leaderboard_t *next;
};

//Data type for a single tile on the board, revealed is a char so the correct value can be displayed
typedef struct{
	int adjacent_mines;
	char revealed;
	bool is_mine;
	bool is_flag;
} Tile;

//Data type for the game state
typedef struct{
	int gameover;	
	time_t start_time;
	int remaing_mines;
	Tile tiles[NUM_TILES_X][NUM_TILES_Y];
} GameState;

//////////Globals//////////

GameState gamestate;
node_login_t *head_login;


//////////Thread Pooling////////
void* p_thread_create(void* arg) {
	printf("New thread created!");

}

//////////Leaderboard//////////

void node_add_leaderboard(node_leaderboard_t *head, player_t *player){
	//Will be inserted after head,Adds player between two nodes.

	node_leaderboard_t *new_addition = (node_leaderboard_t*) malloc(sizeof(node_leaderboard_t));

	new_addition->player = player;
	new_addition->next = head->next;

	head->next = new_addition;
}

void leaderboard_sort(node_leaderboard_t *head, player_t *player_new){
	//With the new player to add, find their appropriate location in 
	//the current leaderboard linked list

	//Find in list where player_new time is less than position in lists time value
	while(player_new->playtime < head->next->player->playtime){
		head = head->next;

		//Check if next node is same time as player to insert,
		//if it is organise by games won
		if(player_new->playtime == head->next->player->playtime){

			//Find in list where player_new won is greater than position in lists won value
			while(player_new->playtime == head->next->player->playtime \
				&& player_new->won > head->next->player->won){
				head = head->next;

				//Check if next node is same games won as player to insert,
				//if it is organise by alphabetical order
				if(player_new->won == head->next->player->won){

					//Find in list where player_new username is compared alphabetically than position in lists username
					while(player_new->playtime == head->next->player->playtime \
					&& player_new->won == head->next->player->won\
					&& strcmp(player_new->username,head->next->player->username) < 0){
						head = head->next;
					}
				}
			}
		}
		
		//If the next position is the last item in the list break the loop
		//Last item is filer data, therefore can be found where games won is -1.
		if(head->next->player->won == -1){
			break;
		}
	}

	//Place player_new in node after position just found above.
	node_add_leaderboard(head,player_new);	
}

void print_leaderboard(node_leaderboard_t *head){
	//Print each node in leaderboard, besides filler data.

	//Print titles
	printf("\nUsername\t Time\t Games Won\t Games Played");

	while(head != NULL & head->player->won != -1){
		printf("\n%s\t %ld\t %d\t %d", head->player->username,head->player->playtime,head->player->won,head->player->played);
		head = head->next;
	}
}

//////////Creating gameboard//////////

bool tile_contains_mine(int x,int y){
	//Check if tile in the x,y positon contains a mine

	if(gamestate.tiles[x][y].is_mine == true){
		return true;
	}
	return false;
}

void place_mines(void){
	//Place mines on the gameboard, locations are randomly generated.
	// Adjacent mine values are also set when placing the mines.

	for(int i = 0;i<NUM_MINES;i++){
		int x,y;
		do{
			x = rand() % NUM_TILES_X;
			y = rand() % NUM_TILES_Y;
		}while(tile_contains_mine(x,y));

		//For surrounding tiles increase adjacent mines by 1
		//Do this first so the adjacent tiles for the mine square can be replaced with -1
		for(int i = 0;i<3;i++){		
			for(int j = 0;j<3;j++){

				int Adjacent_X = x+j-1;
				int Adjacent_Y = y+i-1;

				if((Adjacent_X >= 0 && Adjacent_X < NUM_TILES_X) && (Adjacent_Y >= 0 && Adjacent_Y < NUM_TILES_Y)){
					gamestate.tiles[Adjacent_X][Adjacent_Y].adjacent_mines++;
				}				
			}
		}

		//Change specific tile to a mine
		gamestate.tiles[x][y].is_mine = true;
		gamestate.tiles[x][y].adjacent_mines = -1;	//Set to -1, as adjacent mines dosnt make sense here
	}
}	

////////// Load authentication file//////////

node_login_t * node_add_login(node_login_t *head, login_t *login){
	//Add usernames and passwords to linked list

	//Create the new node
	node_login_t *new = (node_login_t*) malloc(sizeof(node_login_t));
	if(new == NULL){
		return NULL;
	}

	//Add data
	new->login = login;
	new->next = head;

	return new;
}

node_login_t* load_auth(void){ 
	//Read authentication file and process contents

	//Open Authentication file.
	FILE* Auth = fopen("Authentication.txt","r");

	//Variables
	char data[1000];

	//Linked list
	node_login_t *add_data;
	login_t *add_login;
	node_login_t *data_list = NULL;

	//Read single line of document at a time.
	while(fgets(data,1000,Auth) != NULL){

		//Seperate usernames and passwords into seperate strings
		char *username = NULL;
		char *password = NULL;
		char *ptr = strtok(data," 	");

		//Save username and password
		while(ptr != NULL){
			if(username == NULL){
				username = ptr;
			}else{
				password = ptr;
				ptr = NULL;
			}
			ptr = strtok(NULL," 	");
		}
	
		//Dont add username and password titles as a valid login
		if(strcmp("Username",username) != 0){
			login_t *add_login = (login_t*)malloc(sizeof(login_t));

			//Add \0 to end of username and passwords
			username[strlen(username)] = '\0';
			password[strlen(password)] = '\0';

			//Populate add_login with data.
			add_login->username = strdup(username);
			add_login->password = strdup(password);

			//Add add_login to linked list
			node_login_t *newhead = node_add_login(data_list,add_login);

			//Error checking
			if (newhead == NULL) {
	            printf("memory allocation failure\n");
	        }

	        //Make new head of linked list.
	        data_list = newhead;
    	}
	}

	//Close connection with file.
	fclose(Auth);

	//Return head of list
	return(data_list);
}

// Check inputted username and password against lists //
int check_login(char* username,char*  password){
	//Checks the users inputs for password and username against 
	//linked list/Authentication.txt. 

	//Remove end of line char from user input for username,
	//Not required for password as \0 is included at the ends of the passwords in the linked list.
	username[strcspn(username, "\r\n")] = 0;

	for( ; head_login != NULL; head_login = head_login->next){
		//Check if username exists
		if(strcmp(username, head_login->login->username) == 0){
			//If the username is in list, check if cooresponding password matches input.
			if(strcmp(password,head_login->login->password) == 0){
				//Password matches and should login
				return 2;
			}else{
				//Password is incorrect
				return 1;
			}
		}
	}

	//Username is not in list.
	return 0;
}

////////// Gameplay elemnts//////////

void reveal_selected_tile(int x,int y){
	//For the specific square make is reveled the equivelent char.

	char revealed;

	if(gamestate.tiles[x][y].is_flag == true){
		revealed = '+';
	}else if(gamestate.tiles[x][y].is_mine == true){
		revealed = '*';
	}else{
		//Add 0, to make the adjacent_mines the correct ascii character
		revealed = gamestate.tiles[x][y].adjacent_mines + '0';
	}

	gamestate.tiles[x][y].revealed = revealed;
}

//might be broken
void open_surrounding_tiles(int x, int y){
	//Open us the surrounding tiles from user 
	//selection where adjacent_mines == 0 until the tiles where
	//adjacent_mines > 0 if tile selected by user has adjacent_mines 
	//value of 0.
	
	//For surrounding tiles from user specified tile.
	for(int i = 0;i<3;i++){	
		for(int j = 0;j<3;j++){	
			
			int Adjacent_X = x+j-1;
			int Adjacent_Y = y+i-1;

			//Checks that the Adjacent values are withing the game limits
			if((Adjacent_X >= 0 && Adjacent_X < NUM_TILES_X) && (Adjacent_Y >= 0 && Adjacent_Y < NUM_TILES_Y)){
				
				//Checks if it has 0 adjacent mines and is not revealed yet.
				if(gamestate.tiles[Adjacent_X][Adjacent_Y].adjacent_mines == 0 && gamestate.tiles[Adjacent_X][Adjacent_Y].revealed == false){
					
					//Displays the tile, then calls the same function, 
					//to search the surrounding tiles of that sqaure for adjacent_mines == 0;
					reveal_selected_tile(Adjacent_X,Adjacent_Y);
					open_surrounding_tiles(Adjacent_X,Adjacent_Y);
				}else{
					//Display one square, next to a 0.

					//dosent work, if you click and edge of the 0s, e.g. (4,6)
					reveal_selected_tile(Adjacent_X,Adjacent_Y);
				}
			}
		}
	}
}

int user_input(int x, int y){
	//Recieves the users input and reveals the selected tile.

	//0 == mine was selected and player has lost
	//1 == tile selected was ok 

	//So the user inputted numbers line up, with the correct coords
	x--;
	y--;

	//Reveal the selected tile
	//gamestate.tiles[x][y].revealed = true;
	reveal_selected_tile(x,y);

	if(gamestate.tiles[x][y].is_mine == true){
		//Selected tile was a mine, change all tiles with mines is_revealed to true
		for(int i = 0;i<NUM_TILES_Y;i++){		//Y values
			for(int j = 0;j<NUM_TILES_X;j++){		//X values
				if(gamestate.tiles[j][i].is_mine == true){
					reveal_selected_tile(j,i);
				}
			}
		}
		return 0;

	}else if(gamestate.tiles[x][y].adjacent_mines >0){
		//If the adjacent_mines number is not 0, only make the sqaure selected visable
		return 1;

	}else{
		//If the adjacent_mines number is 0, 
		//reveal surrounding zeros till adjacent_mines is > 0

		open_surrounding_tiles(x,y);
		return 1;
	}
}

void place_flags(int x, int y){
	//Place a flag at the user specified. Decrement remaining_mines counter by 1.
	//Flag cannot be placed on tile which is not a mine
	
	//So the user inputted numbers line up, with the correct coords
	x--;
	y--;

	if(gamestate.tiles[x][y].is_mine == true){
		gamestate.tiles[x][y].is_flag = true;
		reveal_selected_tile(x,y);
		gamestate.remaing_mines--;
	}else{
		printf("No mine here");
	}
}

void display_board(void){
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
			printf(" %c", gamestate.tiles[i][j].revealed);
		}
	}
}

////////// Server & Client connection//////////

void Send_Array_Data(int socket_id, char *text) {
	uint16_t statistics;  
	for (int i = 0; i < 150; i++) {
		statistics = htons(text[i]);
		send(socket_id, &statistics, sizeof(uint16_t), 0);
	}
}

void client_play(int socket_id){
	bool exit = false;
	char buffer[MAXDATASIZE];

	printf("\nSending\n");

	//While user does not want to stop playing the game
	while(!exit){
		for(int i = 0;i<MAXDATASIZE;i++){
			buffer[i] = ' ';
		}

		//Populate array with the gameboard
		for(int i = 0;i<NUM_TILES_X;i++){
			for(int j = 0;j<NUM_TILES_Y;j++){
				buffer[j+i*10] = gamestate.tiles[i][j].revealed; 
			}
		}

		Send_Array_Data(socket_id,buffer);

		recv(socket_id,buffer,MAXDATASIZE,0);

		printf("\n%s\n",buffer);
	}
}

void client_login(int client_socket){
	char username[MAXLOGINDATA];	//Store username from input
	char password[MAXLOGINDATA];	//Store password from input

	//Login
	Send_Array_Data(client_socket,"Welcome to the online Minesweeper gaming system.\nYou are required to log on with your registered username and password\n\nUsername: ");
	recv(client_socket, username, MAXLOGINDATA, 0);

	Send_Array_Data(client_socket,"Password: ");
	recv(client_socket, password, MAXLOGINDATA, 0);

	if(check_login(username,password) == 2){
		//User has successfully logged in
		Send_Array_Data(client_socket,"Login Successful");
		client_play(client_socket);
	}else{
		//User has entered invalid login details
		Send_Array_Data(client_socket,"You entered either an incorrect username or password. Disconnecting.");
		close(client_socket);
		exit(0);
	}
}

int server_setup ( void ){
	
	int error, exit_check = 0, *newsocket, client_socket;
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t sin_size;

	int server_socket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = Internet Protocol v4 Addresses (Family), SOCK_STREAM = TCP, 0 = protocol default

	// Start/define network services
	printf("Server starts networking services.\n");

	struct sockaddr_in server_address; // address of server
	server_address.sin_family = AF_INET; // Define protocol
	server_address.sin_port = htons(MYPORT); // Use defined port
	server_address.sin_addr.s_addr = INADDR_ANY; // Use any IP address on local machine IE 0.0.0.0 
	
	// Bind the socket to the IP and PORT specified

	do{
		error = bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

		//Only sleep if there is an error, if it binds a socket straight away, dont sleep.
		if(error == -1){
			printf("\nUnable to bind socket, will try again in 5 seconds");
			sleep(5);
			exit_check++;
		}

		//If after 50 seconds, the socket cannot be bound, 
		//exit attempting to create the socket and print an error message.
		if(exit_check > 10){
			printf("\nSocket could no be bound, exiting.\n");
			return 0;
		}
	}while(error == -1);

	// Start listening for connections
	listen(server_socket, BACKLOG);
	printf("\nServer is waiting for log in request...\n");


	// Client connection // NULL and NULL would be filed with STRUC if you want to know where the client is connecting from etc
	while ((client_socket = accept(server_socket, (struct sockaddr *)&their_addr, &sin_size))){




		// add_request(requestcount, &request_mutex, &got request, client_socket);
		//pthread_t server_thread;
		// newsocket = malloc(sizeof(client_socket));
		// *newsocket = client_socket;
		//pthread_create(&server_thread, NULL, server_handle, (void*) newsocket);
		// I've tried with joining the thread and exiting it and not having this line of code at all.
		// pthread_join(server_thread,NULL);

	}

	
	if ( client_socket == -1 ){
		printf("\nClient Unable to connect.");
	}

	client_login(client_socket);

	//Close socket connection
	close(client_socket);
	exit(0);
}

////////// Main //////////

int main ( void ){	
	// /////////////Testing functions work/////////////
	// srand(RANDOM_NUMBER_SEED);	//See random number generator

	// gamestate.gameover = 1;		//Initilise at 1, as game is not won or lost yet
	// gamestate.start_time = time(0);
	// gamestate.remaing_mines = NUM_MINES;

	// //Placing mines, then print for confimation
	// place_mines();

	// user_input(2,1);
	// place_flags(1,1);
	// place_flags(2,2);

	// display_board();
	// printf("\n"); 


	int thread_ID[BACKLOG]; // Thread ID array size of BACKLOG
	pthread_t thread_data_ID[BACKLOG];	// Thread ID array for pthread data type size of BACKLOG

	//Place all usernames and passwords in a linked list, loads from text file
	head_login = load_auth();
	printf("\nServer started.\n");
	printf("\nUsernames in linked lists from text file.\n");

	while(1){
		// Create Pthread pool of 10 (size of BACKLOG)
		for (int i = 0; i < BACKLOG; i++){
			thread_ID[i] = i;
			pthread_create(&thread_data_ID[i], NULL, p_thread_create, (void*) &thread_ID[i]);

		}

		server_setup();

	}








	// Re-join Threads
	for (int i = 0; i < BACKLOG; i++){
		pthread_join(thread_data_ID[i], NULL);
	}

	return 0;
}