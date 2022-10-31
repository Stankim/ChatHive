#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include <unistd.h>
#include <pthread.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#define DEFAULT_REMOTE_IP "127.0.0.1"
#define DEFAULT_PORT 8123
#define MAX_SIZE_OF_MESSAGE 110
#define MAX_SIZE_OF_IPADDRESS 24
#define DEFAULT_KEY "8345.676"


int interpret_command(const char *arg){
	const char *commands[] = {
		"messages",
		"send",
		"reply",
		"listen",
		"login",
		"logout",
		"set-key",
		"set-ring",
		NULL
	};
	for(int j=0;commands[j];j++){
		_Bool match = 1;
		for(int i=0;arg[i]&&commands[j][i];i++){
			if(arg[i] != commands[j][i]  && arg[i] != (commands[j][i] - 32)){
				match = 0;
				break;
			}
		}
		if(match) return j;
	}
	return -1;
}


char* readFileToBuffer(char *filename){
	FILE *file = fopen(filename, "r");
	if(file == NULL) return NULL;
	fseek(file, 0, SEEK_END); // seek to end of file
	int size = (int)ftell(file); // get current file pointer
	fseek(file, 0, SEEK_SET); // seek back to beginning of file
	char* buffer = malloc(size);
	buffer[size]='\0';
	fread(buffer, 1, size, file);
	fclose(file);
	return buffer;
}

int readAllFileToBuffer(char *filename, int size_to_read, char**buffer){
	printf("ring is \n" );

	FILE *file = fopen(filename, "r");
	if(file == NULL) return -1;
	int size = fread(buffer, 1, size_to_read, file);
	printf("read %d\n", size);
	printf("buffer  %c\n", buffer[0][0]);
	fclose(file);
	return size;
}



int extractNextInteger(char** str, char separator){
	if(str[0] == NULL || strcmp(str[0],"")==0) return -1;
	int i;
	for(i=0;str[0][i] && str[0][i] != (int)separator;i++);
		char number_str[i+1];
	number_str[i]= '\0';
	for(int j=0;j<i;number_str[j]=*str[0],j++,str[0]++);
		if(str[0][0] != '\0') str[0]++;
	return atoi(number_str);
}

_Bool write_log(char * message){
	time_t timer;
	struct tm* tm_info;
	timer = time(NULL);
	tm_info = localtime(&timer);
	FILE *file = fopen("logs", "w+");
	if(file==NULL) return 0;
	int success = fprintf(file, "%s\n\n%s\n\n\n\n",asctime(tm_info), message);
	fclose(file);
	if(success<=0) return 0;
	return 1;
}


_Bool writeKey(unsigned int  first_key, unsigned int second_key){
	unsigned int step = second_key - first_key;
	FILE *file = fopen("key", "w");
	if(file==NULL) return 0;
	int success = fprintf(file, "%u.%u", first_key, step);
	fclose(file);
	if(success<=0) return 0;
	return 1;
}
_Bool IsPrime(int n)
{
	if (n == 2 || n == 3)
		return 1;
	if (n <= 1 || n % 2 == 0 || n % 3 == 0)
		return 0;
	for (int i = 5; i * i <= n; i += 6){
		if (n % i == 0 || n % (i + 2) == 0)
			return 0;
	}
	return 1;
}

unsigned int nextPrime(unsigned int number){
	do	number++; while(!IsPrime(number));
	return number;
}


_Bool addKey(char * key){
	unsigned int keys[2];
	unsigned int step;
	if(sscanf(key, "%d.%d", &keys[0], &step) < 2 )
		return 0;
	keys[1] = keys[0] + step;
	if(!IsPrime(keys[0]))
		keys[0] = nextPrime(keys[0]);
	if(!IsPrime(keys[1]))
		keys[1] = nextPrime(keys[1]);
	if(writeKey(keys[0], keys[1])) return 1;
}

