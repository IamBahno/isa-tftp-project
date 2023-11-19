// autor: Ondřej Bahounek, xbahou00

#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5
#define OACK 6

#define BLOCK_SIZE 512

#define ERROR_NOT_DEFINED 0
#define ERROR_FILE_NOT_FOUND 1
#define ERROR_ACCES_VIOLATION 2
#define ERROR_DISK_FULL 3
#define ERROR_ILLEGAL_TFTP 4
#define ERROR_UNKNOWN_TRANSFER_ID 5
#define ERROR_FILE_ALREADY_EXIST 6
#define ERROR_NO_SUCH_USER 7
#define ERROR_OPTION_NEGATION 8


typedef struct{
    char *datagram;
    int size;
}datagram_and_size_t;

//send acknowledgment n to socket the int socket is connected with 
int send_ack_n(int socket,unsigned n);

// takes block_number (starting from 1),file is pointer to string
// returns size of data block pointed to by new_datagram
// (returns size of data block, not data_block + 4)
datagram_and_size_t get_data_datagram(int block_number,int block_size,char *file,int file_size);

//print DATA message in format given by assigment
int print_data_rcv_message(int block_number,int receiver_socket);

//print ACK message in format given by assigment
int print_ack_message(int block_number,int receiver_socket);

//send ERROR with code error_code and message error_message, to the socket connected with sender_socket
int send_err_message(int sender_socket,int error_code,const char* error_message);

//print ERROR message in format given by assigment
int print_error_message(int receiver_socket,char* message);

//print 0ACK message in format given by assigment
int print_0ack(int receiver_socket,char* message,int len);

//check type of message receive and call given function to print it
int print_received_message(int receiver_socket,char * message,int len);

