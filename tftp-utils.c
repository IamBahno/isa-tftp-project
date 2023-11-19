// autor: Ondřej Bahounek, xbahou00

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include "tftp-utils.h"
#include <arpa/inet.h> 
#include <netinet/in.h>

#define BUFFER 512

int send_ack_n(int socket,unsigned n)
{
    char buffer[5] = {'\0'};
    buffer[1] = ACK;
    if(n <= 255)
    {
        buffer[3] = n;
    }
    else
    {
        buffer[2] = n/255;
        buffer[3] = n % 255;
    }
    int message_size = send(socket,buffer,4,0);
    if (message_size == -1){                   // check if data was sent correctly
      printf("send() failed");
      return 0;}
    else if (message_size != 4){
      printf("send(): buffer written partially");
      return 0;}
    else{
        printf("send acknwodledgment: %d\n",n);
        return 1;
    }
}


datagram_and_size_t get_data_datagram(int block_number,int block_size,char *file,int file_size)
{
    char datagram[4] = {'\0'};

    datagram[1] = DATA;

    if(block_number > 255)
    {
        datagram[2] = block_number/255;
        datagram[3] = block_number % 255;
    }
    else{
        datagram[3] = block_number;
    }



    //sending not 512
    if(block_number*block_size > file_size)
    {
        // strncpy(datagram+4,file + block_size*(block_number - 1),file_size -(block_number-1)*block_size);
        char * new_datagram = (char *) malloc(file_size -(block_number-1)*block_size + 4);
        memcpy(new_datagram,datagram,4);
        memcpy(new_datagram+4,file + block_size*(block_number - 1),file_size -(block_number-1)*block_size+4);
        datagram_and_size_t neco;
        neco.size = file_size -(block_number-1)*block_size;
        neco.datagram = new_datagram;
        return neco; 
        // return file_size -(block_number-1)*block_size;
    }
    else
    {
        datagram_and_size_t neco;
        neco.datagram = malloc(block_size+4);
    
        memcpy(neco.datagram,datagram,4);
        memcpy(neco.datagram+4,file+block_size*(block_number-1),block_size);
        neco.size = block_size;
        return neco;
    }


}


int print_data_rcv_message(int block_number,int receiver_socket)
{
    char buffer[BUFFER] = {"\0"};
    strcpy(buffer,"DATA ");

    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if (getpeername(receiver_socket, (struct sockaddr *)&sender_addr, &addr_len) == -1) {
        printf("getpeername failed");
        exit(1);
    }

    struct sockaddr_in receiver_addr;

    if (getsockname(receiver_socket, (struct sockaddr *)&receiver_addr, &addr_len) == -1) {
        printf("getsockname failed");
        exit(1);
    }

    strcat(buffer,inet_ntoa(sender_addr.sin_addr));
    strcat(buffer,":");
    char port[20];
    sprintf(port, "%d", ntohs(sender_addr.sin_port));
    strcat(buffer,port);
    strcat(buffer,":");
    sprintf(port, "%d", ntohs(receiver_addr.sin_port));
    strcat(buffer,port);
    strcat(buffer," ");

    sprintf(port, "%d", block_number);

    strcat(buffer,port);
    strcat(buffer,"\n");
    fprintf(stderr,"%s",buffer);


    return 1;

}

int print_ack_message(int block_number,int receiver_socket)
{
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if (getpeername(receiver_socket, (struct sockaddr *)&server_addr, &addr_len) == -1) {
        printf("getpeername failed");
        exit(1);
    }
    char message[BUFFER] = {'\0'};
    strcpy(message,"ACK ");
    strcat(message,inet_ntoa(server_addr.sin_addr));
    strcat(message,":");
    char port[20];
    sprintf(port, "%d", ntohs(server_addr.sin_port));
    strcat(message,port);
    strcat(message," ");
    sprintf(port, "%d", block_number);

    strcat(message,port);
    strcat(message,"\n");
    fprintf(stderr,"%s",message);

    return 1;

}

// int print_err_message(int receiver_socket,)
int send_err_message(int sender_socket,int error_code,const char* error_message)
{
    char *buffer = calloc(2+2+strlen(error_message)+1,sizeof(char));
    buffer[1] = ERROR;
    buffer[3] = error_code;
    strcpy(buffer+4,error_message);
    int message_size = send(sender_socket,buffer,5+strlen(error_message),0);
    if (message_size == -1){                   // check if data was sent correctly
      printf("send() failed");
      return 0;}
    else if (message_size != 5+(long signed) strlen(error_message)){
      printf("send(): buffer written partially");
      return 0;}
    else{
        printf("send error:%d %s\n",error_code,error_message);
        return 1;
    }
}

int print_error_message(int receiver_socket,char* message)
{
    char buffer[BUFFER] = {"\0"};
    strcpy(buffer,"ERROR ");

    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if (getpeername(receiver_socket, (struct sockaddr *)&sender_addr, &addr_len) == -1) {
        printf("getpeername failed");
        exit(1);
    }

    struct sockaddr_in receiver_addr;

    if (getsockname(receiver_socket, (struct sockaddr *)&receiver_addr, &addr_len) == -1) {
        printf("getsockname failed");
        exit(1);
    }

    strcat(buffer,inet_ntoa(sender_addr.sin_addr));
    strcat(buffer,":");
    char port[20];
    sprintf(port, "%d", ntohs(sender_addr.sin_port));
    strcat(buffer,port);
    strcat(buffer,":");
    sprintf(port, "%d", ntohs(receiver_addr.sin_port));
    strcat(buffer,port);
    strcat(buffer," ");
    char error_code[5];
    sprintf(error_code, "%d", message[3]);
    strcat(buffer,error_code);
    strcat(buffer," \"");
    strcat(buffer,message+4);
    strcat(buffer,"\"\n");

    fprintf(stderr,"%s",buffer);

    return 1;

}

int print_0ack(int receiver_socket,char* message,int len)
{
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if (getpeername(receiver_socket, (struct sockaddr *)&server_addr, &addr_len) == -1) {
        printf("getpeername failed");
        exit(1);
    }
    char buffer[BUFFER] = {'\0'};
    strcpy(buffer,"ACK ");
    strcat(buffer,inet_ntoa(server_addr.sin_addr));
    strcat(buffer,":");
    char port[20];
    sprintf(port, "%d", ntohs(server_addr.sin_port));
    strcat(buffer,port);
    int bytes_parsed = 1;
    while(bytes_parsed < len)
    {
        strcat(buffer," ");
        strcat(buffer,message+bytes_parsed);    
        bytes_parsed+=strlen(message+bytes_parsed) + 1;
        strcat(buffer,"=");
        strcat(buffer,message+bytes_parsed);    
        bytes_parsed+=strlen(message+bytes_parsed) + 1;
    }
    strcat(buffer,"\n");
    fprintf(stderr,"%s",buffer);
    return 0;

}

int print_received_message(int receiver_socket,char * message,int len){
    if(message[1] == RRQ)
    {
        return 0;
    }
    else if(message[1] == WRQ)
    {
        return 0;
    }
    else if(message[1] == DATA)
    {
        print_data_rcv_message(256*((unsigned char) message[2]) + (unsigned char) message[3],receiver_socket);
    }  
    else if(message[1] == ACK)
    {
        print_ack_message(256*((unsigned char) message[2]) + (unsigned char) message[3],receiver_socket);
    }
    else if(message[1] == ERROR)
    {
        print_error_message(receiver_socket,message);
    }
    else if(message[1] == OACK)
    {
        print_0ack(receiver_socket,message,len);
    }
    else
    {
        return 0;
    }
    return 1;
}