int countPrimes(unsigned int min, unsigned int n, unsigned int count[2]){
	if(min >= n) return 0;
	count[0] = 0;
	count[1] = 0;
	unsigned int num[n], i, j;
	for(i = 0; i < n; i++)
		num[i] = i + 1;  
	for(i = 1; (num[i] * num[i]) <= n; i++){
		if(num[i] != 0){
			for(j = num[i] * num[i]; j <= n; j += num[i])
				num[j - 1] = 0;  
		}
	}
	for(i = 1; i < n; i++){  
		if(num[i] != 0){
			if(i<min-1)
				count[1]++;
			count[0]++;
		}
	}
	return 1;
}


unsigned long int generateDynamicReferenceNumber(){
	FILE *file = fopen("key", "r");
	if(file==NULL){
		addKey(DEFAULT_KEY);
		file = fopen("key", "r");
	}
	unsigned int keys[2];
	unsigned int step;
	int read_vars = fscanf(file, "%d.%d", &keys[0], &step);
	fclose(file);
	if(read_vars<=0){
		return 0;
	}
	keys[1] = keys[0] + step;
	//check if first key and second key are prime that they are not adjacent
	if(keys[0] == keys[1] || !IsPrime(keys[0]) || !IsPrime(keys[1]))
	{ 
		write_log(strcat("The key has been corrupted.", "Try\n: appname set-key"));
		return 0;
	}
	unsigned int count[2];

	countPrimes(keys[0], keys[1], count);
	if(count[0] - count[1] == 1  || (count[0] + count[1])%2 == 0){
		keys[1] = nextPrime(keys[1]);
	}
	else{
		keys[0] = nextPrime(keys[0]);
	}
	if(writeKey(keys[0], keys[1])) return (unsigned long int)keys[0] * (unsigned long int)keys[1];
	return 0;
}

unsigned int extractPort(char **ring){
	unsigned int ring_port = 0;
	char* index_of_separator = strchr(ring[0],':');
	if(index_of_separator != NULL && index_of_separator  != ring[0]){
		ring_port = atoi(index_of_separator+1);
		index_of_separator[0] = '\0';
	}
	return ring_port;
}

void prepareCallToServer(struct sockaddr_in* ring_addr){
	unsigned int ring_port;
	char ring[MAX_SIZE_OF_IPADDRESS];
	FILE *fp = fopen("ring", "r");
	int read_vars = fscanf(fp, "%s : %d", ring, &ring_port);
	fclose(fp);
	if(read_vars < 2){
		sprintf(ring, "%s", DEFAULT_REMOTE_IP);
		ring_port = DEFAULT_PORT;
	}
	memset(ring_addr, '0', sizeof(ring_addr[0]));
	ring_addr->sin_family = AF_INET;
	ring_addr->sin_port = htons(ring_port);
	ring_addr->sin_addr.s_addr = inet_addr(ring);
	return;
}





int  callServer(struct sockaddr_in *ring_addr, char *message, char incoming_buffer[][MAX_SIZE_OF_MESSAGE]){
	unsigned int sock;
	int valread;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("An error occurred. The receiving socket cannot be created.\n");
		return 0;
	}
	else{
		if (connect(sock, (struct sockaddr *)ring_addr, sizeof(ring_addr[0])) < 0)
		{
			printf("Error: Fetcher cannot make connection to server.\n");
			valread= -1;
		}
		else{
			if(send(sock , message , strlen(message) , 0 ) <0){
				printf("There was an error sending your messsage.\n");
			}
			else{
				printf("Client sent message to server.\n");
			}
		    			#ifdef _WIN32
			if((valread = recv(sock , incoming_buffer[0] , MAX_SIZE_OF_MESSAGE , 0)) <0)
			{
				puts("Receiving data failed");
			}
		    			#else
			if(valread = read( sock , incoming_buffer[0], MAX_SIZE_OF_MESSAGE) < 0){
				printf("Receiving data failed\n");
				return 0;
			}

		    			#endif
			incoming_buffer[0][valread] = '\0';
		}
					#ifdef _WIN32
		closesocket(sock);
    				#else
		close(sock);
    				#endif
	}
	return valread;
}

