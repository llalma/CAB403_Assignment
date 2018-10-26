// Server Client
#define _GNU_SOURCE
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
#include <ctype.h>
#include <signal.h> 


#define RANDOM_NUMBER_SEED 42 // Seed for randomisation
#define NUM_THREADS 10     // how many pending connections queue will hold
#define MAXDATASIZE 1000 

// Defining size of play area
#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 1

//Define max transmit sizes
#define MAXLOGINDATA 100

pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* global condition variable for our program. assignment initializes it. */
pthread_cond_t  got_request   = PTHREAD_COND_INITIALIZER;

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

// Data type for a single tile on the board, revealed is a char so the correct value can be displayed
typedef struct{
	int adjacent_mines;
	char revealed;
	bool is_mine;
	bool is_flag;
} Tile;

// Data type for the game state
typedef struct{
	int gameover;	
	bool game_won;
	int remaing_mines;
	Tile tiles[NUM_TILES_X][NUM_TILES_Y];
	char *username;
	int won;
	int played;
} GameState[20];  // Create GameState structure array size of 20


// Data Structure for thread queue
struct request {
    int number;             // number of the request                 
    struct request* next;   // pointer to next request, NULL if none. 
	int client_socket;
};

// linked lists head for request and last request for tracking
struct request* last_request = NULL; 
struct request* requests = NULL;

//////////Globals//////////
GameState gamestate;
node_login_t *head_login;
//char * UserName; // 
node_leaderboard_t *head_leaderboard;
int server_socket; // Server socket global
volatile bool server_running = true; // Server running state
int MYPORT = 12345;    // The default port if no port is specified
int port_ID_pos;


// Global void pointer for pthreads
void *server_handle;
int num_requests = 0; // Number of requests


// Define required functions to avoid ordering errors.
void client_login();
void handle_request();

//////////Send data to client//////////
void Send_Array_Data(int socket_id, char *text) {
	//Send data to the client
	uint16_t statistics;  
	for (int i = 0; i < 150; i++) {
		statistics = htons(text[i]);
		send(socket_id, &statistics, sizeof(uint16_t), 0);
	}
}

void Send_Board(int socket_id) {
	//Send each cell of the gameboard, one at a time.

	uint16_t statistics;  
	for (int i = 0; i < 9; i++) {
		for(int j =0;j<9;j++){
			statistics = htons(gamestate[socket_id-port_ID_pos].tiles[j][i].revealed);
			send(socket_id, &statistics, sizeof(uint16_t), 0);
		}
	}
}

//////////Thread Pooling + Request management + GamePlay setup////////
void add_request(int request_num, pthread_mutex_t* p_mutex, pthread_cond_t* p_cond_var, int client_socket){
	int return_code; // Return code from pthread function
	struct request* a_request; // Pointer to newly added request

	// Allocate memory the size of the structure request
	a_request = (struct request*)malloc(sizeof(struct request));
	a_request->number = request_num;
	a_request->next = NULL;
	a_request->client_socket = client_socket;

	// Lock thread 
	return_code = pthread_mutex_lock(p_mutex); 

	// Add new request to the end of the list and update the list.
	// This accounts for if the list is empty
    if (num_requests == 0) { /* special case - list is empty */
        requests = a_request;
        last_request = a_request;
    }
    else {
        last_request->next = a_request;
        last_request = a_request;
    }

	// increase the number of pending requests by one
	num_requests++;

	// Unlock thread
	return_code = pthread_mutex_unlock(p_mutex);

	// signal the condition variable - there's a new request to handle
	return_code = pthread_cond_signal(p_cond_var);
}

struct request* get_request(pthread_mutex_t* p_mutex){
	int return_code; // Return code from pthread function
	struct request* a_request; // Pointer to request

	// Lock thread
	return_code = pthread_mutex_lock(p_mutex);

    if (num_requests > 0) { // explain...
        a_request = requests;
        requests = a_request->next;
        if (requests == NULL) { // last request on the list 
            last_request = NULL;
        }
       
        num_requests--; // reduce the number of requests by one
    }
    else { // error handling if list 
        a_request = NULL;
    }

	// Unlock the thread
	return_code = pthread_mutex_unlock(p_mutex);
	
	// return the request
	return a_request; 
}

