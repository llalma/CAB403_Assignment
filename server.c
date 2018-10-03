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


#define RANDOM_NUMBER_SEED 42 // Sead for randomisation

#define MYPORT 12345    // the port users will be connecting to 
#define BACKLOG 10     // how many pending connections queue will hold
#define MAXDATASIZE 1000 

// Defining size of play area
#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10

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

// Read authentication file and process contents //
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
}

// Check inputted username and password against lists //
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


void server_setup ( void ){
	printf("\nServer started.\n");

	int server_socket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = Internet Protocol v4 Addresses (Family), SOCK_STREAM = TCP, 0 = protocol default

	//Heads of linked lists
	node_login_t* head_login;

	//Place all usernames and passwords in a linked list, loads from text file
	head_login = load_auth();
	printf("Usernames in linked lists from text file.\n");

	// Start/define network services
	printf("Server starts networking services.\n");

	struct sockaddr_in server_address; // address of server
	server_address.sin_family = AF_INET; // Define protocol
	server_address.sin_port = htons(MYPORT); // Use defined port
	server_address.sin_addr.s_addr = INADDR_ANY; // Use any IP address on local machine IE 0.0.0.0 
	
	// Bind the socket to the IP and PORT specified

	bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

	// Start listening for connections
	listen(server_socket, BACKLOG);
	printf("\nServer is waiting for log in request...\n");
	// Client connection
	int client_socket = accept(server_socket, NULL, NULL); // NULL and NULL would be filed with STRUC if we want to know where the client is connecting from etc
	if ( client_socket == -1 ){
		printf("\nClient Unable to connect.");
	}
}

int main ( void ){	
	server_setup();

	return 0;
}