int getFromServer(char *message, char incoming_buffer[][MAX_SIZE_OF_MESSAGE]){
	struct sockaddr_in ring_addr;
	prepareCallToServer(&ring_addr);
	return callServer(&ring_addr, message, incoming_buffer);
}

long long int getDynamicReferenceNumber(){
	char reply[MAX_SIZE_OF_MESSAGE];
	int amount_received = getFromServer("1",&reply);
	char *end_ptr;
	return strtoull(reply,&end_ptr,10);
}

_Bool writeSessionUser(long long int user_id){
	FILE *file = fopen("session_user", "w");
	if(file==NULL) return 0;
	int success = fprintf(file, "%llu", user_id);
	fclose(file);
	if(success<=0) return 0;
	return 1;
}

long long int getSessionUser(){
	long long int ref_id;
	FILE *file = fopen("session_user", "r");
	int read_vars = fscanf(file, "%llu", &ref_id);
	fclose(file);
	if(read_vars==1){
		return ref_id;
	}
	ref_id = getDynamicReferenceNumber();
	if(ref_id == 0)
	{
		printf("Error: Unable to determine the user.\n\
			Please make sure you have an active connection to the ring server.\n\
			If that's not it try changing the ring server: appname set-ring {url}\n\
			NB:\n\
			To remain remembered from any computer within the network of the ring server create an account: appname login {username|email} {password}\n");
		return 0;
	}
	if(writeSessionUser(ref_id)){
		return ref_id;
	}
	else{
		printf("An error occurred saving the user id. Please check if you have writing permissions on the file.\n");
	}
}



_Bool writeRing(const char* ring){
	unsigned int ring_port= extractPort((char**)&ring);
	if(ring_port  == 0)
		ring_port = DEFAULT_PORT;
	FILE *file = fopen("ring", "w");
	if(file==NULL) return 0;
	int success = fprintf(file, "%s : %d", ring, ring_port);
	fclose(file);
	if(success<=0) return 0;
	return 1;
}

int readIntFromFile(char *filename){
	char * number = readFileToBuffer(filename);
	int num = atoi(number);
	free(number);
	return num;
}
_Bool writeIntToFile(int number, char *filename){
	FILE *file = fopen("ring", "w");
	if(file==NULL) return 0;
	int success = fprintf(file, "%d", number);
	fclose(file);
	if(success<=0) return 0;
	return 1;
}

int getNewMessageId(){
	int message_id = readIntFromFile("message-id");
	message_id++;
	if(writeIntToFile(message_id, "message-id")){
		return message_id;
	}
	return 0;
}

_Bool queueMessage(char *message, char *recipient, int in_reply_to, long long int sender){
	FILE *file = fopen("outgoing", "a");
	if(file == NULL)
		return 0;
	int success = fprintf(file, "%s %d (;)%s(;) %d %llu", recipient, getNewMessageId(), message, in_reply_to, sender);
	fclose(file);
	if(success<=0) return 0;
	return 1;
}

_Bool writeNewMessage(char* message, unsigned long long int session_user){
	char file_path[27];
	sprintf(file_path, "users/%llu", session_user);
	FILE *file = fopen(file_path, "a");
	int written_chars = fprintf(file, "%s\n", message);
	fclose(file);
	if(written_chars>0) return 1;
	return 0;
}


int getUserMessages(char message[][MAX_SIZE_OF_MESSAGE], unsigned long long int session_user){
	char file_path[27];
	sprintf(file_path, "users/%llu", session_user);
	FILE *file = fopen(file_path, "r");
	int read_vars = fscanf(file, "%s", message);
	fclose(file);
	return read_vars;
}

void closeServerSocket(int server_fd){
	#ifdef _WIN32
	closesocket(server_fd);
    #else
	close(server_fd);
    #endif
}


int replyToClient(int client_sock, char reply[][MAX_SIZE_OF_MESSAGE]){
	int valsend;
		#ifdef _WIN32
	valsend = send(client_sock , reply[0] , strlen(reply[0]) , 0);
	closesocket(client_sock);
		#else
	valsend = write(client_sock , reply[0] , strlen(reply[0]));
	close(client_sock);
		#endif
	if(valsend<0){
		printf("Error replying to client\n");
	}
	return valsend;
}


