// autor: Ondřej Bahounek, xbahou00


typedef struct {

    int opcode;
    char mode[15];
    char file_path[100];
    int err;
    bool unknown_extension;
    bool accepted_extensions;
    bool blck_size_option;
    int block_size;
    bool t_size_option;
    long unsigned int t_size;
    bool time_out_option;
    int time_out;
}initial_data_t;

typedef struct {
    initial_data_t initial;
    struct sockaddr_in client_address;
}initial_and_client_t;


typedef struct {
    int port;
    char root_dir_path[50];
}command_line_args_t;

typedef struct{
    char **options;
    char **values;
}extension_and_values_t;


typedef struct{
    char *data;
    int size;
}data_and_size_t;
//parse extesion from buffer at position bytes_parsed,
//load them into extension_and_values
//if given extension is accepted by server (tsize,blksize,timeout), set given values in initial_data_t initial
//increment bytes_parsed
int parse_extension(int *bytes_parsed,char *buffer,initial_data_t *initial,extension_and_values_t *extension_and_values);

//print request message received to stderr in format given by assigment
int print_request_message(extension_and_values_t extension_and_values,initial_data_t initial_data,struct sockaddr_in client_address);

//parse request message into initial_data_t structure
initial_data_t handle_first_packet(struct sockaddr_in client_address,char *buffer,int bytes_recieved);

//blocking fucntion that waits until server receive request message,
//than create child procces to handle the request, when parent proccess goes back to waiting for next request
//function uses semaphore for making sure that there is no mishandling of data accross different processes
initial_and_client_t waiting_for_initial(command_line_args_t args);


//send option acknowledgment to the client
//sercer acknowledg only blksize,timeout and tsize options
//socket is expected to be connected udp socket
int send_options_acknowledgment(initial_data_t initial, int socket,command_line_args_t args);


//load data send by client
//uses options given by initial_data
//socket is expected to be connected udp socket
//returns file in data_and_size_t struct
data_and_size_t server_load_data(int server_socket,initial_data_t initial_data);

//create file in folder given by args and named by name given in initil_and_client from request message
int create_file(command_line_args_t args, initial_and_client_t initial_and_client, data_and_size_t data_and_size);

//wait for data block 0
int recv_data_0(int server_socket);

//send file to client
//uses options given by initial_data
//socket is expected to be connected udp socket
//returns 1 on succes
int upload_file(int server_socket,command_line_args_t args,initial_and_client_t initial_and_client);

//check if file i i want to create alredy exist
//return 1 if he does, 0 otherwise
int file_already_exist(command_line_args_t args,initial_and_client_t initial_and_client);


