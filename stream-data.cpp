#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 5005         // The port server listens on
#define BUFFER_SIZE 65535 // Max size of a UDP packet mtlab 65535 bytes k packets can be sent

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddress, clientAddress;

    // Creating a UDP Socket
    // Which is alos called Socket Descriptor
    // It is an integer that uniquely identifies the socket in the operating system (kernal) very similar to file descriptors

    // AF_INET: This specifies that we are using the IPv4 address family the usual 198.168.x.x
    // SOCK_DGRAM: This indicates that we want to create a datagram socket which is used for UDP communication
    // For TCP its SOCK_STREAM
    // 0 : This is the protocol field. For UDP we can set it to 0 to let the system choose the appropriate protocol (which will be IPPROTO_UDP)
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Listen on ALL available network interfaces
    serverAddress.sin_port = htons(PORT);       // htons converts the port number from host byte order to network byte order (big-endian) which is required for network communication
                                                // big-endian mtalab MSB phele store hogi then LSB, left to right padhne k liye

    // Bind the socket to the port, saara traffic jo is port pe aayega wo is socket pe receive hoga
    if (bind(sockfd, (const struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Define our "Subscribers" (for testing, send to ports 6001 and 6002)
    // Jaha pe video data receive karna hai, matlab subscriber ke address aur port number define karna hai
    struct sockaddr_in sub1, sub2;
    sub1.sin_family = sub2.sin_family = AF_INET;
    sub1.sin_port = htons(6001);
    sub2.sin_port = htons(6002);

    // IP Network to Presentation: This function converts an IP address from its standard text presentation form
    // taki network card smajh sake, aur usko binary format me store kar sake, jo ki network communication ke liye zaruri hai
    inet_pton(AF_INET, "192.168.1.143", &sub1.sin_addr);
    inet_pton(AF_INET, "127.0.0.1", &sub2.sin_addr);
    // inet_pton(AF_INET, "192.168.15.255", &sub1.sin_addr); // Agar broadcast karna hai toh using router etc.
    // localhost pe hi dono subscribers hain, agar alag machines pe hote to unka IP address yaha specify karna padta

    std::cout << "UDP Reflector listening on port " << PORT << "..." << std::endl;

    while (true) {   
        int count = 0;
        socklen_t len = sizeof(clientAddress);
        // Receive packet
        int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddress, &len);

        // if (n > 0) {
        //     std::cout << "Received " << n << " bytes from " << inet_ntoa(clientAddress.sin_addr) << std::endl;
        // }

        if (n > 0) {
            count++;
            if (count % 100 == 0) { // Print every 100 packets so the console isn't too messy
                std::cout << "Reflected 100 packets to " << inet_ntoa(sub1.sin_addr) << std::endl;
            }
        //     sendto(sockfd, buffer, n, 0, (struct sockaddr *)&sub1, sizeof(sub1));
        }

        // Forward the packet to subscribers clone karke
        sendto(sockfd, buffer, n, 0, (struct sockaddr *)&sub1, sizeof(sub1));
        sendto(sockfd, buffer, n, 0, (struct sockaddr *)&sub2, sizeof(sub2));
    }

    close(sockfd);
    return 0;
}