int serverAccept(int server_fd, struct sockaddr_in * local_address, char incoming_buffer[][MAX_SIZE_OF_MESSAGE]){
	int client_sock, valread;
	int addrlen = sizeof(local_address[0]);
	if ((client_sock = accept(server_fd, (struct sockaddr *)local_address, (socklen_t*)&addrlen))<0)
	{
		perror("There is a problem accepting the connection");
		return -1;
	}
	#ifdef _WIN32
	valread = recv(client_sock , incoming_buffer[0] , MAX_SIZE_OF_MESSAGE , 0);
	incoming_buffer[0][valread] = '\0';
						#else
	valread = read(client_sock , incoming_buffer[0], MAX_SIZE_OF_MESSAGE);
	incoming_buffer[0][valread] = '\0';
						#endif
	if(valread<0){
		printf("Receiving data failed");
		return -1;
	}
	return client_sock;
}

int prepareServer(struct sockaddr_in * local_address){
	int server_fd;
	if((server_fd = socket(AF_INET , SOCK_STREAM , 0 )) <0){
		printf("Could not create socket for the server.");
		return -1;
	}
	else{
		local_address->sin_family = AF_INET;
		local_address->sin_addr.s_addr = INADDR_ANY;
		local_address->sin_port = htons( DEFAULT_PORT );
		memset(local_address->sin_zero, '\0', sizeof local_address->sin_zero);
		if (bind(server_fd, (struct sockaddr *)local_address, sizeof(local_address[0]))<0)
		{
			printf("The server cannot be created. Please try another port number.\n");
			return -1;	
		}
	}
	if(listen(server_fd, 10) < 0)
	{
		perror("Error: The server is unable to start listening.\n");
		return -1;
	}
	return server_fd;
}


