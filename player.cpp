#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "potato.h"
#include <string>
#include <string.h>
#include <stdio.h>

using namespace std;

int main(int argc, char *argv[])
{
  if (argc != 3) {
    cerr << "Hostname and port number" << endl;
    return 0;
  }  
  //PART1: connecting with ringmaster
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = argv[1];
  const char *master_port = argv[2]; //host's port number
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(hostname, master_port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << master_port << ")" << endl;
    return -1;
  } //if
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol); // fake socket
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << master_port << ")" << endl;
    return -1;
  } //if
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << master_port << ")" << endl;
    return -1;
  } //if
  int num, id, hop, first_player;
  recv(socket_fd, &first_player, sizeof(first_player), 0);
  recv(socket_fd, &num, sizeof(num), 0);
  recv(socket_fd, &id, sizeof(id), 0);
  recv(socket_fd, &hop, sizeof(hop), 0);
  //  cout << "number of hops " << hop << endl;
  struct players player[num];
  recv(socket_fd, &player[id].port, sizeof(player[id].port), 0);
  cout << "Connected as player " << id << " out of " << num << " total players" << endl;
  gethostname(player[id].hostname, 256); //send hostname to ringmaster
  (player[id].hostname)[255] = '\0';
  send(socket_fd, &player[id].hostname, sizeof(player[id].hostname), 0);
  
  //PART2: Preparation for being a master!!!!!!!!!!!!!!!!!!!!(require player's hostname and port number of itself)
  struct addrinfo host_info2;
  struct addrinfo *host_info_list2;
  int status2;
  memset(&host_info2, 0, sizeof(host_info));  
  host_info2.ai_family = AF_UNSPEC;
  host_info2.ai_socktype = SOCK_STREAM;
  host_info2.ai_flags = AI_PASSIVE;
  string temp = to_string(player[id].port);
  status2 = getaddrinfo(player[id].hostname, temp.c_str(), &host_info2, &host_info_list2);
  if (status2 != 0) { //success returns zero                                 
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << player[id].hostname << "," << player[id].port << ")" << endl;
    return -1;
  }
  int socket_fd2;
  socket_fd2 = socket(host_info_list2->ai_family,
                     host_info_list2->ai_socktype,
                     host_info_list2->ai_protocol); // fake socket
  if (socket_fd2 == -1) {
    cerr <<"Error: cannot create socket" << endl;
    cerr << "  (" << player[id].hostname << "," << player[id].port << ")" << endl;
    return -1;
  }
  int yes = 1;
  status2 = setsockopt(socket_fd2, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status2 = bind(socket_fd2, host_info_list2->ai_addr, host_info_list2->ai_addrlen);
  if (status2 == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << player[id].hostname << "," << player[id].port << ")" << endl;
    return -1;
  }
  status2 = listen(socket_fd2, 10);
  if (status2 == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << player[id].hostname << "," << player[id].port << ")" << endl;
    return -1;
  }
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  
  // PART 3 : Preparation for connecting player i with player i - 1 (require player i - 1's hostname and port number)
  char hostname2[256];
  hostname2[255] = '\0';
  int master_port2; //master(client) 's port, from ringmaster
  int status3, socket_fd3;
  struct addrinfo host_info3;
  struct addrinfo *host_info_list3;
  memset(&host_info3, 0, sizeof(host_info3));
  host_info3.ai_family = AF_UNSPEC;
  host_info3.ai_socktype = SOCK_STREAM;  
 //Important Part!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  if (id == 0) { // first accept then connect
    player[id].socket = accept(socket_fd2, (struct sockaddr *)&socket_addr, &socket_addr_len); // create real socket for its neighbor                             
    if (player[id].socket == -1) {
      cerr << "Error: cannot accept connection on socket" << endl;
      return -1;
    }
    // cout << "Stop !!! waiting for the last player to accept connection" << endl;
    recv(socket_fd, &hostname2, sizeof(hostname2), 0);
    // receive master's port
    recv(socket_fd, &master_port2, sizeof(master_port2), 0);
    // cout << "player " << id << "'s master's hostname " << hostname2 << endl;
    temp = to_string(master_port2);
    status3 = getaddrinfo(hostname2, temp.c_str(), &host_info3, &host_info_list3);
    if (status3 != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << hostname2 << "," << master_port2 << ")" << endl;
      return -1;
    }
    socket_fd3 = socket(host_info_list3->ai_family,
                        host_info_list3->ai_socktype,
                        host_info_list3->ai_protocol);
    if (socket_fd3 == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << hostname2 << "," << master_port2 << ")" << endl;
      return -1;
    }
    status3 = connect(socket_fd3, host_info_list3->ai_addr, host_info_list3->ai_addrlen);
    if (status3 == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << hostname2 << "," << master_port2 << ")" << endl;
      return -1;
    }
    // cout << "I have connected to the last player" << endl;   
  }  
  else if (id != 0) { // first connect and then accept
    recv(socket_fd, &hostname2, sizeof(hostname2), 0);  // previous player's hostname
    //receive master's port
    recv(socket_fd, &master_port2, sizeof(master_port2), 0);
    // cout << "player " << id << "'s master's port number " << master_port2 << endl;
    temp = to_string(master_port2);
    status3 = getaddrinfo(hostname2, temp.c_str(), &host_info3, &host_info_list3);
    if (status3 != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << hostname2 << "," << master_port2 << ")" << endl;
      return -1;
    }
    socket_fd3 = socket(host_info_list3->ai_family,
                        host_info_list3->ai_socktype,
                        host_info_list3->ai_protocol);
    if (socket_fd3 == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << hostname2 << "," << master_port2 << ")" << endl;
      return -1;
    }
    status3 = connect(socket_fd3, host_info_list3->ai_addr, host_info_list3->ai_addrlen);
    if (status3 == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << hostname2 << "," << player[id].port << ")" << endl;
      return -1;
    }
    //cout << "I have connected with " << id - 1 << endl;
    if (id != num - 1) {
      player[id].socket = accept(socket_fd2, (struct sockaddr *)&socket_addr, &socket_addr_len); // create real socket for its neighbor                             
      if (player[id].socket == -1) {
	cerr << "Error: cannot accept connection on socket" << endl;
	return -1;
      }
    }
    if (id == num - 1) {
      int last_player_ready_to_accept = 1;
      send(socket_fd, (char *)&last_player_ready_to_accept, sizeof(last_player_ready_to_accept), 0);
      //cout << "I am waiting for palyer 0 to connect" << endl;
      player[id].socket = accept(socket_fd2, (struct sockaddr *)&socket_addr, &socket_addr_len);
      if (player[id].socket == -1) {
	cerr << "Error: cannot accept connection on socket" << endl;
	return -1;
      }
    }
  }
  if (hop == 0) {
    close(player[id].socket);
    freeaddrinfo(host_info_list);
    return 0;
  }  

  srand(time(NULL));
  fd_set player_fds;
  //  hop --;
  while (1) {
    // struct potato potato;
    int socket[3] = {socket_fd, player[id].socket, socket_fd3};
    FD_ZERO(&player_fds);
    int max_fd = 0;
    for (int i = 0; i < 3; i++) { // find max fd
      FD_SET(socket[i], &player_fds);
      if (socket[i] > max_fd) {
	max_fd = socket[i];
      }
    }
    int activity = select(max_fd + 1, &player_fds, NULL, NULL, NULL);
    if (activity < 0) {
      cerr << "Error: player select" << endl;
      return 0;
    }
    if (FD_ISSET(socket_fd, &player_fds)) {// from ringmaster
      //first player
      struct potato potato;
      recv(socket_fd, &potato.hop_count, sizeof(potato.hop_count), MSG_WAITALL);
      //STOP signal!!!!!!!
      if (potato.hop_count == -100) {                  
        close(player[id].socket);
        break;
      }
      recv(socket_fd, potato.trace, sizeof(potato.trace), MSG_WAITALL);
      if (potato.hop_count == 0) { // if hop_count == 1
	send(socket_fd, &potato.hop_count, sizeof(potato.hop_count), 0);
	send(socket_fd, &potato.trace, sizeof(potato.trace), 0);
	if (potato.hop_count == 0) {
	  cout << "I'm it" << endl;
	  sleep(1);
	  continue;
	}
      }
      potato.hop_count --;
      int next = rand() % 2;
      if (next == 0) { //send to the next player
	int next_player;
	if (id == num - 1) {
	  next_player = 0;
	}
	else if (id != num - 1){
	  next_player = id + 1;
	}
	cout << "Sending potato to " << next_player << endl;
	send(player[id].socket, &potato.hop_count, sizeof(potato.hop_count), 0);
	send(player[id].socket, potato.trace, sizeof(potato.trace), 0);
       	continue;
      }
      else if (next == 1) { //send to the previous player
	int prev_player;
	if (id == 0) {
	  prev_player = num - 1;
	}
	else if (id != 0){
	  prev_player = id - 1;
	}
	cout << "Sending potato to " << prev_player << endl;
	send(socket_fd3, &potato.hop_count, sizeof(potato.hop_count), 0);
	send(socket_fd3, potato.trace, sizeof(potato.trace), 0);
     	continue;
      }
    }
    if (FD_ISSET(socket_fd3, &player_fds)) { // receive potato from previous player
      struct potato potato;
      recv(socket_fd3, &potato.hop_count, sizeof(potato.hop_count), MSG_WAITALL);
      recv(socket_fd3, potato.trace, sizeof(potato.trace), MSG_WAITALL);
      //append trace
      potato.trace[hop - potato.hop_count - 1] = id;
      if (potato.hop_count == 0) {
	send(socket_fd, &potato.hop_count, sizeof(potato.hop_count), 0);
	send(socket_fd, potato.trace, sizeof(potato.trace), 0);
	if (potato.hop_count == 0) {
	  cout << "I'm it" << endl;
	  sleep(1);
	  continue;
	}
      }
      else { //hop != 0
	//	if (potato.trace[hop - potato.hop_count - 1] == id) {
	  potato.hop_count  = potato.hop_count - 1;
	  // cout << "Sending potato to player " << next << endl;
	  int next = rand() % 2;
	  if (next == 0) { //send to the next player
	    int next_player;
	    if (id == num - 1) {
	      next_player = 0;
	    }
	    else if (id != num - 1){
	      next_player = id + 1;
	    }
	    cout << "Sending potato to " << next_player << endl;
	    send(player[id].socket, &potato.hop_count, sizeof(potato.hop_count), 0);
	    send(player[id].socket, potato.trace, sizeof(potato.trace), 0);
	    continue;
	  }
	  else if (next == 1) { //send to the previous player
	    int prev_player;
	    if (id == 0) {
	      prev_player = num - 1;
	    }
	    else if(id != 0) {
	      prev_player = id - 1;
	    }
	    cout << "Sending potato to " << prev_player << endl;
	    send(socket_fd3, &potato.hop_count, sizeof(potato.hop_count), 0);
	    send(socket_fd3, potato.trace, sizeof(potato.trace), 0);
	    continue;
	  }
      }
    }
    if (FD_ISSET(player[id].socket, &player_fds)) { // receive potato from the next player
      struct potato potato;
      recv(player[id].socket, &potato.hop_count, sizeof(potato.hop_count), MSG_WAITALL);
      recv(player[id].socket, potato.trace, sizeof(potato.trace), MSG_WAITALL);
      // append trace
      potato.trace[hop - potato.hop_count - 1] = id;
      // cout << "hop = " << potato.hop_count << endl;
      // cout << "trace "<< hop - potato.hop_count << " = " << id << endl;
      if (potato.hop_count == 0) {
	send(socket_fd, &potato.hop_count, sizeof(potato.hop_count), 0);
	send(socket_fd, potato.trace, sizeof(potato.trace), 0);
	if (potato.hop_count == 0) {
	  cout << "I'm it" << endl;
	  sleep(1);
	  continue;
	}
      }
      else { //hop != 0
	//	if (potato.trace[hop - potato.hop_count - 1] == id) {
	  potato.hop_count  = potato.hop_count - 1;
	  // cout << "Sending potato to player " << next << endl;
	  int next = rand() % 2;
	  if (next == 0) { //send to the next
	    int next_player;
	    if (id == num - 1) {
	      next_player = 0;
	    }
	    else if(id != num - 1){
	      next_player = id + 1;
	    }
	    cout << "Sending potato to "	<< next_player << endl;
	    send(player[id].socket, &potato.hop_count, sizeof(potato.hop_count), 0);
	    send(player[id].socket, potato.trace, sizeof(potato.trace), 0);
	    continue;
	  }
	  if (next == 1) { //send to the previous
	    int prev_player;
	    if (id == 0) {
	      prev_player = num - 1;
	    }
	    else if(id != 0){
	      prev_player = id - 1;
	    }
	    cout << "Sending potato to " << prev_player << endl;
	    send(socket_fd3, &potato.hop_count, sizeof(potato.hop_count), 0);
	    send(socket_fd3, potato.trace, sizeof(potato.trace), 0);
	    continue;
	}
      }
    }
  }

  freeaddrinfo(host_info_list);
  return 0;  
}    
