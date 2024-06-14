#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <arpa/inet.h>  

#define PORT 8080  

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = { 0 };

    // 创建socket  IPV4 TCP套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // 设置服务器的网络地址和端口  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 将IP地址转换为网络字节序  
    if (inet_pton(AF_INET, "192.168.137.15", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // 连接到服务器  
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // 循环接收用户输入并发送到服务器  
    while (1) {
        printf("Enter command (on/off/blink <interval>): ");
        fgets(buffer, 1024, stdin);     //从命令行读取文本
        buffer[strcspn(buffer, "\n")] = 0;  // 去除换行符  

        // 发送数据到服务器  
        send(sock, buffer, strlen(buffer), 0);
        printf("Command sent: %s\n", buffer);
    }
    close(sock);
    return 0;
}