int main(int argc, char const *argv[])
{
	if(argc == 1){
		printf("Please try:\n\
			appname messages -new -all -from {username|email}\n\
			appname send \"{text}\" to {username|email|unique-ref}\n\
			appname reply {message-id} -with \"{text}\"\n\
			appname listen\n\
			appname login {username|email} {password} -new {password}\n\
			appname logout\n\
			appname set-key {stem.branch}\n\
			appname set-ring {ip:port}\n\
			appname set-verify-email {true|false}\n\
			appname set-verify-non-robot {true|false}\n");
		return 0;
	}




	switch(interpret_command(argv[1])){
		case 0:
		;long long int session_user = getSessionUser();
		if(session_user == 0) return 0;
		char file_path[27];
		sprintf(file_path, "users/%llu", session_user);
		FILE *file = fopen(file_path, "r");
		if(file == NULL){
			printf("Error: You currently have no messages. Make sure to switch on listening mode: appname listen%s\n");
			return 0;
		}
		printf("Your Messages\n");
		char read_char;
		do
		{
			read_char = (char)fgetc(file);
			printf("%c",read_char);
		} while(read_char != EOF);
		fclose(file);
		break;
		





		case 1:
		session_user = getSessionUser();
		if(session_user == 0) return 0;
		if(argc != 5){
			printf("Error: Expected 3 arguments. {\"messages\"} -to {username|email|unique-ref}\n");
			return 0;
		}
		if(strcmp(argv[3], "-to") != 0){
			printf("Error: Wrong format. Try:\nappname send {\"messages\"} -to {username|email|unique-ref}\n");
			return 0;
		}
		printf("Sending...");
		if(queueMessage((char *)argv[4], (char *)argv[2], 0 ,session_user)){
			printf("Message send.\n");
		}
		else{
			printf("Message not send. Please try again.\n");
		}
		break;






		case 2:
		session_user = getSessionUser();
		if(session_user == 0) return 0;
		if(argc != 4)
		{
			printf("Error: You have not provided adequate arguments for the command: reply {message-id} with \"{message}\"\n");
			return 0;
		}
		if(strcmp(argv[2], "-with") != 0){
			printf("Error: Wrong format. Try:\nappname send {\"messages\"} to {username|email|unique-ref}\n"); 
			return 0;
		}
		printf("Sending reply...\n");
		if(queueMessage((char *)argv[4], 0, atoi(argv[2]), 0 )){
			printf("Message queued for sending.\n");
		}
		else{
			printf("Message not replied. Please try again.\n");
		}
		break;




		case 3:
		#ifdef _WIN32
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
		{
			printf("Failed. Error Code : %d \n", WSAGetLastError());
			return 1;
		}
    		#endif

		;void *serve(){

			struct sockaddr_in local_address;
			unsigned int server_fd, client_sock;
			server_fd = prepareServer(&local_address);
			while(1){
				char incoming_buffer[MAX_SIZE_OF_MESSAGE] , reply[MAX_SIZE_OF_MESSAGE];
				client_sock = serverAccept(server_fd, &local_address, &incoming_buffer);
				switch(incoming_buffer[0]){
					case '1':
					sprintf(reply, "%lu" , generateDynamicReferenceNumber());
					break;
					case '2':
					;unsigned long long int user;
					sscanf(incoming_buffer, "%llu ", &user);
					writeNewMessage(incoming_buffer+1, user);
					sprintf(reply, "2%s", "ok");
					break;
					case '3':
					;char user_messages[MAX_SIZE_OF_MESSAGE];
					sscanf(incoming_buffer, "%llu ", &user);
					if(getUserMessages(&user_messages, user) > 0){
						sprintf(reply, "1%s", user_messages);
					}
					else{
						reply[0] = '9';
						reply[1] = '\0';
					}
					break;
					default:
					printf("%s received\n", incoming_buffer);
					sprintf(reply, "Hello.");
				}

				replyToClient(client_sock, &reply);
			}
			closeServerSocket(server_fd);
		}

		pthread_t thread_id;
		pthread_create(&thread_id, NULL, serve, NULL);

		;void *listen(){
			struct sockaddr_in ring_addr;
			prepareCallToServer(&ring_addr);

			unsigned long long int session_user = getSessionUser();
			if(session_user ==0) return NULL;
			
			while(1){
				char message[MAX_SIZE_OF_MESSAGE];
				sprintf(message, "3%llu", session_user);
				char incoming_buffer[MAX_SIZE_OF_MESSAGE];
				int chars_returned = callServer(&ring_addr, message, &incoming_buffer);
				if(chars_returned<=0)
					continue;
				switch(incoming_buffer[0]){
					case '1':
					printf("New message received:\n%s\n", incoming_buffer+1);
					writeNewMessage(incoming_buffer+1, session_user);
					break;
					case '2':
					printf("Message sent.\n");
					break;			
					default:
					printf("client received %s\n",incoming_buffer);

				}

			}
		}
		pthread_t thread_id2;
		pthread_create(&thread_id2, NULL, listen, NULL);
		getchar();
		pthread_exit(listen);
		pthread_exit(serve);
		#ifdef _WIN32  
		WSACleanup();
    	#endif
		//post reply -save
		//post message - save
		//get their message - reply with users/{username|email|unique ref}
		//get them a dynamic key

		break;






		case 4:
		printf("Login\n");
		break;




		case 5:
		printf("Logout\n");
		break;




		case 6:
		if(argc == 2){
			printf("Error: No arguments provided for set-key command.\n");
			return 0;
		}

		if(!addKey((char *)argv[2])){
			printf("An error occurred setting the key.\n\n");
		}
		break;






		case 7:
		if(argc<3)
		{
			printf("Error: Inadequate number of arguments. Please provide an ip address for the ring computer and port. eg. 123.456.789.001:1234. The port is optional.\n");
			return 0;
		}
			//add a regex expression for ip address or domain with port
		if(!writeRing(argv[2])){
			printf("An error occurred setting the ring address. Make sure it is in the correct format ie: {ip:port}. For example 123.456.789.001:1234 \n\n");
		}
		break;
		default:
		printf("Error: %s not a valid appname command. To see the available commands. Try \nappname\n ", argv[1] );
	}

	return 0;
}