void* p_thread_create(void* arg) {
	int return_code; // Return code from pthread function
	struct request* a_request; // Pointer to a request
	int thread_id = *((int*)arg); // Thread identifying No.

	// Lock thread
	return_code = pthread_mutex_lock(&request_mutex);
	
	while(1) { // Loop forever 
		if (num_requests > 0) { // If there is a queued request
			// Get request from client
			a_request = get_request(&request_mutex);

			if (a_request) { // Process request
				// Unlock Mutex
				return_code = pthread_mutex_unlock(&request_mutex);
				// Send request to function for processing 
				handle_request(a_request, thread_id);
				// Free pointer and deallocate memory
				free(a_request);
				// Lock Mutex
				return_code = pthread_mutex_lock(&request_mutex);
			}
		}
		else {
			return_code = pthread_cond_wait(&got_request, &request_mutex);
			// Wait for request to arrive and Mutex will be unlocked
			// to allow all other threads to access the requests linked list

		}
	}
}

void handle_request(struct request* a_request, int thread_id){
    if (a_request) { // If there is a request to log in.
        printf("Thread '%d' handled request '%d'\n",
               thread_id, a_request->number); // Printing the thread ID and request number
		if (port_ID_pos == 0){ // Assisting with positioning the data in the order in the Gamestate struc array.
			port_ID_pos = a_request->client_socket;
		}
        client_login(a_request->client_socket); // Run the login in screen and if passed, the game will procced.
        fflush(stdout);
    }	
}

//////////Leaderboard//////////
node_leaderboard_t* fill_leaderboard(node_leaderboard_t *head, player_t *player){
	//Fill leaderboard with filler data

	//New node to add to leaderboard
	node_leaderboard_t *leaderboard_addition = (node_leaderboard_t*) malloc(sizeof(node_leaderboard_t));

	//Check creation
	if(leaderboard_addition == NULL){
		return NULL;
	}

	//Add data to new node
	leaderboard_addition->player = player;
	leaderboard_addition->next = head;

	//Return new head of list
	return leaderboard_addition;
}

void node_add_leaderboard(node_leaderboard_t *head, player_t *player){
	//Add new node to leaderboard, will insert new node immedietly after node given as head of the function.
	//The new node will always be inserted between 2 nodes.

	//New node to add
	node_leaderboard_t *new_addition = (node_leaderboard_t*) malloc(sizeof(node_leaderboard_t));

	//Populate new node with data
	new_addition->player = player;
	new_addition->next = head->next;

	//Point head to new node just created.
	head->next = new_addition;

}

