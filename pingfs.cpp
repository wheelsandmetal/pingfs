#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <iostream>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h> 

const int MAX_MESSAGE_SIZE = 64;

uint16_t checksum(void* vdata, size_t length) {
    char* data=(char*)vdata;
    uint32_t acc=0xffff;

    // Handle complete 16-bit blocks.
    for (size_t i = 0; i+1 < length; i += 2) {
        uint16_t word;
        memcpy(&word,data+i,2);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Handle any partial block at the end of the data.
    if (length&1) {
        uint16_t word=0;
        memcpy(&word,data+length-1,1);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Return the checksum in network byte order.
    return htons(~acc);
}

struct data_packet
{
  struct icmphdr hdr;
  char msg[MAX_MESSAGE_SIZE];
};

int main(int argc, char *argv[]) {

  auto host_entity = gethostbyname(argv[1]);
  struct sockaddr_in addr_con;
  addr_con.sin_family = host_entity->h_addrtype;
  addr_con.sin_port = htons (0);
  addr_con.sin_addr.s_addr = *(long*)host_entity->h_addr;

  int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd < 0){
    std::cout << "Failed to create Socket" << std::endl;
    switch (errno) {
      case 1: std::cout << "Permisssion Denined" << std::endl; break;
      default: std::cout << "Check '$man socket' for errno " << errno << std::endl;
    }
    return EXIT_FAILURE;
  }
  int ttl = 64;
  setsockopt(sockfd, SOL_SOCKET, IP_TTL, &ttl, sizeof(ttl));
  struct timeval time_out = {0, 500};
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(time_out));

  struct data_packet packet;
  while (1) {
    bzero(&packet, sizeof(packet));
    strcpy(packet.msg, "oh what a life");
    packet.hdr.type = ICMP_ECHO;
    packet.hdr.checksum = checksum(&packet, sizeof(packet));
    if ( 0 >= sendto(sockfd, &packet, sizeof(packet), 0,
          (struct sockaddr*)&addr_con, sizeof(addr_con))
       )
    {
      std::cout << "Failed to send packet" << std::endl;
      switch (errno) {
        case 1: std::cout << "Permisssion Denined" << std::endl; break;
        default: std::cout << "Check '$man sentto' for errno " << errno << std::endl;
      }
      return EXIT_FAILURE;
    }
    usleep(100000);

    struct sockaddr_in r_addr; 
    unsigned int addr_len = sizeof(r_addr);
    if ( 0 >= recvfrom(sockfd, &packet, sizeof(packet), 0,
          (struct sockaddr*)&r_addr, &addr_len)
       )
    {
      std::cout << "Failed to recvpacket errno " << errno << std::endl;
      std::cout << hstrerror(errno) << std::endl;
    }

    std::cout.write(packet.msg, sizeof(packet.msg)) << std::endl;
  }

  return EXIT_SUCCESS;
}
