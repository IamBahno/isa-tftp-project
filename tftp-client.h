// autor: Ondřej Bahounek, xbahou00


typedef struct {
    char hostname[36];
    int port;
    char filepath[50];
    char dest_filepath[50];
    int opcode;
}command_line_args_t;





typedef struct{
    char *data;
    int size;
}data_and_size_t;

//function to parse command line arguments into command_line_args_t structure
command_line_args_t parse_arguments(int argc,char *argv[]);

//send request to the server based on command_line_args_t structure
//return 1 on succes, 0 on failure, calls exit, if file i want create already exists
int send_initial_packet(int client_socket,command_line_args_t args,struct sockaddr *server,int server_size);

//upload file on server
//send in datagrams with 512B of data, after sending block of data wait for acknowledgment
//if acknowledgment didnt come within 3 second, send the block again, function will send the block 3 times,
//before sending error message to server and terminating run of program with exit(EXIT_FAILURE)
//parameter int client_socket is expected to be connected udp socket with the server
int upload_file(int client_socket,char *first_message,int message_size);

//download file from server
//function receive DATA blocks from server, and join them together to form a file
//sends ackowledgemnt after each block receive
//if function didnt receive new block in 3 seconds, func. sends a aknowledgment again, up to 3 times,
//before sending ERROR message and terminating
//function ends the communication with server after receiving DATA block with less than 512B of DATA
//parameter int client_socket is expected to be connected udp socket with the server
data_and_size_t dowload_file(int client_socket,char *first_message,int message_size);


//function creates file named by command_line_args_t args
//with data given by data_and_size
//if function fails, function terminates with exit(ERROR_FAILURE)
int create_file(command_line_args_t args, data_and_size_t data_and_size);