#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "potato.h"
#include <sys/time.h>
#include <vector>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
  cout << "Potato Ringmaster" << endl;
  int status;
  int socket_fd;
  struct addrinfo host_info; //host address information
  struct addrinfo *host_info_list;
  char hostname[256];
  gethostname(hostname, 256);
  if (argc != 4) {
    cerr << "Error: port number, number of players, number of hops" << endl;
    return 0;
  }
  const char *port = argv[1];
  int player_num = atoi(argv[2]);
  int hop_num = atoi(argv[3]);
  if (hop_num > 512 || hop_num < 0) {
    cerr << "Error: hop number must be less or equal than 512" << endl;
    return -1;
  }
  if (player_num <= 1) {
    cerr << "number of player must be greater than 0" << endl;
    return -1;
  }
  //  cout << "Potato Ringmaster" << endl;
  cout << "Players = " << player_num << endl;
  cout << "Hops = " << hop_num << endl;
  memset(&host_info, 0, sizeof(host_info));  //fill a block of memory with a particular value
  host_info.ai_family   = AF_UNSPEC; 
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) { //success returns zero
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } 
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol); // fake socket
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } 
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } 

  status = listen(socket_fd, player_num);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl; 
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } 
  //  cout << "Ringmaster waiting for connection on port " << port << endl;
  struct sockaddr_storage socket_addr; // used for accept
  socklen_t socket_addr_len = sizeof(socket_addr);
  
  // important part!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  srand(hop_num + player_num);
  int first_player = ((rand()) % player_num);
  struct players player_connection_fd[player_num];
  for (int i = 0; i < player_num; i ++) {
    player_connection_fd[i].socket = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len); // real socket
    if (player_connection_fd[i].socket == -1) {
      cerr << "Error: cannot accept connection on socket" << endl;
      return -1;
    }
    send(player_connection_fd[i].socket, (char *)&first_player, sizeof(first_player), 0);
    send(player_connection_fd[i].socket, (char *)&player_num, sizeof(player_num), 0); // send # of players
    send(player_connection_fd[i].socket, (char *)&i, sizeof(i), 0); //id
    send(player_connection_fd[i].socket, (char *)&hop_num, sizeof(hop_num), 0);
    player_connection_fd[i].port = 6666 + i;
    send(player_connection_fd[i].socket, (char *)&player_connection_fd[i].port, sizeof(player_connection_fd[i].port), 0);
    recv(player_connection_fd[i].socket, (char *)&player_connection_fd[i].hostname, sizeof(player_connection_fd[i].hostname), 0);
    cout << "Player " << i << " is ready to play" << endl;
    if (i != 0) {
      // cout << "sending hostname to player" << endl;
      // cout << player_connection_fd[i - 1].hostname << endl;
      send(player_connection_fd[i].socket, &player_connection_fd[i - 1].hostname, sizeof(player_connection_fd[i - 1].hostname), 0);
      // send port
      send(player_connection_fd[i].socket, (char *)&player_connection_fd[i - 1].port, sizeof(player_connection_fd[i - 1].port), 0);
      if (i == player_num - 1) {
	int last_player_ready_to_accept;
	recv(player_connection_fd[i].socket, &last_player_ready_to_accept, sizeof(last_player_ready_to_accept), 0);
	if (last_player_ready_to_accept == 1) {
	  send(player_connection_fd[0].socket, (char *)&player_connection_fd[i].hostname, sizeof(player_connection_fd[i].hostname), 0);
	  //send port
	  send(player_connection_fd[0].socket, (char *)&player_connection_fd[i].port, sizeof(player_connection_fd[i].port), 0);
	}
      }
    }
  }
  // record trace!!!!!
  if (hop_num == 0) {
    freeaddrinfo(host_info_list);
    for (int i = 0; i < player_num; i++) {
      close(player_connection_fd[i].socket);
    }
    return 0;
  }
  cout << "Ready to start the game, sending potato to player " << first_player << endl;
  struct potato potato;
  potato.hop_count = hop_num - 1;
  potato.trace[0] = first_player; 
  send(player_connection_fd[first_player].socket, &potato.hop_count, sizeof(potato.hop_count), 0);
  send(player_connection_fd[first_player].socket, potato.trace, sizeof(potato.trace), 0);
  fd_set master_fds;
  FD_ZERO(&master_fds);                                                        
  int max_fd = 0;                                                                     
  for (int i = 0; i < player_num; i ++) {                                             
    FD_SET(player_connection_fd[i].socket, &master_fds);                              
    if (player_connection_fd[i].socket > max_fd) {                                    
      max_fd = player_connection_fd[i].socket;                                        
    }                                                                                 
  }
  int res = select(max_fd + 1, &master_fds, NULL, NULL, NULL);                        
  if (res < 0) {                                                                      
    cerr << "Error: ringmaster select" << endl;                                       
  }
  for (int i = 0; i < player_num; i++) { //record the trace           
    if (FD_ISSET(player_connection_fd[i].socket, &master_fds)) {
      struct potato potato;
      recv(player_connection_fd[i].socket, &potato.hop_count, sizeof(potato.hop_count), MSG_WAITALL);
      recv(player_connection_fd[i].socket, potato.trace, sizeof(potato.trace), MSG_WAITALL);
      if (potato.hop_count == 0) {
	struct potato stop;
	stop.hop_count = -100;
	for (int i = 0; i < player_num; i++) {
	  send(player_connection_fd[i].socket, &stop.hop_count, sizeof(stop.hop_count), 0);
	}
       	cout << "Trace of potato:" << endl;
	if (hop_num == 1) {
	  cout << potato.trace[0] << endl;
	}
	else {
	  cout << potato.trace[0];
	  for (int i = 1; i <= hop_num - 1; i ++) {
	    if (i == hop_num - 1) {
	      cout << ",";
	      cout << potato.trace[i] << endl;
	    }
	    else {
	      cout << ",";
	      cout << potato.trace[i];
	    }
	  }
	}
      }      
      /*      struct potato stop;
      stop.hop_count = -100;
      for (int i = 0; i < player_num; i++) {
	send(player_connection_fd[i].socket, &stop.hop_count, sizeof(stop.hop_count), 0);
	}*/
      break;
    }
  }
 
  freeaddrinfo(host_info_list);
  for (int i = 0; i < player_num; i++) {
    close(player_connection_fd[i].socket);
  }
  return 0;
}
