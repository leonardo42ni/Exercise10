#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_WORKERS 10
#define PORT 8080

// --- CẤU TRÚC DỮ LIỆU ---
typedef struct {
    int id;
    int socket_fd;
    int is_alive;
    time_t last_heartbeat;
    int current_load; // Số task đang chạy
} Worker;

Worker workers[MAX_WORKERS];
int worker_count = 0;

// Khóa bảo vệ dữ liệu dùng chung (Cực kỳ quan trọng)
pthread_mutex_t worker_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- LUỒNG 1: Lắng nghe kết nối từ Worker ---
void* accept_connections(void* arg) {
    int server_fd = *(int*)arg;
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);

    while(1) {
        int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len);
        if (client_sock >= 0) {
            // CÓ THAY ĐỔI DỮ LIỆU DÙNG CHUNG -> PHẢI KHÓA LẠI
            pthread_mutex_lock(&worker_mutex);
            
            workers[worker_count].id = worker_count + 1;
            workers[worker_count].socket_fd = client_sock;
            workers[worker_count].is_alive = 1;
            workers[worker_count].last_heartbeat = time(NULL);
            workers[worker_count].current_load = 0;
            
            printf("[MASTER] Worker %d đã kết nối thành công!\n", workers[worker_count].id);
            worker_count++;
            
            // LÀM XONG PHẢI MỞ KHÓA
            pthread_mutex_unlock(&worker_mutex);
        }
    }
    return NULL;
}

// --- LUỒNG 2: Giám sát Nhịp tim (Heartbeat Monitor) ---
void* monitor_heartbeat(void* arg) {
    while(1) {
        sleep(2); // Cứ 2 giây quét một lần
        time_t current_time = time(NULL);
        
        pthread_mutex_lock(&worker_mutex);
        for (int i = 0; i < worker_count; i++) {
            if (workers[i].is_alive) {
                // Nếu quá 6 giây không thấy nhịp tim -> Phán quyết FAILED
                if (difftime(current_time, workers[i].last_heartbeat) > 6.0) {
                    workers[i].is_alive = 0;
                    printf("[CẢNH BÁO] Worker %d ĐÃ CHẾT! Mất kết nối.\n", workers[i].id);
                    // TODO: Gọi hàm thu hồi Task của Worker này ném lại vào Hàng đợi
                }
            }
        }
        pthread_mutex_unlock(&worker_mutex);
    }
    return NULL;
}

// --- LUỒNG 3: Thuật toán Điều phối (Scheduler) ---
void* schedule_tasks(void* arg) {
    while(1) {
        // TODO: Lấy Task từ Hàng đợi
        // TODO: Tìm Worker rảnh rỗi (Dùng FIFO, Round Robin, hoặc Least Loaded)
        // TODO: Gửi tín hiệu phân công qua socket
        sleep(1); 
    }
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;

    // 1. Khởi tạo Socket Server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, MAX_WORKERS);

    printf("=== MASTER NODE KHỞI ĐỘNG ===\n");

    // 2. Kích hoạt 3 luồng chạy song song
    pthread_t thread_accept, thread_monitor, thread_scheduler;
    
    pthread_create(&thread_accept, NULL, accept_connections, &server_fd);
    pthread_create(&thread_monitor, NULL, monitor_heartbeat, NULL);
    pthread_create(&thread_scheduler, NULL, schedule_tasks, NULL);

    // 3. Main thread chờ các luồng con (Thực tế là chạy vô hạn)
    pthread_join(thread_accept, NULL);
    pthread_join(thread_monitor, NULL);
    pthread_join(thread_scheduler, NULL);

    return 0;
}
