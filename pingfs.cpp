#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <iostream>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h> 

const size_t MAX_MESSAGE_SIZE = 256;
const size_t IPV4_PING_HEADER_SIZE = 28;

struct data_packet
{
  struct icmphdr hdr;
  char msg[MAX_MESSAGE_SIZE];
};

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

int get_sock_fd() {
  int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd < 0){
    std::cout << "Failed to create Socket" << std::endl;
    std::cout << hstrerror(errno) << std::endl;
    throw std::exception();
  }
  int ttl = 64;
  setsockopt(sockfd, SOL_SOCKET, IP_TTL, &ttl, sizeof(ttl));
  struct timeval time_out = {0, 500};
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(time_out));
  return sockfd;
}

int main(int argc, char *argv[]) {

  auto host_entity = gethostbyname(argv[1]);
  struct sockaddr_in addr_con;
  addr_con.sin_family = host_entity->h_addrtype;
  addr_con.sin_port = htons (0);
  addr_con.sin_addr.s_addr = *(long*)host_entity->h_addr;
  std::string message = argv[2];
  size_t recv_size = message.size() + IPV4_PING_HEADER_SIZE;

  int sockfd;
  try {
    sockfd = get_sock_fd();
  } 
  catch (std::exception)
  {
    return EXIT_FAILURE;
  }

  struct data_packet packet;
  bzero(&packet, sizeof(packet));
  strcpy(packet.msg, message.c_str());
  while (1)
  {
    packet.hdr.type = ICMP_ECHO;
    packet.hdr.checksum = checksum(&packet, sizeof(packet));
    if (0 >= sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&addr_con, sizeof(addr_con)))
    {
      std::cout << "Failed to send packet" << std::endl;
      std::cout << hstrerror(errno) << std::endl;
      return EXIT_FAILURE;
    }

    bzero(&packet, sizeof(packet));
    usleep(1000000);

    struct sockaddr_in r_addr; 
    unsigned int addr_len = sizeof(r_addr);
    char buffer[recv_size];
    auto bRead = recvfrom(sockfd, &buffer, sizeof(buffer), 0, (struct sockaddr*)&r_addr, &addr_len);
    if (bRead == -1)
    {
      std::cout << "Failed to recvpacket errno " << errno << std::endl;
      std::cout << hstrerror(errno) << std::endl;
      return EXIT_FAILURE;
    }

    strcpy(packet.msg, std::string(buffer, bRead).substr(IPV4_PING_HEADER_SIZE).c_str());
    if (bRead == recv_size)
    {
      std::cout << "Your data is SECURE in the cloud: \"" << packet.msg << "\"" << std::endl;
    }
    else if (bRead < recv_size)
    {
      recv_size = bRead;
      std::cout << "Data was lost by your chosen cloud host.                  Bummer :-(" << std::endl;
      std::cout << "This is what we recovered:" << std::endl;
      std::cout << packet.msg << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
