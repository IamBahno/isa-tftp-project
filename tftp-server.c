// autor: Ondřej Bahounek, xbahou00
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <semaphore.h>  
#include <fcntl.h> // Required for O_CREAT and O_EXCL flags
#include <sys/stat.h> // Required for mode_t type
#include "tftp-server.h"
#include "tftp-utils.h"
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <sys/statvfs.h> // space available on disk
#include <sys/stat.h> //size of file

#define BUFFER (1024)


#define MAX_WAIT_TIME_SEC 3


#define BLOCK_SIZE 512


sem_t *my_semaphore;


//for handling Ctrl+c
void handler()
{
    // Close the named semaphore when done with it
    if (sem_close(my_semaphore) != 0) {
        printf("Semaphore close failed");
        exit(1);
    }

    // Unlink the named semaphore (optional) to remove it from the system
    if (sem_unlink("/my_semaphore_xbahou00") != 0) {
        printf("Semaphore unlink failed");
        exit(1);
    }
    printf("SIGINT");
    exit(EXIT_SUCCESS);
}

command_line_args_t parse_arguments(int argc,char *argv[])
{
    if(argc != 2 && argc != 4)
    {
        printf("Wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    command_line_args_t args;
    args.port = 69;
    if (argc == 2){
        strcpy(args.root_dir_path,argv[1]);
    }
    else{
        if(strcmp(argv[1],"-p") == 0){
            args.port = atoi(argv[2]);
            if(args.port != 0){
                strcpy(args.root_dir_path,argv[3]);
                return args;
            }
        }
        printf("Wrong arguments\n");
        exit(EXIT_FAILURE);
    }
    return args;
}


int parse_extension(int *bytes_parsed,char *buffer,initial_data_t *initial,extension_and_values_t *extension_and_values)
{
    char extension[BUFFER];

    strcpy(extension,buffer + *bytes_parsed);
    for (int i = 0 ; extension[i] != '\0' ;i++) 
    {
        extension[i] = tolower(extension[i]);
    }
            int i;
        for(i = 0;extension_and_values->options[i] != NULL;i++);
        extension_and_values->options[i] = malloc(strlen(extension) + 1);
        extension_and_values->values[i] = malloc(strlen(buffer + *bytes_parsed) + 1);
        strcpy(extension_and_values->options[i],extension);
        strcpy(extension_and_values->values[i],buffer + *bytes_parsed+strlen(extension) + 1);
    if ((strcmp(extension,"blksize") == 0) || (strcmp(extension,"tsize") == 0) || (strcmp(extension,"timeout") == 0))
    {
        if( strcmp(extension,"blksize") == 0)
        {
            (*initial).blck_size_option = true;
            *bytes_parsed += strlen("blksize") + 1;
            (*initial).block_size = atoi(buffer + *bytes_parsed);
            *bytes_parsed += strlen(buffer + *bytes_parsed) + 1;
        }
        else if( strcmp(extension,"tsize") == 0)
        {
            (*initial).t_size_option = true;
            *bytes_parsed += strlen("tsize") + 1;
            (*initial).t_size = atol(buffer + *bytes_parsed);
            *bytes_parsed += strlen(buffer + *bytes_parsed) + 1;
        }
        else
        {
            (*initial).time_out_option = true;
            *bytes_parsed += strlen("timeout") + 1;
            (*initial).time_out = atoi(buffer + *bytes_parsed);
            *bytes_parsed += strlen(buffer + *bytes_parsed) + 1;
        }
        (*initial).accepted_extensions = true;

    }
    else
    {
        initial->unknown_extension = true;
        *bytes_parsed += strlen(extension) + 1;
        *bytes_parsed += strlen(buffer + *bytes_parsed) + 1;
    }
    return 1;
}

int print_request_message(extension_and_values_t extension_and_values,initial_data_t initial_data,struct sockaddr_in client_address){
    char message[BUFFER];
    if(initial_data.opcode == WRQ){
        strcpy(message,"WRQ ");
    }
    else{
        strcpy(message,"RRQ ");
    }
    strcat(message,inet_ntoa(client_address.sin_addr));
    strcat(message,":");
    char port[20];
    sprintf(port, "%d", ntohs(client_address.sin_port));
    strcat(message,port);
    strcat(message," \"");
    strcat(message,initial_data.file_path);
    strcat(message,"\" ");
    strcat(message,initial_data.mode);
    
    for(int i = 0;extension_and_values.options[i] != NULL;i++){
        strcat(message," ");
        strcat(message,extension_and_values.options[i]);
        strcat(message,"=");
        strcat(message,extension_and_values.values[i]);
    }
    strcat(message,"\n");
    fprintf(stderr,"%s",message);
    return 1;    


}

initial_data_t handle_first_packet(struct sockaddr_in client_address,char *buffer,int bytes_recieved)
{

     initial_data_t initial;
     initial.err = 0;
        if(buffer[1] == 1){
            initial.opcode = RRQ;
        }
        else if(buffer[1] == 2){
            initial.opcode = WRQ;
        }
        else{
            initial.err = ERROR_ILLEGAL_TFTP;
            return initial;
        }

        int bytes_parsed = 2;
        strcpy(initial.file_path,buffer+2);
        bytes_parsed += strlen(buffer+2) + 1;

        strcpy(initial.mode,buffer+bytes_parsed);
        bytes_parsed += strlen(initial.mode) + 1;
        extension_and_values_t extension_and_values;
        if (bytes_parsed == bytes_recieved){
            initial.accepted_extensions = false;
            extension_and_values.options = malloc(100*sizeof(char *));
            extension_and_values.values = malloc(100*sizeof(char *));
            print_request_message(extension_and_values,initial,client_address);
            return initial;
        }
        else
        {
            initial.time_out_option = false;
            initial.t_size_option = false;
            initial.blck_size_option = false;
            extension_and_values.options = malloc(100*sizeof(char *));
            extension_and_values.values = malloc(100*sizeof(char *));
            while(bytes_parsed < bytes_recieved)
            {
                parse_extension(&bytes_parsed,buffer,&initial,&extension_and_values);
            }
            print_request_message(extension_and_values,initial,client_address);
            return initial;
        }
}



initial_and_client_t waiting_for_initial(command_line_args_t args)
{
    // Create or open a named semaphore named "/my_semaphore_xbahou00"
    my_semaphore = sem_open("/my_semaphore_xbahou00", O_CREAT, 0644, 1);
    if (my_semaphore == SEM_FAILED) {
        printf("Semaphore creation/opening failed");
        exit(1);
    }

    int fd;
    socklen_t length;
    struct sockaddr_in server,client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(args.port);

    fd = socket(AF_INET, SOCK_DGRAM, 0);

      if (bind(fd, (struct sockaddr *)&server, sizeof(server)) == -1){
    printf("bind() failed");
    exit(EXIT_FAILURE);}
    length = sizeof(client);

    int n;
    char buffer[BUFFER];


    pid_t pid;
    while(1)
    {
        n= recvfrom(fd, buffer, BUFFER, 0, (struct sockaddr *)&client, &length);
        printf("recieved request\n");
        struct sockaddr_in new_client;

        pid = fork();
        if (pid < 0) {
            printf("Error in fork");
            exit(1);
        }
        //child proccess copy the request message and client info
        //and call function to handle the request 
        else if (pid == 0) 
        {
            memcpy(&new_client,&client, sizeof(struct sockaddr_in));
            sem_post(my_semaphore);
            initial_data_t initial = handle_first_packet(new_client, buffer, n);
            initial_and_client_t initial_and_client;
            initial_and_client.initial = initial;
            initial_and_client.client_address = new_client;
            return initial_and_client;
        } else { 
            // Parent process
            //wait untill child coppied data and call its function
            // Continue listening for the next packet
            sem_wait(my_semaphore); 
            continue;
        }

       


    }


}

int send_options_acknowledgment(initial_data_t initial, int socket,command_line_args_t args)
{
    char *buffer = malloc(BUFFER);
    buffer[0] = '\0';
    buffer[1] = 6; //opcode for option acknowledgment
    int buffer_size = 2;

    if(initial.blck_size_option == true)
    {
        strcpy(buffer + buffer_size,"blksize");
        buffer_size += strlen("bklsize") + 1;
        sprintf(buffer + buffer_size, "%d", initial.block_size);
        buffer_size += strlen(buffer + buffer_size) + 1;
    }
    if(initial.t_size_option == true)
    {
        //size of the file that client want to load to server has to be smallet the available space on server
        if(initial.opcode == WRQ)
        {   
            char path[BUFFER] = {'\0'};
            strcat(path,args.root_dir_path);
            if(path[strlen(path) - 1] != '/' )
            {
                strcat(path,"/");
            }
            struct statvfs fs_stat;
            if (statvfs(path, &fs_stat) == 0) {
                printf("Available free space: %llu bytes\n", (unsigned long long)fs_stat.f_bsize * fs_stat.f_bavail);
                if((long int)fs_stat.f_bsize * fs_stat.f_bavail < initial.t_size)
                {
                    send_err_message(socket,ERROR_DISK_FULL,"Too big of a file for server");
                    printf("couldn store file do to its size\n");
                    exit(1);
                }
            } else {
                send_err_message(socket,ERROR_DISK_FULL,"Full disk");
                printf("Full disk\n");
                exit(1);
            }
                strcpy(buffer + buffer_size,"tsize");
                buffer_size += strlen("tsize") + 1;
                sprintf(buffer + buffer_size, "%ld", initial.t_size);
                buffer_size += strlen(buffer + buffer_size) + 1;
        }
        //client wants to know size of the file stored on server
        else
        {
            char filename[BUFFER] = {'\0'};
            strcat(filename,args.root_dir_path);
            if(filename[strlen(filename) - 1] != '/' )
            {
                strcat(filename,"/");
            }
            strcat(filename,initial.file_path);
            printf("%s\n",filename);
            struct stat file_stat;
            if(stat(filename, &file_stat)== -1) {
                printf("stat");
                exit(EXIT_FAILURE);
            }
            long unsigned int size_of_file = file_stat.st_size;
            strcpy(buffer + buffer_size,"tsize");
            buffer_size += strlen("tsize") + 1;
            sprintf(buffer + buffer_size, "%lu", size_of_file);
            buffer_size += strlen(buffer + buffer_size) + 1;
        }

    }
    if(initial.time_out_option == true)
    {
        strcpy(buffer + buffer_size,"timeout");
        buffer_size += strlen("timeout") + 1;
        sprintf(buffer + buffer_size, "%d", initial.time_out);
        buffer_size += strlen(buffer + buffer_size) + 1;
    }

     int message_size;
    message_size = send(socket,buffer,buffer_size,0);
    if (message_size == -1){                   // check if data was sent correctly
      printf("send() failed");}
    else if (message_size != buffer_size)
      printf("send(): buffer written partially");
    else{
        printf("odeslan option ackwledgment");
        return 1;
    }
    return 0;
}

int convert_from_netascii(char *string,int total_size)
{
    if (string == NULL || total_size <= 0) {
        // Invalid input, handle accordingly
        return total_size;
    }
    int new_size = 0;
    char *new_string = malloc(total_size);
    for(int i =0 ; i <total_size; i++)
    {
        if (string[i] == '\r' && string[i+1] == '\n') {
            // Replace '\n' with '\r\n'
            new_string[new_size++] = '\n';
            i++;
        }
        else if(string[i] == '\r' && string[i+1] == '\0')
        {
            new_string[new_size++] = '\r';
            i++;
        }
        else
        {
            new_string[new_size++] = string[i];
        }
    }
    string = new_string;
    return new_size;
}

data_and_size_t server_load_data(int server_socket,initial_data_t initial_data)
{
    int block_size = BLOCK_SIZE;
    if(initial_data.accepted_extensions == true && initial_data.blck_size_option == true){
        block_size = initial_data.block_size;
    }
    data_and_size_t data_and_size;
    data_and_size.data = NULL;
    data_and_size.size = 0;


    //TIME PART #######################################
    fd_set readfds;
    struct timeval tv;

    // Set up the file descriptor set for select
    FD_ZERO(&readfds);
    FD_SET(server_socket, &readfds);

    // Set up the timeout for select
    tv.tv_sec = MAX_WAIT_TIME_SEC;
    tv.tv_usec = 0;
    //TIME PART #######################################


    int block_num;
    unsigned short blocks_received = 0;
    int acknowledgment_send_n_times = 0;
    char buffer[block_size+4];
    int recv_size = block_size + 4;
    while (recv_size == block_size + 4){
        //RECEIVE
        // Wait for data to be available on the socket, or timeout
        int select_result = select(server_socket+1, &readfds, NULL, NULL, &tv);
        if (select_result < 0) {
            printf("ERROR: select");
            exit(EXIT_FAILURE);

        }
        else if (select_result == 0)
        {
            if(acknowledgment_send_n_times >= 3)
                {
                    //send error message
                    //terminate
                    printf("timeout\n");
                    send_err_message(server_socket,ERROR_NOT_DEFINED,"Timeout");
                    exit(EXIT_SUCCESS);
                }
            //timeout
            tv.tv_sec = tv.tv_sec * 2;
            //sned last ackowledgment again      
            send_ack_n(server_socket,blocks_received);
            acknowledgment_send_n_times ++;
            continue;
        }
        tv.tv_sec = MAX_WAIT_TIME_SEC;

        recv_size = recv(server_socket,buffer,block_size + 4,0);
        if(!print_received_message(server_socket,buffer,recv_size)){
            send_err_message(server_socket,ERROR_ILLEGAL_TFTP,"Got wrong opcode");
            exit(EXIT_FAILURE);
        }
        if (buffer[1] == ERROR)
        {
            exit(EXIT_FAILURE);
        }
        else if(buffer[1] == DATA)
        {
            if(blocks_received < 255)
            {
                block_num = (int) ((unsigned char) buffer[3]);
            }
            else
            {   
                block_num = (int) ((unsigned char) buffer[2]) * 255  + (int) ((unsigned char) buffer[3]);
            }
            //data block number match
            if(block_num == blocks_received + 1)
            {
                data_and_size.data = realloc(data_and_size.data,data_and_size.size + recv_size - 4);
                memcpy(data_and_size.data + data_and_size.size,buffer+4,recv_size-4);
                data_and_size.size += recv_size - 4;
                blocks_received++;
                acknowledgment_send_n_times = 0;
                if(!send_ack_n(server_socket,blocks_received))
                {
                    send_err_message(server_socket,ERROR_NOT_DEFINED,"server inner error\n");
                    exit(EXIT_FAILURE);
                }
            }
            else if(block_num <= blocks_received + 1){
                if(acknowledgment_send_n_times >= 3)
                {
                    //send error message
                    //terminate
                    printf("timeout\n");
                    send_err_message(server_socket,ERROR_NOT_DEFINED,"Timeout");
                    exit(EXIT_SUCCESS);
                }
                //send last acknowledgment again
                if(!send_ack_n(server_socket,blocks_received))
                {
                    send_err_message(server_socket,ERROR_NOT_DEFINED,"server inner error\n");
                    exit(EXIT_FAILURE);
                }
                acknowledgment_send_n_times ++;
            }
            else{
                send_err_message(server_socket,ERROR_ILLEGAL_TFTP,"Expected new data block");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            send_err_message(server_socket,ERROR_ILLEGAL_TFTP,"Expected new data block");
            exit(EXIT_FAILURE);
        }
        //send ackonwledgment

    }
    printf("whole file recieved\n");

    if(!strcmp(initial_data.mode,"netascii"))
    {
        //convert to linux coding
        data_and_size.size =  convert_from_netascii(data_and_size.data,data_and_size.size);
    }

    return data_and_size;
    
}

int create_file(command_line_args_t args, initial_and_client_t initial_and_client, data_and_size_t data_and_size)
{
    char file_path_name[BUFFER] = {'\0'};
    if(strcmp(args.root_dir_path,""))
    {
        strcpy(file_path_name,args.root_dir_path);
        //if user gave directory with '/' at the end
        if(args.root_dir_path[strlen(args.root_dir_path)] != '/')
        {
        strcpy(file_path_name + strlen(file_path_name),"/");

        }
    }
    strcpy(file_path_name + strlen(file_path_name),initial_and_client.initial.file_path);
    FILE *fptr;
    fptr = fopen(file_path_name, "w");
    if(fptr == NULL)
    {
        printf("fopen(): ");
        exit(EXIT_FAILURE);
    }
    fprintf(fptr,"%s",data_and_size.data);
    fclose(fptr);
    printf("file saved\n");
    return 1;
}

int recv_data_0(int server_socket)
{
    char buffer[BUFFER];
    //TIME PART #######################################
    fd_set readfds;
    struct timeval tv;

    // Set up the file descriptor set for select
    FD_ZERO(&readfds);
    FD_SET(server_socket, &readfds);

    // Set up the timeout for select
    tv.tv_sec = MAX_WAIT_TIME_SEC;
    tv.tv_usec = 0;
    //TIME PART #######################################

    // Wait for data to be available on the socket, or timeout
    int select_result = select(server_socket+1, &readfds, NULL, NULL, &tv);
    if (select_result < 0) {
        printf("ERROR: select");
        exit(EXIT_FAILURE);

    }

    int recv_size = recv(server_socket,buffer,BUFFER,0);
    if (buffer[1] != ACK || buffer[3] != 0)
    {
        return 0;
    }

    if(!print_received_message(server_socket,buffer,recv_size))
    {
        send_err_message(server_socket,ERROR_ILLEGAL_TFTP,"Got wrong opcode");
        exit(EXIT_FAILURE);
    }

    return 1;
}


int converts_to_netascii(char *string,int total_size)
{
    if (string == NULL || total_size <= 0) {
        // Invalid input, handle accordingly
        return total_size;
    }
    int new_size = 0;
    char *new_string = malloc(total_size+1000);
    for(int i =0 ; i <total_size; i++)
    {
        if (string[i] == '\n') {
            // Replace '\n' with '\r\n'
            new_string[new_size++] = '\r';
            new_string[new_size++] = '\n';
        }
        else if(string[i] == '\r')
        {
            // Replace '\n' with '\r\n'
            new_string[new_size++] = '\r';
            new_string[new_size++] = '\0';
        }
        else
        {
            new_string[new_size++] = string[i];
        }
    }
    string = new_string;
    return new_size;
}


int upload_file(int server_socket,command_line_args_t args,initial_and_client_t initial_and_client)
{
    char *file = NULL;
    char buffer[1000];
    size_t total_size = 0;
    size_t bytes_read;
    FILE *fptr;
    char file_path[BUFFER] = {'\0'};
    strcpy(file_path,args.root_dir_path);
    //check if last char is "/"
    if(file_path[strlen(file_path) - 1] != '/' )
    {
        file_path[strlen(file_path)] = '/';
    }

    strcpy(file_path+strlen(file_path),initial_and_client.initial.file_path);
    if(!strcmp(initial_and_client.initial.mode,"netascii")){
        fptr = fopen(file_path, "r");
    }
    else{
        fptr = fopen(file_path, "rb");
    }
    if(fptr == NULL){
        //send error message
        if (errno == ENOENT) {
            // File does not exist
            printf("File does not exist");
            send_err_message(server_socket,ERROR_FILE_NOT_FOUND,"");
        } else if (errno == EACCES) {
            // Access denied
            printf("Access violation");
            send_err_message(server_socket,ERROR_ACCES_VIOLATION,"");
        } else {
            // Other error
            printf("File open error");
            send_err_message(server_socket,ERROR_NOT_DEFINED,"Problem with opening file");
        }
        exit(EXIT_FAILURE);
    }

    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fptr)) > 0) {
        if(file == NULL)
        {
            file = malloc(total_size+bytes_read+1);
        }
        else{
            file = realloc(file, total_size + bytes_read + 1);
        }
        if (file == NULL) {
            printf("Memory allocation failed");
            //send error message()
            send_err_message(server_socket,ERROR_DISK_FULL,"Memmory allocation failed");
            exit(EXIT_FAILURE);
        }

        // Copy the newly read data to the end of the input buffer
        memcpy(file + total_size, buffer, bytes_read);
        total_size += bytes_read;
    }


    if(!strcmp(initial_and_client.initial.mode,"netasci"))
    {
        total_size =  converts_to_netascii(file,total_size);
    }

    //split file into blocks
    //send block with #number
    //wait for acknwoledgment/error/timeout
    //data packet:  
    //      2B | 2B   | <=512B
    //   opcode|#block|Data
    

    int block_size;
    if(initial_and_client.initial.accepted_extensions == true && initial_and_client.initial.blck_size_option == true)
    {
        block_size = initial_and_client.initial.block_size;
    }
    else{
        block_size = BLOCK_SIZE;
    }
    int final_block_number = (total_size/block_size) + 1;

    int blocks_acknowledget = 0;
    
    int block_send_n_times = 0;

        //TIME PART #######################################
    fd_set readfds;
    struct timeval tv;

    // Set up the file descriptor set for select
    FD_ZERO(&readfds);
    FD_SET(server_socket, &readfds);

    // Set up the timeout for select
    tv.tv_sec = MAX_WAIT_TIME_SEC;
    if(initial_and_client.initial.accepted_extensions == true && initial_and_client.initial.time_out_option == true){
        tv.tv_sec = initial_and_client.initial.time_out;
    }
    //TIME PART #######################################

    char recv_datagram[BUFFER];
    int n_bytes_rcv;
    int bytes_tx;
    int block_num;
    datagram_and_size_t datagram_and_size;
    while(blocks_acknowledget < final_block_number)
    {

        datagram_and_size = get_data_datagram(blocks_acknowledget+1,block_size,file,total_size);

        bytes_tx=send(server_socket,datagram_and_size.datagram,datagram_and_size.size+4,0);
        printf("send data block #%d\n",blocks_acknowledget + 1); fflush(stdout);
        if (bytes_tx < 0) {
           printf("ERROR: send");
            send_err_message(server_socket,ERROR_NOT_DEFINED,"server inner error\n");
            exit(EXIT_FAILURE);
        }


        //RECEIVE
        // Wait for data to be available on the socket, or timeout
        int select_result = select(server_socket+1, &readfds, NULL, NULL, &tv);
        if (select_result < 0) {
            printf("ERROR: select");
            send_err_message(server_socket,ERROR_NOT_DEFINED,"server inner error\n");
            exit(EXIT_FAILURE);

        }
        else if (select_result == 0)
        {
            //timeout
            tv.tv_sec = tv.tv_sec * 2;
            //send packet again up to 3 times
            block_send_n_times ++;
            if(block_send_n_times >= 3)
            {
                //terminate
                //send error message
                printf("timeout\n");
                send_err_message(server_socket,ERROR_NOT_DEFINED,"Timeout");
                exit(EXIT_FAILURE);
            }
            continue;
        }
        else
        {
            //set timetou back to original number
            tv.tv_sec = MAX_WAIT_TIME_SEC;
            if(initial_and_client.initial.accepted_extensions == true && initial_and_client.initial.time_out_option == true){
                tv.tv_sec = initial_and_client.initial.time_out;
            }

            if(block_send_n_times >= 3)
            {
                //terminate
                //send error message
                printf("timeout\n");
                send_err_message(server_socket,ERROR_NOT_DEFINED,"Timeout");
                exit(EXIT_FAILURE);
            }
        }
        //check if i got acknowledgment or error
        n_bytes_rcv = recv(server_socket,recv_datagram,BUFFER,0);
        if(!print_received_message(server_socket,recv_datagram,n_bytes_rcv))
        {
            send_err_message(server_socket,ERROR_ILLEGAL_TFTP,"Got wrong opcode");
            exit(EXIT_FAILURE);
        }
        if(recv_datagram[1] == ERROR)
        {
            //terminate
            exit(EXIT_FAILURE);
        }
        else if (recv_datagram[1] == ACK)
        {
            //check if block numbers match
            //if  block number does match, react same as in timeout
            if (blocks_acknowledget >= 255)
            {
                block_num = (int) ((unsigned char) recv_datagram[2]) * 255  + (int) ((unsigned char) recv_datagram[3]);
                
            }
            else{
                block_num = (int) ((unsigned char) recv_datagram[3]);
            }
            if(block_num ==  blocks_acknowledget + 1)
            {
                blocks_acknowledget++;
            }
            else if(block_num <= blocks_acknowledget)
            {
                    block_send_n_times ++;
                    if(block_send_n_times >= 3)
                    {
                        //terminate
                        //send error message
                        printf("timeout\n");
                        send_err_message(server_socket,ERROR_NOT_DEFINED,"Timeout");
                        exit(EXIT_FAILURE);
                    }
                    continue;
            }
            else
            {
                send_err_message(server_socket,ERROR_NOT_DEFINED,"Expected acknowledgment");
                exit(EXIT_FAILURE); 
            }

        }
        else
        {
            block_send_n_times ++;
        }


        block_send_n_times = 0;
    }

    //file was succesfuly send
    printf("file send succesfully\n");
    // Clean up allocated memory
    free(file);
    return 1;
}

