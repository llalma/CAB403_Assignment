

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
	their_addr.sin_port = htons(PORTNUMBER);    // Use defined port
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