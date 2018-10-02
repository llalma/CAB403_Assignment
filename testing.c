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

#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10

#define RANDOM_NUMBER_SEED 42

#define MYPORT 12345    /* the port users will be connecting to */
#define BACKLOG 10     /* how many pending connections queue will hold */
#define MAXDATASIZE 1000

#define PORTNUMBER 12345
#define ARRAY_SIZE 50

//Data type for a single tile on the board
typedef struct{
	int adjacent_mines;
	bool revealed;
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

//////////////Login linked list///////////////////////
typedef struct login login_t;

struct login {
    char *username;
    char *password;
};

//A node in a linked list of usernames and passwords
typedef struct node_login node_login_t;

struct node_login {
    login_t *login;
    node_login_t *next;
};

///////////Leaderboard Linked list//////////////
typedef struct player player_t;

struct player {
    char *username;
    time_t playtime;
    int won;
    int played;
};

//A node in a linked list of usernames and passwords
typedef struct node_leaderboard node_leaderboard_t;

struct node_leaderboard {
    player_t *player;
    node_leaderboard_t *next;
};

//malloc this?
GameState gamestate;	//not sure if this was meant to be a global

//Recieve array data from client
// int *Receive_Array_Int_Data(int socket_identifier, int size){
// 	int number_of_bytes;
// 	uint16_t statistics;

// 	int *results = malloc(sizeof(int)*size);
// 	for(int i = 0;i<size;i++){
// 		number_of_bytes = recv(socket_identifier, &statistics,sizeof(uint16_t),0);
// 		results[i] = ntohs(statistics);
// 	} 
// 	return results;
// }

// //Send array data to client
// void Send_Array_Data(int socket_id, int *myArray) {
// 	int i=0;
// 	uint16_t statistics;  
// 	for (i = 0; i < ARRAY_SIZE; i++) {
// 		statistics = htons(myArray[i]);
// 		send(socket_id, &statistics, sizeof(uint16_t), 0);
// 	}
// }

node_leaderboard_t * node_add_leaderboard(node_leaderboard_t *head, player_t *player){
	//Input will be the head previous of where the new players is to be inserted

	//create the new node
	node_leaderboard_t *new = (node_leaderboard_t*) malloc(sizeof(node_leaderboard_t));
	if(new == NULL){
		return NULL;
	}

	if(head == NULL){

		new->player = player;
		new->next = head;


		//If first item in leaderboard return this head, else dont.
		return new;
	}

	node_leaderboard_t *previous = head;

	new->player = player;
	new->next = head;


	//If first item in leaderboard return this head, else dont.
	return NULL;
}

node_leaderboard_t* leaderboard_sort(node_leaderboard_t *head, player_t *player_new){

	// //New player to add
	// player_t *player_new = (player_t*)malloc(sizeof(player_t));
	// //login_t *add_login = (login_t*)malloc(sizeof(login_t));

	// //Populate new player with data
	// player_new->username = "Paul";
	// player_new->playtime = time(0);
	// player_new->won = 10;
	// player_new->played = 15;

	node_leaderboard_t *previous = NULL;

	if(head == NULL){

		head = node_add_leaderboard(head,player_new);

	}else{
		//Rank based on based on time
		
		while(player_new->playtime < head->player->playtime){
			previous = head;
			head = head->next;
			if(head == NULL){
				break;
			}
		}
		printf("\n%d\n",previous->next);
		//Add new person into leaderboard
		node_leaderboard_t *newnode =  node_add_leaderboard(previous,player_new);





		if(head == NULL){
			head = newnode; 
		}
	}

	return head;

	// //Check if times match
	// if(player->time != head->player->time){
	// 	//Add to list after head position
	// }else{
	// 	//Times match, therefore rank on games won
	// 	if(player->won > head->player->won){
	// 		//add to list here
	// 	}else if(player->won < head->player->won){
	// 		while(player->won <= head->player-won && player->time == head->player->time){
	// 			head->next;
	// 		}
	// 		//add to list here
	// 	}
		
	// 	}
	// }

}

void print_leaderboard(node_leaderboard_t *head){
	//Print titles
	printf("\nUsername\t Time\t Games Won\t Games Played");

	while(head != NULL){
		printf("\n%s\t %d\t %d\t %d", head->player->username,head->player->playtime,head->player->won,head->player->played);
		head = head->next;
	}
}


void Send_Array_Data(int socket_id, char *text) {
	uint16_t statistics;  
	for (int i = 0; i < 150; i++) {
		statistics = htons(text[i]);
		send(socket_id, &statistics, sizeof(uint16_t), 0);
	}
}

//Check if tile contains a mine
bool tile_contains_mine(int x,int y){
	
	if(gamestate.tiles[x][y].is_mine == true){
		return true;
	}
	return false;
}

//Place mines
void place_mines(void){
	for(int i = 0;i<NUM_MINES;i++){
		int x,y;
		do{
			x = rand() % NUM_TILES_X;
			y = rand() % NUM_TILES_Y;
		}while(tile_contains_mine(x,y));

		//For surrounding tiles increase adjacent mines by 1
		//Do this first so the adjacent tiles for the mine square will be -1
		for(int i = 0;i<3;i++){		//Y values
			for(int j = 0;j<3;j++){		//X values

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

void node_print(node_login_t *head) {
    for ( ; head != NULL; head = head->next) {
       printf("\n%s",head->login->username);
       printf("	%s\n",head->login->password);
    }
}

//Add usernames and passwords to linked list
node_login_t * node_add_login(node_login_t *head, login_t *login){
	//create the new node
	node_login_t *new = (node_login_t*) malloc(sizeof(node_login_t));
	if(new == NULL){
		return NULL;
	}

	//printf("\n%s",login->username);	has correct name

	//Add data
	new->login = login;
	new->next = head;

	//printf("\n%s",new->login->username);

	return new;
}

node_login_t* load_auth(void){

	FILE* Auth = fopen("Authentication.txt","r");
	char x;
	char data[1000];

	node_login_t *add_data;
	login_t *add_login;

	node_login_t *data_list = NULL;
	//Read single line of document at a time.
	while(fgets(data,1000,Auth) != NULL){

		//Seperate usernames and passwords into seperate strings
		char *username = NULL;
		char *password = NULL;
		char *ptr = strtok(data," 	");

		while(ptr != NULL){
			if(username == NULL){
				username = ptr;
			}else{
				password = ptr;
				ptr = NULL;
			}
			ptr = strtok(NULL," 	");
		}

		//Dont add username and password as a valid login
		if(strcmp("Username",username) != 0){
			login_t *add_login = (login_t*)malloc(sizeof(login_t));

			//Populate new login with data.
			add_login->username = strdup(username);
			add_login->password = strdup(password);

			node_login_t *newhead = node_add_login(data_list,add_login);

			if (newhead == NULL) {
	            printf("memory allocation failure\n");
	            //faliure returns
	        }

	        data_list = newhead;
    	}
	}

	//Close connection with file.
	fclose(Auth);

	//Print list for testing
	//node_print(data_list);

	//Return head of list
	return(data_list);
}

int check_login(node_login_t* head, char* username,char*  password){
	for( ; head != NULL; head = head->next){
		if(strcmp(username, head->login->username) == 0){
			//If the username is in list
			if(strcmp(password,head->login->password) == 0){
				//Password matches and should login
				return 2;
			}else{
				//Password is incorrect
				return 1;
			}
		}
	}
	return 0;
}

void display_board(void){
	//Might not need to show mines as can just make them all apear when one is clicked

	//Create an array of letters to use
	char letters[10]; // cause there is an end line char
	strcpy(letters,"ABCDEFGHI");

	//Printing top 2 rows of board
	printf("\n   ");
	for(int i = 1;i<10;i++){
		printf(" %d",i);
	}
	printf("\n");
	for(int i = 0;i<21;i++){
		printf("-");
	}

	//Printing rows of board
	for(int i = 0;i<NUM_TILES_Y;i++){		//Y values
		printf("\n%c |",letters[i]);	//Print on new row, with preceeding letter

		for(int j = 0;j<NUM_TILES_X;j++){		//X values
			if(gamestate.tiles[j][i].revealed == true){
				//Is revealed, so show the tile underneath
				if(gamestate.tiles[j][i].is_mine == false){
					//Printing adjacent mines, e.g. not a mine or a flag
					printf(" %d",gamestate.tiles[j][i].adjacent_mines);
				}else{
					//Print Mine
					printf(" *");
				}
			}else if(gamestate.tiles[j][i].is_flag == true){
				//Print Flags
				printf(" x");
			}else{
				//Is not revelead yet,so draw blank square
				printf("  ");
			}
		}
	}
	printf("\n");
}

void open_surrounding_tiles(int x, int y){
	//Open us the surrounding tiles from user selection where adjacent_mines == 0
	for(int i = 0;i<3;i++){		//Y values
		for(int j = 0;j<3;j++){		//X values
			
			int Adjacent_X = x+j-1;
			int Adjacent_Y = y+i-1;

			//Checks that the Adjacent values are withing the game limits
			if((Adjacent_X >= 0 && Adjacent_X < NUM_TILES_X) && (Adjacent_Y >= 0 && Adjacent_Y < NUM_TILES_Y)){
				//Checks if it has 0 adjacent mines and is not revealed yet.
				if(gamestate.tiles[Adjacent_X][Adjacent_Y].adjacent_mines == 0 && gamestate.tiles[Adjacent_X][Adjacent_Y].revealed == false){
					//displays the tile, then calls the same function, to search the sorrunding tiles for adjacent_mines == 0;
					gamestate.tiles[Adjacent_X][Adjacent_Y].revealed = true;
					open_surrounding_tiles(Adjacent_X,Adjacent_Y);
				}else{
					//Display one square, next to a 0.

					//dosent work, if you click and edge of the 0s, e.g. (4,6)
					gamestate.tiles[Adjacent_X][Adjacent_Y].revealed = true;
				}
			}
		}
	}
}

int user_input(int x, int y){
	//0 == lose
	//1 == tile selected was ok 

	//So the user inputted numbers line up, with the correct coords
	x--;
	y--;

	//Show whatever tile is selected
	gamestate.tiles[x][y].revealed = true;

	if(gamestate.tiles[x][y].is_mine == true){
		//is a mine and lost, change all mines is revealed to true
		for(int i = 0;i<NUM_TILES_Y;i++){		//Y values
			for(int j = 0;j<NUM_TILES_X;j++){		//X values
				if(gamestate.tiles[j][i].is_mine == true){
					gamestate.tiles[j][i].revealed = true;
				}
			}
		}

		return 0;
	}else if(gamestate.tiles[x][y].adjacent_mines >0){
		//If the adjacent_mines number is not 0, only make the sqaure selected visable
		return 1;

	}else{
		//If the adjacent_mines number is 0, search surronding tiles for 0s
		open_surrounding_tiles(x,y);

		return 1;
	}
}

void place_flags(int x, int y){
	//Place a flag at the user specified. Decrement remaining_mines counter by 1.

	//So the user inputted numbers line up, with the correct coords
	x--;
	y--;

	if(gamestate.tiles[x][y].is_mine == true){
		gamestate.tiles[x][y].is_flag = true;
		gamestate.remaing_mines--;
	}else{
		printf("No mine here");
	}
}

//Server Function
int server_host(node_login_t* head_login){
	int sockfd, new_fd, numbytes;  /* listen on sock_fd, new connection on new_fd */
	struct sockaddr_in my_addr;     //my address information 
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t sin_size;

	//Sending variables 
	char buf[MAXDATASIZE];
	//struct hostent *he;
	//struct sockaddr_in their_addr; /* connector's address information */

	/* generate the socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	/* generate the end point */
	my_addr.sin_family = AF_INET;         /* host byte order */
	my_addr.sin_port = PORTNUMBER;     /* short, network byte order */
	my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
		/* bzero(&(my_addr.sin_zero), 8);   ZJL*/     /* zero the rest of the struct */

	/* bind the socket to the end point */
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) \
	== -1) {
		perror("bind");
		exit(1);
	}

	/* start listnening */
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("\nserver starts listnening ...\n");

	while(1){
		sin_size = sizeof(struct sockaddr_in);

		//Throw error if the player cannot connect propperly
		if((new_fd = accept(sockfd,(struct sockaddr *)&their_addr, &sin_size)) == -1){
			perror("accept");
			continue;
		}

		printf("Server got connection\n");

		//Seperate each connection into a child process
		if(!fork()){

			char* Username = "";
			char* Password = "";
			
			char *text;
			strcpy(text,"Welcome to the online Minesweeper gaming system.\nYou are required to log on with your registered username and password\n\nUsername: ");

			//Username
			//Send inital welcome text
			Send_Array_Data(new_fd, text);
			//Recieve user input
			recv(new_fd, buf, MAXDATASIZE, 0);
			Username = buf;

			//Password
			//Send inital welcome text
			Send_Array_Data(new_fd, "Password: ");
			//Recieve user input
			recv(new_fd, buf, MAXDATASIZE, 0);
			Password = buf;

			//Check user and pass, then return appropriate text
			int login_state = check_login(head_login, Username,Password);

			if(login_state == 2){
				//Username and password are correct
				text = "Welcome - ";
				strcat(text,Username);
				Send_Array_Data(new_fd,text);
			}else{
				Send_Array_Data(new_fd,"You entered either an incorrect username or password. Disconnecting.");
				close(new_fd);
				exit(0);
				return 0;
			}


			//Loop until user closes connection
			// while(1){
			// 	/* Create an array of squares of first 30 whole numbers */
			// 	// int simpleArray[ARRAY_SIZE] = {0};
			// 	// for (int i = 0; i < ARRAY_SIZE; i++) {
			// 	// 	simpleArray[i] = i * i;
			// 	// }
			// 	// simpleArray[0] = 1;

			// 	//Send_Array_Data(new_fd, text);

			// 	/* Receive message back from server */
			// 	if ((numbytes=recv(new_fd, buf, MAXDATASIZE, 0)) == -1) {
			// 		perror("recv");
			// 		exit(1);
			// 	}

			// 	//Print stuff recieved from server.
			// 	buf[numbytes] = '\0';

			// 	//Either print recieved or break the while loop
			// 	if(buf == "close\0"){
			// 		break;
			// 	}else{
			// 		printf("Received: %s",buf);
			// 	}
			// }

		//Convert users input to uppercase
		// for(int i = 0;i<sizeof(input)/sizeof(char);i++){
		// 	input[i] = toupper((int)input[i]);
		// }
		

			//Close connection to player
			close(new_fd);
			exit(0);
		}
	}
}

int main(int argc, char **argv){

	srand(RANDOM_NUMBER_SEED);	//See random number generator

	gamestate.gameover = 1;		//Initilise at 1, as game is not won or lost yet
	gamestate.start_time = time(0);
	gamestate.remaing_mines = NUM_MINES;

	//Score
	long score;

	//Heads of linked lists
	node_login_t* head_login;

	//Login Details
	char* user = "Maolin";
	char* pass = "111111\n";	//escape character is on end of password in list, could try make it better as alast minute thing

	//Variables
	//int gameover = 1;	//0 == loss, 1 == continue, 2 == win
	/////////////////////////////////////////////////////////////////

	//Placing mines, then print for confimation
	place_mines();
	
	//testing where mines are
	// for(int i = 0;i<NUM_TILES_Y;i++){		//Y values
	// 	printf("\n");
	// 	for(int j = 0;j<NUM_TILES_X;j++){		//X values
	// 		printf(" %d ",gamestate.tiles[j][i].adjacent_mines);
	// 	}
	// }
	//printf("\n");

	////////////////////////////////////////////////////////////////

	//Load login list, then search and return if the username and password is correct

	//Place all usernames and passwords in a linked list, loads from text file
	head_login = load_auth();

	//Print login list
	//node_print(head_login);

	int login_state = check_login(head_login,user,pass);

	if(login_state == 0){
		//Username did not exist
		printf("\nUsername does not exist\n");
	}else if(login_state == 1){
		//Incorrect password
		printf("\nIncorrect password\n");
	}else{
		printf("\nLogin succesful\n");
	}

	////////////////////////////////////////////////////////////////

	//Userinput and display board

	//Displays all tiles
	for(int j = 0;j<9;j++){
		for(int i = 0;i<9;i++){
			gamestate.tiles[i][j].revealed = true;
		}
	}
	display_board();

	// //sets back to unseen state
	// for(int j = 0;j<9;j++){
	// 	for(int i = 0;i<9;i++){
	// 		gamestate.tiles[i][j].revealed =  false;
	// 	}
	// }


	//Converts user input of char to number
	char Userinput = 'A';
	int row = (int)Userinput - (int)'A'+1;

	// if(user_input(3,row) == 0){
	// 	gameover = 0;	//Player lost as mine was selected
	// }

	//user_input(4,6);

	//place_flags(3,4);

	//display_board();

	////////////////////////////////////////////////////////////////

	//Win conditions
	//0 == loss, 1 == continue, 2 == win

	if(gamestate.remaing_mines == 0){
		//User has placed all flags on mines
		gamestate.gameover = 2;
	}

	if(gamestate.gameover == 0){
		printf("\nYou Lost\n");
	}else if(gamestate.gameover == 2){
		printf("\nYou Won\n");
		//Score at end of game
		score = time(0)-gamestate.start_time;
		printf("\n Score = %ld",score);
	}

	////////////////////////////////////////////////////////////////
	//Leaderboard

	//Blank leaderboard initially
	node_leaderboard_t *head = NULL;

		//New player to add
	player_t *player_new = (player_t*)malloc(sizeof(player_t));
	//login_t *add_login = (login_t*)malloc(sizeof(login_t));

	//Populate new player with data
	player_new->username = "Paul";
	player_new->playtime = time(0);
	player_new->won = 10;
	player_new->played = 15;

		//New player to add
	player_t *player_new1 = (player_t*)malloc(sizeof(player_t));
	//login_t *add_login = (login_t*)malloc(sizeof(login_t));

	sleep(1);
	//Populate new player with data
	player_new1->username = "jhon";
	player_new1->playtime = time(0);
	player_new1->won = 11;
	player_new1->played = 18;

	head = leaderboard_sort(head,player_new);
	printf("\nhere\n");
	leaderboard_sort(head,player_new1);

	print_leaderboard(head);



	////////////////////////////////////////////////////////////////

	//Server Stuff
	server_host(head_login);
}


///////// Stuff Left //////////////
//server and client connections
//Change it so at input, it decrements value by 1. e.g. in place flags and user input
//Check win conditon is only flags, there isnt a option to display the whole board except mines
//Do you get a score if you lose?
//Make it so text sent to client is only sending the exact size of the text everytime.
//check that mines are in the correct places

///////// Done stuff ////////////
//Open sorrounding tiles
//Flags
//user inputs
//Win and lose conditions

