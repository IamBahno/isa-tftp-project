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
#include <sys/select.h>
#include <sys/time.h>
#include "tftp-utils.h"
#include "tftp-client.h"
#include <unistd.h> 

//TODO merlin test
//TODO kdyz prijde paket ktery je neocekavany a neni to zatoulany ACK tak poslu error a termiante (misto posilani znovu)


#define MAX_WAIT_TIME_SEC 3


#define BUFFER 1024

int glob_socket;




//for handling Ctrl+c
void handler()
{
    exit(EXIT_SUCCESS);
}

command_line_args_t parse_arguments(int argc,char *argv[])
{
    if(argc != 5 && argc != 7 && argc != 9)
    {
        printf("Wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    command_line_args_t args;
    memset(args.hostname, '\0', sizeof(args.hostname));
    args.port = 69; //default TFTP port
    memset(args.filepath, '\0', sizeof(args.filepath));
    memset(args.dest_filepath, '\0', sizeof(args.dest_filepath));



    int opt;
    while ((opt = getopt(argc, argv, "h:p:f:t:")) != -1) {
        switch (opt) {
            case 'h':
                strcpy(args.hostname,optarg);
                break;
            case 'p':
                args.port = atoi(optarg);
                if (args.port == 0){
                    printf("Error: port has to be a number\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'f':
                strcpy(args.filepath,optarg);
                break;
            case 't':
                strcpy(args.dest_filepath,optarg);
                break;
            default:
                printf("Usage: %s -h hostname [-p port] [-f filepath] -t dest_filepath\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if(strcmp(args.filepath,"") == 0)
    {
        args.opcode = WRQ;
    }
    else{
        args.opcode = RRQ;
    }
    return args;
}

int send_initial_packet(int client_socket,command_line_args_t args,struct sockaddr *server,int server_size)
{


    // 2bytes + len(filename) + byte + len(mode) + byte + extension

    //OPCODE
    int buffer_size = 2;
    char *buffer = malloc(2);

    char opcode;
    if (args.opcode == WRQ){
        opcode = 2;
    }
    else{
        opcode = 1;
        // if(access(args.dest_filepath, F_OK) != -1)
        // {
        //     printf("File with same name already exist");
        //     exit(EXIT_FAILURE);
        // }
    }
    buffer[0] = '\0';
    buffer[1] = opcode;
    // filename + \0
    buffer = realloc(buffer,buffer_size + strlen(args.dest_filepath) + 1);
    if(args.opcode == WRQ)
    {
        strcpy(buffer+2,args.dest_filepath);
        buffer_size += strlen(args.dest_filepath) + 1;
    }
    else //RRQ
    {
        //in RRQ -f filepath, is path to file at the server
        //        -t desst_file_path is where to store the file, and its name
        strcpy(buffer+2,args.filepath);
        buffer_size += strlen(args.filepath) + 1;
    }

    //mode + \0
    buffer = realloc(buffer,buffer_size + strlen("octet") + 1);
    strcpy(buffer+buffer_size,"octet");
    buffer_size += strlen("octet") + 1;

    // pro testovani serveru
            // tsize + \0 + size + /0
    // buffer = realloc(buffer,buffer_size + strlen("timeout") + 1);
    // strcpy(buffer+buffer_size,"timeout");
    // buffer_size += strlen("timeout") + 1;
    // buffer = realloc(buffer,buffer_size + strlen("6") + 1);
    // strcpy(buffer+buffer_size,"6");
    // buffer_size += strlen("6")+1;


    // //     //blksize + \0 + size + /0
    // buffer = realloc(buffer,buffer_size + strlen("blksize") + 1);
    // strcpy(buffer+buffer_size,"blksize");
    // buffer_size += strlen("blksize") + 1;
    // buffer = realloc(buffer,buffer_size + strlen("1500") + 1);
    // strcpy(buffer+buffer_size,"1500");
    // buffer_size += strlen("1500")+1;


    int message_size;
        message_size = sendto(client_socket,buffer,buffer_size,0,server,server_size);
    if (message_size == -1){                   // check if data was sent correctly
      printf("send() failed");
      return 0;}
    else if (message_size != buffer_size){
      printf("send(): buffer written partially");
      return 0;}
    else{
        printf("request send\n");
        return 1;
    }
    
}



int upload_file(int client_socket,char *first_message,int message_size)
{
    //got error
    if(message_size != 4 || first_message[1] == ERROR)
    {
        //terminate
        exit(EXIT_FAILURE);
    }
        //got ack block 0
    if(!(first_message[1] == ACK && first_message[3] == 0))
    {
        //send error, terminate
        send_err_message(client_socket,ERROR_ILLEGAL_TFTP,"Expected acknowledgment 0, got someting else...\n");
        exit(EXIT_FAILURE);
    }

    //upload    
    char *file = NULL;
    char buffer[1000];
    size_t total_size = 0;
    size_t bytes_read;

    //read file from stdin, dont put '\0' at the end
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
        if(file == NULL)
        {
            file = malloc(total_size+bytes_read+1);
        }
        else{
            file = realloc(file, total_size + bytes_read + 1);
        }
        if (file == NULL) {
            printf("Memory allocation failed");
            free(file);
            //send error message()
            send_err_message(client_socket,ERROR_DISK_FULL,"Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        // Copy the newly read data to the end of the input buffer
        memcpy(file + total_size, buffer, bytes_read);
        total_size += bytes_read;
    }


    //split file into blocks
    //send block with #number
    //wait for acknwoledgment/error/timeout
    //data packet:  
    //      2B | 2B   | <=512B
    //   opcode|#block|Data
    
    int final_block_number = (total_size/BLOCK_SIZE) + 1;
    int blocks_acknowledget = 0;

    
    int block_send_n_times = 0;

    //TIME PART #######################################
    fd_set readfds;
    struct timeval tv;

    // Set up the file descriptor set for select
    FD_ZERO(&readfds);
    FD_SET(client_socket, &readfds);

    // Set up the timeout for select
    tv.tv_sec = MAX_WAIT_TIME_SEC;
    tv.tv_usec = 0;
    //TIME PART #######################################
    char recv_datagram[BUFFER];
    int n_bytes_rcv;
    int bytes_tx;
    int block_num;
    datagram_and_size_t datagram_and_size;
    while(blocks_acknowledget < final_block_number)
    {
        datagram_and_size = get_data_datagram(blocks_acknowledget+1,BLOCK_SIZE,file,total_size);

        bytes_tx=send(client_socket,datagram_and_size.datagram,datagram_and_size.size+4,0);
        if (bytes_tx < 0) {
           printf("ERROR: send");
            exit(EXIT_FAILURE);
        }


        //RECEIVE
        // Wait for data to be available on the socket, or timeout
        int select_result = select(client_socket+1, &readfds, NULL, NULL, &tv);
        if (select_result < 0) {
            printf("ERROR: select");
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
                send_err_message(client_socket,ERROR_NOT_DEFINED,"Timeout");
                exit(EXIT_FAILURE);
            }
            continue;
        }
        tv.tv_sec = MAX_WAIT_TIME_SEC;
        //check if i got acknowledgment or error
        n_bytes_rcv = recv(client_socket,recv_datagram,BUFFER,0);
        if(!print_received_message(client_socket,recv_datagram,n_bytes_rcv)){
                    send_err_message(client_socket,ERROR_ILLEGAL_TFTP,"Got wrong opcode");
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
            //if block number is bigger than 255 i have to check the number with different statement, since its stored accross 2 bytes
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
                        send_err_message(client_socket,ERROR_NOT_DEFINED,"Timeout");
                        exit(EXIT_FAILURE);
                    }
                    continue;
            }
            else
            {
                send_err_message(client_socket,ERROR_NOT_DEFINED,"Expected acknowledgment");
                exit(EXIT_FAILURE); 
            }

        }
        else
        {
            send_err_message(client_socket,ERROR_ILLEGAL_TFTP,"epected acknowledgment ");
            exit(EXIT_FAILURE);
        }


        block_send_n_times = 0;
    }

    //file was succesfuly send

    // Clean up allocated memory
    free(file);
    return 1;
}



data_and_size_t dowload_file(int client_socket,char *first_message,int message_size)
{
    //got error
    if(first_message[1] == ERROR)
    {
        exit(EXIT_FAILURE);
    }
    //expects data block 1
    if(!(first_message[1] == DATA && first_message[3] == 1))
    {
        //send error, terminate
        send_err_message(client_socket,ERROR_ILLEGAL_TFTP,"Expected acknowledgment 0, got someting else...\n");
        exit(EXIT_FAILURE);
    }
    else{
    }

    //TIME PART #######################################
    fd_set readfds;
    struct timeval tv;

    // Set up the file descriptor set for select
    FD_ZERO(&readfds);
    FD_SET(client_socket, &readfds);

    // Set up the timeout for select
    tv.tv_sec = MAX_WAIT_TIME_SEC;
    tv.tv_usec = 0;


    //TIME PART #######################################

    int block_size = BLOCK_SIZE;
    data_and_size_t data_and_size;
    data_and_size.data = malloc(block_size);
    memcpy(data_and_size.data,first_message+4,message_size-4);
    data_and_size.size = message_size-4;

    unsigned short blocks_received = 1;
    int acknowledgment_send_n_times = 0;
    char buffer[block_size+4];
    int recv_size = message_size;

    int block_num;
    while(1)
    {
        if(acknowledgment_send_n_times >= 4){
            //timeout
            printf("timeout\n");
            send_err_message(client_socket,ERROR_NOT_DEFINED,"Timeout");
            exit(EXIT_FAILURE);
        }
        if(!send_ack_n(client_socket,blocks_received))
        {
            exit(EXIT_FAILURE);
        }
        else{
            acknowledgment_send_n_times++;
        }



        //check if data i acknowledget was last
        if(recv_size < block_size + 4)
        {
            break;
        }

        //RECEIVE
        // Wait for data to be available on the socket, or timeout
        int select_result = select(client_socket+1, &readfds, NULL, NULL, &tv);
        if (select_result < 0) {
            printf("ERROR: select");
            exit(EXIT_FAILURE);

        }
        else if (select_result == 0)
        {
            //timeout exponentialy getting bigger
            tv.tv_sec = tv.tv_sec * 2;
            //send last ackowledgment again   
            continue;
        }

        tv.tv_sec = MAX_WAIT_TIME_SEC;


        recv_size = recv(client_socket,buffer,block_size + 4,0);
        if(!print_received_message(client_socket,buffer,recv_size)){
            send_err_message(client_socket,ERROR_ILLEGAL_TFTP,"Got wrong opcode");
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

            }
            else if(block_num <= blocks_received){
                continue;
            }
            else
            {
                send_err_message(client_socket,ERROR_NOT_DEFINED,"New Data block");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            continue;
        }
    }


    return data_and_size;
}

int create_file(command_line_args_t args, data_and_size_t data_and_size)
{
    char file_path_name[BUFFER] = {'\0'};
    strcpy(file_path_name,args.dest_filepath);
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


int main(int argc, char *argv[]){
    command_line_args_t args = parse_arguments(argc,argv);
    socklen_t len;        

    int client_socket;
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    glob_socket = client_socket;
    if (client_socket <= 0)
    {
        printf("socket(): ");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT,handler);

    struct hostent *servent;         // network host entry required by gethostbyname()
    // make DNS resolution of the first parameter using gethostbyname()
    if ((servent = gethostbyname(args.hostname)) == NULL) // check the first parameter
        printf("gethostbyname() failed\n");

    struct sockaddr_in server;

    // copy the first parameter to the server.sin_addr structure
    memset(&server,0,sizeof(server)); // erase the server structure
    server.sin_family = AF_INET;    
    memcpy(&server.sin_addr,servent->h_addr_list[0],servent->h_length); 

    server.sin_port = htons(args.port);   
    len = sizeof(server);


    if(!send_initial_packet(client_socket,args,(struct sockaddr *)&server,len))
    {
        exit(EXIT_FAILURE);
    }

    //TIME PART #######################################
    fd_set readfds;
    struct timeval tv;

    // Set up the file descriptor set for select
    FD_ZERO(&readfds);
    FD_SET(client_socket, &readfds);

    // Set up the timeout for select
    tv.tv_sec = MAX_WAIT_TIME_SEC;
    tv.tv_usec = 0;
    //TIME PART #######################################


    //RECEIVE
    // Wait for data to be available on the socket, or timeout
    int select_result = select(client_socket+1, &readfds, NULL, NULL, &tv);
    if (select_result < 0) {
        printf("ERROR: select");
        exit(EXIT_FAILURE);

    }
    else if (select_result == 0)
    {
        printf("ERROR: No data received within %d seconds.\n", MAX_WAIT_TIME_SEC);
        exit(EXIT_FAILURE);
    }

    char *buffer = malloc(BLOCK_SIZE+4);
    struct sockaddr_in new_server;
    socklen_t length = sizeof(new_server);
    // i receive the first packet with recvfrom, because the  i want know from which port it came
    int bytes_rx = recvfrom(client_socket, buffer, BLOCK_SIZE + 4, 0, (struct sockaddr *)&new_server, &length);
    // connected UDP to the new server (changed port)
    if (connect(client_socket, (struct sockaddr *)&new_server, sizeof(server))  == -1)
    printf("connect() failed");
    
    if(!print_received_message(client_socket,buffer,bytes_rx)){
        send_err_message(client_socket,ERROR_ILLEGAL_TFTP,"Got wrong opcode");
        exit(EXIT_FAILURE);
    }

    if(args.opcode == WRQ)
    {
        //upload file
        upload_file(client_socket,buffer,bytes_rx);
    }
    else if(args.opcode == RRQ)
    {
        //download file
        data_and_size_t data_and_size = dowload_file(client_socket,buffer,bytes_rx);
        //create file
        create_file(args,data_and_size);
    }
    return 0;
}