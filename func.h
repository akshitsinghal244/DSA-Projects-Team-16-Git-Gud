#ifndef FUNC_H
#define FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_SERVICE_NAME 256
#define MAX_LOG_ENTRY 512
#define MAX_FAILED_QUEUE 100

// Service status enumeration
typedef enum {
    STATUS_ACTIVE,
    STATUS_INACTIVE,
    STATUS_FAILED,
    STATUS_SUSPENDED,
    STATUS_RUNNING,
    STATUS_STOPPED
} ServiceStatus;

// Service structure
typedef struct Service {
    char name[MAX_SERVICE_NAME];
    ServiceStatus status;
    int pid;
    char last_started[64];
    struct Service* next;
} Service;

// Log entry structure
typedef struct LogEntry {
    char service_name[MAX_SERVICE_NAME];
    char action[64];
    char timestamp[64];
    struct LogEntry* next;
} LogEntry;

// Failed service queue structure
typedef struct FailedService {
    char name[MAX_SERVICE_NAME];
    int failure_count;
    time_t last_failure;
    struct FailedService* next;
} FailedService;

// BST node for fast searching
typedef struct BSTNode {
    Service* service;
    struct BSTNode* left;
    struct BSTNode* right;
} BSTNode;

// Global variables
extern Service* service_list;
extern LogEntry* log_stack;
extern FailedService* failed_queue_front;
extern FailedService* failed_queue_rear;
extern BSTNode* service_bst;
extern int failed_queue_size;

// Function prototypes
void load_services_from_system();
void add_service_to_list(const char* name, ServiceStatus status, int pid);
void add_log_entry(const char* service_name, const char* action);
void display_all_services();
void display_logs();
void search_service_by_name(const char* name);
void filter_services_by_status(ServiceStatus status);
void start_service(const char* service_name);
void stop_service(const char* service_name);
void restart_service(const char* service_name);
void detect_failed_services();
void add_to_failed_queue(const char* service_name);
void process_failed_services();
void monitor_services();
BSTNode* insert_bst(BSTNode* node, Service* service);
Service* search_bst(BSTNode* node, const char* name);
const char* status_to_string(ServiceStatus status);
ServiceStatus string_to_status(const char* status_str);
void free_memory();

#endif