void leaderboard_sort(node_leaderboard_t *head, player_t *player_new){
	//With the new player to add, find the appropriate location in 
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

void print_leaderboard(node_leaderboard_t *head,int socket_id){
	//Send each node in leaderboard, besides filler data.

	//Send titles
	Send_Array_Data(socket_id,"\nUsername\t Time (Seconds)\t Games Won\t Games Played");

	//First item will always have a won value of -1, as it is filler data, therefore skip it.
	head = head->next;

	//While there is a row in the keaderboard which is no filler data send it, row by row.
	while(head != NULL & head->player->won != -1){
		char row[50];
		snprintf(row,sizeof(row),"\n%s\t\t\t %ld\t %d\t\t %d", head->player->username,head->player->playtime,head->player->won,head->player->played);

		Send_Array_Data(socket_id,row);
		head = head->next;
	}

	//Signify end of leaderboard
	Send_Array_Data(socket_id,"NULL");
}


void leaderboard_setup(){
	pthread_mutex_lock(&request_mutex); // Lock mutex for thread exclusive access of data
	//Blank leaderboard initially
	head_leaderboard = NULL;
	//Filler data for 1st and last node
	player_t *player_new = (player_t*)malloc(sizeof(player_t));
	//Populate new player with data
	player_new->username = NULL;
	player_new->playtime = time(0) - time(0);
	player_new->won = -1;
	player_new->played = -1;
	//Add two bits of filler data in leaderboard
	head_leaderboard = fill_leaderboard(head_leaderboard,player_new);
	head_leaderboard = fill_leaderboard(head_leaderboard,player_new);
	pthread_mutex_unlock(&request_mutex); // UnLock mutex 
}

//////////Creating gameboard//////////
bool tile_contains_mine(int x,int y, int socket_id){
	//Check if tile in the x,y positon contains a mine
	if(gamestate[socket_id-port_ID_pos].tiles[x][y].is_mine == true){
		return true;
	}
	return false;
}

void place_mines(int socket_id){
	//Place mines on the gameboard, locations are randomly generated.
	// Adjacent mine values are also set when placing the mines.

	for(int i = 0;i<NUM_MINES;i++){
		int x,y;

		do{
			x = rand() % NUM_TILES_X;
			y = rand() % NUM_TILES_Y;
		}while(tile_contains_mine(x,y, socket_id));


		//For surrounding tiles increase adjacent mines by 1
		//Do this first so the adjacent tiles for the mine square can be replaced with -1
		for(int i = 0;i<3;i++){		
			for(int j = 0;j<3;j++){

				int Adjacent_X = x+j-1;
				int Adjacent_Y = y+i-1;

				if((Adjacent_X >= 0 && Adjacent_X < NUM_TILES_X) && (Adjacent_Y >= 0 && Adjacent_Y < NUM_TILES_Y)){
					gamestate[socket_id-port_ID_pos].tiles[Adjacent_X][Adjacent_Y].adjacent_mines++;
				}				
			}
		}
		//Change specific tile to a mine
		gamestate[socket_id-port_ID_pos].tiles[x][y].is_mine = true;
		gamestate[socket_id-port_ID_pos].tiles[x][y].adjacent_mines = -1;	//Set to -1, as adjacent mines dosnt make sense here
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
int check_login(char* username,char*  password, int socket_id){
	//Checks the users inputs for password and username against 
	//linked list/Authentication.txt. 

	//Remove end of line char from user input for username,
	//Not required for password as \0 is included at the ends of the passwords in the linked list.
	username[strcspn(username, "\r\n")] = 0;
	node_login_t *head = head_login;

	for( ; head != NULL; head = head->next){
		//Check if username exists
		if(strcmp(username, head->login->username) == 0){
			//If the username is in list, check if cooresponding password matches input.
			if(strcmp(password,head->login->password) == 0){
				//Password matches and should login\

				//Store data about logged in player in gamestate
				gamestate[socket_id-port_ID_pos].username = username;
				gamestate[socket_id-port_ID_pos].won = 0;
				gamestate[socket_id-port_ID_pos].played = 0;
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

void print_login_list(node_login_t *head){


	//While there is a row in the keaderboard which is no filler data send it, row by row.
	while(head != NULL){
		//Send_Array_Data(socket_id,row);
		printf("\n%s", head->login->username);
		head = head->next;
	}

}

////////// Gameplay elemnts//////////
void reveal_selected_tile(int x,int y, int socket_id){
	//For the specific square make is reveled the equivelent char.

	char revealed;

	if(gamestate[socket_id-port_ID_pos].tiles[x][y].is_flag == true){
		revealed = '+';
	}else if(gamestate[socket_id-port_ID_pos].tiles[x][y].is_mine == true){
		revealed = '*';
	}else{
		//Add 0, to make the adjacent_mines the correct ascii character
		revealed = gamestate[socket_id-port_ID_pos].tiles[x][y].adjacent_mines + '0';
	}

	gamestate[socket_id-port_ID_pos].tiles[x][y].revealed = revealed;
}

void open_surrounding_tiles(int x, int y, int socket_id){
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
				if(gamestate[socket_id-port_ID_pos].tiles[Adjacent_X][Adjacent_Y].adjacent_mines == 0 && gamestate[socket_id-port_ID_pos].tiles[Adjacent_X][Adjacent_Y].revealed == false){
					
					//Displays the tile, then calls the same function, 
					//to search the surrounding tiles of that sqaure for adjacent_mines == 0;
					reveal_selected_tile(Adjacent_X,Adjacent_Y, socket_id);
					open_surrounding_tiles(Adjacent_X,Adjacent_Y, socket_id);
				}else{
					//Display one square, next to a 0.
					reveal_selected_tile(Adjacent_X,Adjacent_Y, socket_id);
				}
			}
		}
	}
}

int user_input(int x, int y, int socket_id){
	//Recieves the users input and reveals the selected tile.

	//0 == mine was selected and player has lost
	//1 == tile selected was ok 

	//So the user inputted numbers line up, with the correct coords
	x--;
	y--;

	//Reveal the selected tile
	reveal_selected_tile(x,y, socket_id);

	if(gamestate[socket_id-port_ID_pos].tiles[x][y].is_mine == true){
		//Selected tile was a mine, change all tiles with mines is_revealed to true
		for(int i = 0;i<NUM_TILES_Y;i++){		//Y values
			for(int j = 0;j<NUM_TILES_X;j++){		//X values
				if(gamestate[socket_id-port_ID_pos].tiles[j][i].is_mine == true){
					reveal_selected_tile(j,i, socket_id);
				}
			}
		}
		return 0;

	}else if(gamestate[socket_id-port_ID_pos].tiles[x][y].adjacent_mines >0){
		//If the adjacent_mines number is not 0, only make the sqaure selected visable
		return 1;

	}else{
		//If the adjacent_mines number is 0, 
		//reveal surrounding zeros till adjacent_mines is > 0

		open_surrounding_tiles(x,y, socket_id);
		return 1;
	}
}

void place_flags(int x, int y, int socket_id){
	//Place a flag at the user specified. Decrement remaining_mines counter by 1.
	//Flag cannot be placed on tile which is not a mine
	
	//So the user inputted numbers line up, with the correct coords
	x--;
	y--;

	if(gamestate[socket_id-port_ID_pos].tiles[x][y].is_mine == true){
		gamestate[socket_id-port_ID_pos].tiles[x][y].is_flag = true;
		reveal_selected_tile(x,y, socket_id);
		gamestate[socket_id-port_ID_pos].remaing_mines--;
	}else{
		//User tried to place flag where there was no mine
		gamestate[socket_id-port_ID_pos].tiles[x][y].revealed = 'X';
	}
}

void restart_game(int socket_id){
	//Reset variables for the game

	for(int j = 0;j<NUM_TILES_Y;j++){
		for(int i = 0;i<NUM_TILES_X;i++){
			gamestate[socket_id-port_ID_pos].tiles[i][j].is_flag = false;
			gamestate[socket_id-port_ID_pos].tiles[i][j].is_mine = false;
			gamestate[socket_id-port_ID_pos].tiles[i][j].revealed = 0;
			gamestate[socket_id-port_ID_pos].tiles[i][j].adjacent_mines = 0;

		}
	}

	//Placing mines
	place_mines(socket_id);
	gamestate[socket_id-port_ID_pos].game_won = NULL;
	gamestate[socket_id-port_ID_pos].gameover = 1;
	gamestate[socket_id-port_ID_pos].remaing_mines = NUM_MINES;
}

////////// Server & Client connection//////////
bool client_play(int socket_id){
	//Game loop

	bool exit = false;
	char buffer[MAXDATASIZE];
	char *row;
	char input[5];
	time_t start_time = time(0);

	//While user does not want to stop playing the game
	while(!exit){

		//Send the gameboard state
		Send_Board(socket_id);

		//Recieve the user selection, place flag reveal square or quit.
		recv(socket_id,input,MAXDATASIZE,0);

		//Covnvert users coordinates to integers
		int Y_Coords = (int) toupper(input[1]) - (int)'A'+1;
		int X_Coords = (int) input[2] - (int)'0';

		//Users selection -checking 0th position as that is where the user sepecifies the task
		if(toupper(input[0]) == 'R'){
			//Reveal tile
			if(user_input(X_Coords,Y_Coords, socket_id) == 0){
				//Player has selected a mine and lost
				Send_Board(socket_id);
				exit = true;
				gamestate[socket_id-port_ID_pos].game_won = false;
				//Add 1 to games player for player
				gamestate[socket_id-port_ID_pos].played++;
			}

		}else if(toupper(input[0]) == 'P'){
			//Place flag
			place_flags(X_Coords,Y_Coords, socket_id);

			//Check end game win condition.
			if(gamestate[socket_id-port_ID_pos].remaing_mines <= 0){
				Send_Board(socket_id);
				exit = true;
				gamestate[socket_id-port_ID_pos].game_won = true;

				//Add 1 games won for player
				gamestate[socket_id-port_ID_pos].won++;
				//Add 1 to games player for player
				gamestate[socket_id-port_ID_pos].played++;
			}
		}else if(toupper(input[0]) == 'Q'){
			//Quit
			exit = true;
			gamestate[socket_id-port_ID_pos].game_won = false;
			//Add 1 to games player for player
			gamestate[socket_id-port_ID_pos].played++;
			break;
		}
	}

	if(gamestate[socket_id-port_ID_pos].game_won == true){
		pthread_mutex_lock(&request_mutex); // Lock mutex for thread exclusive access of data

		//Acutal player to add
		player_t *player_new2 = (player_t*)malloc(sizeof(player_t));
		//Populate new player with data
		player_new2->username = gamestate[socket_id-port_ID_pos].username;
		player_new2->playtime = time(0) - start_time;
		player_new2->won = gamestate[socket_id-port_ID_pos].won;
		player_new2->played = gamestate[socket_id-port_ID_pos].played;
		leaderboard_sort(head_leaderboard,player_new2);
		pthread_mutex_unlock(&request_mutex);// Unlock mutex
		
	}
	if(toupper(input[0]) != 'Q'){
		print_leaderboard(head_leaderboard,socket_id);
	}
	return false;
}

void client_login(int client_socket){
	//Get user to attempt a login
	char username[MAXLOGINDATA];	//Store username from input
	char password[MAXLOGINDATA];	//Store password from input

	//Send login message
	Send_Array_Data(client_socket,"Welcome to the online Minesweeper gaming system.\nYou are required to log on with your registered username and password\n\nUsername: ");
	//Recieve users username input
	recv(client_socket, username, MAXLOGINDATA, 0);

	//Send password login message				pthread_kill(thread_data_ID)
	Send_Array_Data(client_socket,"Password: ");
	//Recieve users password input
	recv(client_socket, password, MAXLOGINDATA, 0);

	//Check login information
	if(check_login(username,password, client_socket) == 2){

		//User has successfully logged in
		Send_Array_Data(client_socket,"Login Successful");

		bool new_game = false;
		while(!new_game){
			//Loop while user has not disconnected from server.

			//Users selection from main menu
			char selection[2];
			recv(client_socket, selection, 2, 0);

			if(toupper(selection[0]) == '3'){
				//User wishes to exit
				new_game = true;

			}else if(toupper(selection[0]) == '2'){
				//User wishes to see leaderboard
				print_leaderboard(head_leaderboard,client_socket);
			}else{
				//User wishes to play game
				restart_game(client_socket);

				//Begin a new game
				new_game = client_play(client_socket);
			}
		}		

	}else{
		//User has entered invalid login details
		Send_Array_Data(client_socket,"You entered either an incorrect username or password. Disconnecting.");

	}
}

void thread_join( pthread_t *thread_data_ID ){
	for (int i = 0; i < NUM_THREADS; i++){
		pthread_join(thread_data_ID[i], NULL);
	}
}

int server_setup ( int request_count){
	// Setup the server
	int error, exit_check = 0, *newsocket, client_socket;
	struct sockaddr_in client_addr; /* connector's address information */
	socklen_t sin_size;

	server_socket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = Internet Protocol v4 Addresses (Family), SOCK_STREAM = TCP, 0 = protocol default

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

		//If after 100 seconds, the socket cannot be bound, 
		//exit attempting to create the socket and print an error message.
		if(exit_check > 20){
			printf("\nSocket could not be bound, exiting.\n");
			return 0;
		}
	}while(error == -1);

		// Start listening for connections
		listen(server_socket, NUM_THREADS);
		printf("\nServer is waiting for log in request...\n");

	// Client connection // NULL and NULL would be filed with STRUC if you want to know where the client is connecting from etc
	while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &sin_size))){
		request_count++;
		printf("Server connected to a client");
		add_request(request_count, &request_mutex, &got_request, client_socket);
		//Set up the leaderboard
		leaderboard_setup(request_count, client_socket);
	}
	
	// Error handling for a connection failure
	if ( client_socket == -1 ){
		printf("\nClient Unable to connect.");
	}

	//Close socket connection
	printf("\nSocket to client closed");
	close(client_socket);
	exit(0);
}

