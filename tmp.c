#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


int converts_from_netascii(char *string,int total_size)
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

int main()
{
    char *string = malloc(100);
    strcpy(string,"ADAM\nONDRA\rBLBEC");
    int size=strlen(string) +1;
    // printf("%d%s\n",strlen(string),string);
    // printf("__");
    // char *tmp = string;
    // while(*tmp){
    //     printf("_%c_\n",*tmp);
    //     tmp++;
    // }
        printf("__");
    char *tmp = string;
    while(*tmp){
        printf("_%c_\n",*tmp);
        tmp++;
    }
    size=converts_to_netascii(string,size);
    //     printf("__\n");
    // char *tmp = string;
    // while(*tmp){
    //     printf("_%c_\n",*tmp);
    //     tmp++;
    // }
    // printf("%d%s\n",strlen(string),string);
    // printf("__");

    size=converts_from_netascii(string,size);
    // printf("__");
    // tmp = string;
    // while(*tmp){
    //     printf("_%c_\n",*tmp);
    //     tmp++;
    // }
    // // printf("%d%s\n",strlen(string),string);

    return 0;
}