int file_already_exist(command_line_args_t args,initial_and_client_t initial_and_client)
{
    char file_path_name[BUFFER] = {'\0'};
    if(strcmp(args.root_dir_path,""))
    {
        strcpy(file_path_name,args.root_dir_path);
        //if user gave directory with '/' at the end
        if(args.root_dir_path[strlen(args.root_dir_path)] != '/')
        {
        strcpy(file_path_name + strlen(file_path_name),"/");

        }
    }
    strcpy(file_path_name + strlen(file_path_name),initial_and_client.initial.file_path);

    if (access(file_path_name, F_OK) != -1) {
        return 1;
    }
    else{
        return 0;
    }
}

int main(int argc, char *argv[]){
    signal(SIGINT,handler);
    command_line_args_t args = parse_arguments(argc,argv);

    initial_and_client_t initial_and_client = waiting_for_initial(args);

    //new socket, with new port specidfied by TID
    int server_socket;
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket <= 0)
    {
        printf("ERROR: socket");
        exit(EXIT_FAILURE);
    }

    if (connect(server_socket, (struct sockaddr *)&(initial_and_client.client_address), sizeof(initial_and_client.client_address))  == -1)
    printf("connect() failed");

    if(initial_and_client.initial.err)
    {
        send_err_message(server_socket,initial_and_client.initial.err,"Fault request message");
        exit(EXIT_FAILURE);
    }
    else if (! strcmp(initial_and_client.initial.mode,"mail"))
    {
        send_err_message(server_socket,ERROR_ILLEGAL_TFTP,"Mail mode is not supported");
        exit(EXIT_FAILURE);
    }


    
    //WRQ
    // server respond with 0ACK or ACK or ERROR
    if (initial_and_client.initial.opcode == WRQ)
    {
        if(file_already_exist(args,initial_and_client)){
            send_err_message(server_socket,ERROR_FILE_ALREADY_EXIST,"file already exists");
            exit(EXIT_FAILURE);
        }

        //error pro denied, error8 pro options
        //0ACK
        if(initial_and_client.initial.accepted_extensions == true){
            if(!send_options_acknowledgment(initial_and_client.initial,server_socket,args)){
                exit(EXIT_FAILURE);
        }

        }
        //send acknowledgment block ACK 0
        else if(send_ack_n(server_socket,0))
        {
        }
        else
        {
            exit(EXIT_FAILURE);
        }
        // server loads data
        data_and_size_t data_and_size = server_load_data(server_socket,initial_and_client.initial);
        //ulozim soubor
        create_file(args,initial_and_client,data_and_size);
    }
    else if (initial_and_client.initial.opcode == RRQ)
    {

        //server can respond with ERR, 0ACk, or DATA
        
        //kdyz byli extensions, poslu 0ACK, obdzim DATA0
        if(initial_and_client.initial.accepted_extensions == true){
            if(!send_options_acknowledgment(initial_and_client.initial,server_socket,args)){
                exit(EXIT_FAILURE);
            }
            if(!recv_data_0(server_socket))
            {
                //mozna poslu error msg
                exit(EXIT_FAILURE);
            }
        }
        //potom normalne posilam soubor
        upload_file(server_socket,args,initial_and_client);
    }   
    //error
    else{
        printf("Expected request, got something else\n");
        exit(EXIT_FAILURE);
    }



    return 0;
}