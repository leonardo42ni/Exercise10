#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define MASTER_IP "127.0.0.1"

int sock;

// --- LUỒNG 1: Gửi Nhịp tim (Heartbeat Sender) ---
void* send_heartbeat(void* arg) {
    char heartbeat_msg[] = "HEARTBEAT";
    while(1) {
        sleep(2); // Yêu cầu của tài liệu: 2 giây gửi 1 lần
        send(sock, heartbeat_msg, strlen(heartbeat_msg), 0);
        printf("[WORKER] Đã bơm nhịp tim (HEARTBEAT) về Master\n");
    }
    return NULL;
}

// --- LUỒNG 2: Nhận Việc và Xử lý (Task Execution) ---
void* receive_tasks(void* arg) {
    char buffer[1024] = {0};
    while(1) {
        memset(buffer, 0, 1024);
        int valread = read(sock, buffer, 1024);
        
        if (valread > 0) {
            printf("[WORKER] Sếp giao việc: %s\n", buffer);
            
            // Giả lập thời gian cày cuốc xử lý Task mất 3 giây
            printf("[WORKER] Đang cày...\n");
            sleep(3); 
            
            // Làm xong thì báo cáo kết quả
            char result_msg[] = "RESULT: DONE_TASK";
            send(sock, result_msg, strlen(result_msg), 0);
            printf("[WORKER] Đã gửi kết quả trả Master!\n\n");
        } else if (valread == 0) {
            printf("[CẢNH BÁO] Master đã ngắt kết nối. Worker xin nghỉ!\n");
            exit(0);
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;

    // 1. Tạo Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Lỗi: Không thể tạo socket\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, MASTER_IP, &serv_addr.sin_addr) <= 0) {
        printf("Lỗi địa chỉ IP không hợp lệ\n");
        return -1;
    }

    // 2. Kết nối tới Master
    printf("Đang tìm Master tại %s:%d...\n", MASTER_IP, PORT);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Kết nối thất bại! Master chưa bật à?\n");
        return -1;
    }

    // 3. Giao thức Đăng ký (Register)
    char register_msg[] = "REGISTER";
    send(sock, register_msg, strlen(register_msg), 0);
    printf("Đã báo danh thành công với Master!\n\n");

    // 4. Kích hoạt Đa luồng (Vừa làm việc vừa báo cáo)
    pthread_t thread_heartbeat, thread_task;
    pthread_create(&thread_heartbeat, NULL, send_heartbeat, NULL);
    pthread_create(&thread_task, NULL, receive_tasks, NULL);

    // 5. Giữ cho Worker chạy liên tục
    pthread_join(thread_heartbeat, NULL);
    pthread_join(thread_task, NULL);

    return 0;
}
