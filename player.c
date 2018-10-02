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


// void Send_Array_Data(int socket_id, int *myArray) {
// 	int i=0;
// 	uint16_t statistics;  
// 	for (i = 0; i < ARRAY_SIZE; i++) {
// 		statistics = htons(myArray[i]);
// 		send(socket_id, &statistics, sizeof(uint16_t), 0);
// 	}
// }

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

int main(int argc, char *argv[]) {
	struct hostent *he;
	struct sockaddr_in their_addr; /* connector's address information */
	int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
	socklen_t sin_size;

	if ((he=gethostbyname(argv[1])) == NULL) {  /* get the host info */
		herror("gethostbyname");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}


	their_addr.sin_family = AF_INET;      /* host byte order */
	their_addr.sin_port = PORTNUMBER;    /* short, network byte order */
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

	if (connect(sockfd, (struct sockaddr *)&their_addr,	sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}

	//Loop until user says to exit
	while(1){
		//Recieve array data
		char *results = Receive_Array_Int_Data(sockfd,  ARRAY_SIZE);

		//Print out the array
		printf("\n%s",results);
		// for(int i = 0;i<ARRAY_SIZE;i++){
		// 	printf("%s",results[i]);
		// }

		free(results);

		//These 100 values need to be changed so they can be used 
		//with any size input, might also need to change server file then.
		char input[100];
		fgets(input,100,stdin);

		//Send to the player that all data was recieved by the server.
		send(sockfd,input,40,0);
		//send(sockfd,"All of array data received by server\n", 40 , 0);
	}

	//sleep(10);

	close(sockfd);
	printf("\nSocket Closed");

	return 0;
}