//////////exit catch/////////
void ctrl_C_handler(int sig_num) { 
    signal(SIGINT, ctrl_C_handler);
    printf("\n Server terminated! \n"); 
	server_running = false;
	close(server_socket);
	kill(0,3);
	} 

////////// Main //////////
int main ( int argc, char *argv[] ){	
	// IF specified PORT inputed by user use that instead of the default.
	if(argc >= 2){
		MYPORT = htons(atoi(argv[1]));
	}
	int thread_ID[NUM_THREADS], request_count = 0; // Thread ID array size of NUM_THREADS
	srand(RANDOM_NUMBER_SEED);	//See random number generator
	pthread_t thread_data_ID[NUM_THREADS];	// Thread ID array for pthread data type size of NUM_THREADS
	
	//Place all usernames and passwords in a linked list, loads from text file
	head_login = load_auth();
	printf("\nServer started.\n");
	printf("\nUsernames in linked lists from text file.\n");

	// Setup leaderboard for first time
	leaderboard_setup();

	// Check if server is closed with Ctrl+C
	signal(SIGINT, ctrl_C_handler);
	while(server_running = true){

		// Create Pthread pool of 10 (size of NUM_THREADS)
		for (int i = 0; i < NUM_THREADS; i++){
			thread_ID[i] = i;
			pthread_create(&thread_data_ID[i], NULL, p_thread_create, (void*) &thread_ID[i]);
		}
		// Listen and bind client sockets in server_setup - 
		// This also begins the game for the user running it in a thread.
		server_setup(request_count);
	}

	// Clean up //
	// Re-join Threads
	thread_join(thread_data_ID);
	close(server_socket);

}