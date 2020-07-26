#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctype.h>
#include "json.hpp" 

#define MAXSIZE 512

extern const std::string getCurrentDateTime();
extern void argument_error_handler(std::string message);
extern void error_handler(std::string message);
std::string clientName;
std::string groupID;
using json = nlohmann::json;

int connectToServer(char* IPaddress, int port){
  struct sockaddr_in serverAddr;
  char buffer[1024];

  int clientFd = socket(AF_INET, SOCK_STREAM, 0);
  if(clientFd < 0)
    error_handler("Error in function socket(): Couldn't create socket!");   

  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr(IPaddress);

  if(connect(clientFd, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0)
    error_handler("Error in connectToServer()!\n");
  
  sprintf(buffer, "CONNECTED TO SERVER!\n\tServer address: '%s:%hd'\n\tTime: %s\n", inet_ntoa (serverAddr.sin_addr), ntohs (serverAddr.sin_port), getCurrentDateTime().c_str());
  write(1, buffer, strlen(buffer));
  return clientFd;
}

void closeConnection(int fd){
  write(1, "Connection has been closed!\n", 28);
  close(fd);
  exit(0);
}

void recvMessage(int fd){
  char buffer[MAXSIZE];
  int readBytes;

  readBytes = read(fd, buffer, MAXSIZE);
  if(readBytes < 0)
    error_handler("Error in read()!");
  else if(readBytes == 0)
    closeConnection(fd);
  else{
    buffer[readBytes] = '\0';
    auto messageJson = json::parse(std::string(buffer));
    std::string clientName = messageJson["clientName"];
    std::string timestamp = messageJson["timestamp"];
    std::string messageContent = messageJson["messageContent"];
    std::string message = "NEW MESSAGE RECEIVED\nSender: " + clientName + "\nTimestamp: " + timestamp + "\nMessage: " + messageContent + "\n"; 
    write(1, message.c_str(), strlen(message.c_str())); 
  }
}

void sendMessage(int fd){
  char buffer[MAXSIZE];
  int readBytes;

  readBytes = read(0, buffer, MAXSIZE);
  if(readBytes < 0)
    error_handler("Error in read()!");
  else if(readBytes == 0)
    closeConnection(fd);
  else{
    buffer[readBytes-1] = '\0';
    json messageJson;
    messageJson["command"] = "chatMessage";
    messageJson["groupID"] = groupID;
    messageJson["clientName"] = clientName;
    messageJson["timestamp"] = std::string(getCurrentDateTime());
    messageJson["messageContent"] = std::string(buffer);
    std::string message = messageJson.dump();
    printf("%s", message.c_str());
    write(fd, message.c_str(), strlen(message.c_str()));
  }
}

void handleMessages(int clientFd){
  fd_set readFd_set;
  int maxNumberFd = clientFd;

  while(1){    
    FD_ZERO(&readFd_set);
    FD_SET(clientFd, &readFd_set);
    FD_SET(0, &readFd_set);

    if(select (maxNumberFd+1, &readFd_set, NULL, NULL, NULL) < 0)
      error_handler("Error in select()!");

    /* Serve all the sockets with input pending */
    for (int i = 0; i <= maxNumberFd; ++i){
      if (FD_ISSET (i, &readFd_set)){
        if (i == clientFd)
          recvMessage(clientFd);
        else
          sendMessage(clientFd);
      }
    }
  }
}

bool joinToHousehold(int clientFd){
  json requestJson;
  requestJson["command"] = "joinToHousehold";
  requestJson["groupID"] = std::string(groupID);
  std::string message = requestJson.dump();
  write(clientFd, message.c_str(), strlen(message.c_str()));
  
  /* Wait for response from server */
  char buffer[MAXSIZE];
  int readBytes;
  readBytes = read(clientFd, buffer, MAXSIZE);
  if(readBytes < 0)
    error_handler("Error in read()!");
  else if(readBytes == 0)
    closeConnection(clientFd);
  else{
    buffer[readBytes] = '\0';
    auto responseJson = json::parse(std::string(buffer));
    std::string command = responseJson["command"];
    if(command == "joinToHousehold")
    {
      std::string responseStatus = responseJson["status"];
      if(responseStatus == "accepted")
        return true;
      else{
        std::string reason = responseJson["reason"];
        reason = "Rejection reason: " + reason + "\n";
        write(1, reason.c_str(), strlen(reason.c_str()));
        return false;
      }
    }
  }
  return false;
}

void runClient(char* IPaddress, int port){
  int clientFd = connectToServer(IPaddress, port);
  if(joinToHousehold(clientFd))
    handleMessages(clientFd);
  else
    closeConnection(clientFd);
}

int main(int argc, char* argv[]){
  int port;
  char* IPaddress = argv[1];

  /* Check valid arguments */
  if(argc != 5)
    argument_error_handler("Error in main(): Invalid number of arguments! Program expects IP adress and port number!");
  if((port = atoi(argv[2])) <= 0)
    argument_error_handler("Error in main(): Passed argument must be number!");
  groupID = std::string(argv[3]);
  clientName = std::string(argv[4]);
  runClient(IPaddress, port);
}
