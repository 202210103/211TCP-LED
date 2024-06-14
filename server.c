#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>      


#define PORT 8080

int blink_interval = 0;
int stop_blinking = 0;
pthread_t blink_thread; //闪烁线程

void set_led(int state) {
    // 如果state为真（非零），则执行以下操作以打开LED  
    if (state) {
        int fd, ret; // 声明文件描述符fd和返回值ret  

        // 打开/dev/leds设备文件，以读写方式，不使用控制终端，且不使用阻塞模式  
        fd = open("/dev/leds", O_RDWR | O_NOCTTY | O_NDELAY);

        // 检查文件描述符是否有效（即文件是否成功打开）  
        if (fd > 0) {
            // 使用ioctl系统调用发送命令给/dev/leds设备文件 
            // 点亮led2  
            ret = ioctl(fd, 1, 0);
        }
        // 关闭文件描述符  
        close(fd);
        printf("LED IS ON\n");

        // 如果state为假（零），则执行以下操作以关闭LED  
    }
    else {
        int fd, ret; 
        // 熄灭LED  
        fd = open("/dev/leds", O_RDWR | O_NOCTTY | O_NDELAY);
        if (fd > 0) {
            // 使用ioctl系统调用发送另一个命令给/dev/leds设备文件，熄灭LED2  
            ret = ioctl(fd, 0, 0);
        }
        // 关闭文件描述符  
        close(fd);
        printf("LED IS OFF\n");
    }
}

// 初始闪烁LED指定次数  
void initial_blink(int interval, int times) {
    int i;
    // 循环times次  
    for (i = 0; i < times; i++) {
        // 打开LED  
        set_led(0);
        // 等待interval秒  
        sleep(interval);
        // 关闭LED  
        set_led(1);
        // 再次等待interval秒  
        sleep(interval);
    }
}

// 线程函数，用于持续闪烁LED  
void* blink_led(void* arg) {
    // 循环直到stop_blinking变为真  
    while (!stop_blinking) {
        // 打开LED  
        set_led(1);
        // 等待blink_interval秒  
        sleep(blink_interval);
        // 关闭LED  
        set_led(0);
        // 再次等待blink_interval秒  
        sleep(blink_interval);
    }
    // 线程结束，返回NULL  
    return NULL;
}

//处理客户端请求
void handle_client(int client_socket) {
    // 定义一个缓冲区用于存储从客户端读取的数据  
    char buffer[1024];
    int valread; // 变量用于存储从socket读取到的字节数  
    // 无限循环，用于持续监听客户端的数据  
    while (1) {
        // 从客户端socket读取数据，最多读取1024个字节  
        valread = read(client_socket, buffer, 1024);
        // 检查是否成功读取到数据  
        if (valread > 0) {
            // 在读取到的数据后添加一个字符串终止符'\0'  
            buffer[valread] = '\0';

            // 检查客户端发送的命令  
            if (strcmp(buffer, "on") == 0) {
                stop_blinking = 1;  //停止闪烁                            
                set_led(1);
            }
            else if (strcmp(buffer, "off") == 0) {
                stop_blinking = 1;                                  
                set_led(0);     
            }
            else if (strncmp(buffer, "blink", 5) == 0) {
                // 如果命令以"blink"开头，则尝试解析闪烁间隔  
                int interval = atoi(buffer + 6); // 从"blink"后面的字符开始解析整数  

                // 检查解析得到的间隔是否大于0  
                if (interval > 0) {
                    // 设置停止闪烁标志为1，等待blink_thread线程结束（如果它正在运行）  
                    // 更新闪烁间隔  
                    blink_interval = interval;
                    // 清除停止闪烁标志，允许blink_led线程开始运行  
                    stop_blinking = 0;
                    // 创建一个新的blink_led线程  
                    pthread_create(&blink_thread, NULL, blink_led, NULL);
                }
                else {
                    // 如果命令无效，打印错误信息  
                    printf("Invalid interval received: %s\n", buffer);
                }
            }
            else {
                // 如果命令无效，打印错误信息  
                printf("Invalid command received: %s\n", buffer);
            }
        }
        else {
            // 如果没有读取到数据（如客户端关闭了连接），则退出循环  
            break;
        }
    }
    // 关闭与客户端的连接  
    close(client_socket);
}

int main() {
    //套接字：应用程序和网络间接口
    int server_fd, client_socket; // 定义服务器套接字（server_fd）和客户端套接字（client_socket）  
    struct sockaddr_in address; // 定义IPv4的套接字地址结构体  
    int opt = 1; // 设置socket选项的值，这里是为了允许地址重用  
    int addrlen = sizeof(address); // 地址结构体的长度  

    // 开机时 LED 闪烁  
    initial_blink(1, 3); // 调用函数，设置LED每秒闪烁一次，闪烁3次  
    printf("ok");

    // 创建IPv4的TCP套接字  
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed"); // 如果创建套接字失败，输出错误信息  
        exit(EXIT_FAILURE); // 退出程序，返回失败状态  
    }

    // 设置socket选项，允许端口复用  
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed"); // 如果设置socket选项失败，输出错误信息  
        close(server_fd); // 关闭套接字  
        exit(EXIT_FAILURE); // 退出程序，返回失败状态  
    }

    // 初始化地址结构体  
    address.sin_family = AF_INET; // 设置为IPv4  
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有可用的网络接口  
    address.sin_port = htons(PORT); // 设置服务器监听的端口号

    // 绑定套接字到指定的地址和端口  
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed"); // 如果绑定失败，输出错误信息  
        close(server_fd); // 关闭套接字  
        exit(EXIT_FAILURE); // 退出程序，返回失败状态  
    }

    // 开始监听连接请求  
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed"); // 如果监听失败，输出错误信息  
        close(server_fd); // 关闭套接字  
        exit(EXIT_FAILURE); // 退出程序，返回失败状态  
    }

    // 循环接受客户端的连接请求  
    while (1) {
        // 等待客户端连接请求，并返回一个新的套接字用于与该客户端通信  
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed"); // 如果接受连接失败，输出错误信息  
            close(server_fd); // 关闭服务器套接字
            exit(EXIT_FAILURE); // 退出程序，返回失败状态  
        }

        // 处理客户端的请求  
        handle_client(client_socket); // 调用函数处理客户端的请求  
    }

    return 0;  
}