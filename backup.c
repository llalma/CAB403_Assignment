void request_thread(pthread_mutex_t p_mutex, pthread_cond_t* p_cond_var, int newfd, int request_num){

	int rc; 
	struct request* a_request;

	a_request = (struct request*)malloc(sizeof(struct request));
	
	a_request->number =request_num;
	a_request->next = NULL;
	a_request->newfd = newfd;

	rc = pthread_mutex_lock(p_mutex); 

	if (num_requests == 0){
		requests = a_request;
		last_request = a_request;
	}
	else{
		last_request->next = a_request;
		last_request = a_request;
	}

	num_requests++;

	rc = pthread_mutex_unlock(p_mutex);

	rc = pthread_cond_signal(p_cond_var);

}


struct requests* get_request(pthread_mutex_t* p_mutex){
	int rc;
	struct request* a_request;

	rc = pthread_mutex_lock(p_mutex);

	if (num_requests > 0){
		a_request = requests;
		requests = a_request->next;
		if (requests == NULL){
			last_request = NULL;
		}
		num_requests--;
	}
	else {
		a_request = NULL;
	}

	rc = pthread_mutex_unlock(p_mutex);

	return a_request
}

void* handle_reuqests_loop(void* arg) {

    struct request* a_request;      
    int thread_id = *((int*)data);  
    rc = pthread_mutex_lock(&request_mutex);

    while (1) {
        if (num_requests > 0) { 
            a_request = get_request(&request_mutex);
            if (a_request) { 
                rc = pthread_mutex_unlock(&request_mutex);
                handle_request(a_request, thread_id);
                free(a_request);
                rc = pthread_mutex_lock(&request_mutex);
            }
        }
        else {                                  
            rc = pthread_cond_wait(&got_request, &request_mutex);

        }
    }
}