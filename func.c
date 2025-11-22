#include "func.h"

// Global variables definition
Service* service_list = NULL;
LogEntry* log_stack = NULL;
FailedService* failed_queue_front = NULL;
FailedService* failed_queue_rear = NULL;
BSTNode* service_bst = NULL;
int failed_queue_size = 0;

// Load services from system using systemctl
void load_services_from_system() {
    FILE *fp;
    char line[1024];
    char service_name[MAX_SERVICE_NAME];
    char load_state[64], active_state[64], sub_state[64];
    
    printf("Loading services from system...\n");
    
    // Get all services with more detailed status information
    fp = popen("systemctl list-units --type=service --all --no-pager --no-legend", "r");
    if (fp == NULL) {
        perror("Failed to load services");
        return;
    }
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Parse systemctl output format: UNIT LOAD ACTIVE SUB DESCRIPTION
        int parsed = sscanf(line, "%255s %63s %63s %63s", 
                           service_name, load_state, active_state, sub_state);
        
        if (parsed >= 4) {
            // Remove .service suffix if present
            char* dot = strstr(service_name, ".service");
            if (dot) *dot = '\0';
            
            // Use the active state and sub-state for more accurate status
            ServiceStatus status;
            
            if (strcmp(active_state, "active") == 0) {
                if (strcmp(sub_state, "running") == 0) {
                    status = STATUS_RUNNING;
                } else {
                    status = STATUS_ACTIVE;
                }
            } else if (strcmp(active_state, "inactive") == 0) {
                status = STATUS_INACTIVE;
            } else if (strcmp(active_state, "failed") == 0) {
                status = STATUS_FAILED;
            } else {
                status = string_to_status(sub_state); // Fallback to string parsing
            }
            
            add_service_to_list(service_name, status, 0);
        }
    }
    pclose(fp);
    
    printf("Services loaded successfully!\n");
}

// *** NEW FUNCTION: List all processes using ps aux ***
void list_all_processes() {
    FILE *fp;
    char line[1024];
    
    printf("\n=== All System Processes (ps aux) ===\n");
    
    // Use 'ps aux' for a detailed list of all running processes
    // --no-headers removes the redundant header if the user wants to pipe this
    fp = popen("ps aux --no-headers", "r"); 
    if (fp == NULL) {
        perror("Failed to execute ps aux");
        return;
    }
    
    // Print header for ps aux output
    printf("%-8s %-6s %-4s %-4s %-8s %-8s %-15s %-20s\n", 
           "USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "STAT", "COMMAND");
    printf("------------------------------------------------------------------------------------------------------------------\n");
    
    // Read and print the output line by line
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }
    
    pclose(fp);
    printf("------------------------------------------------------------------------------------------------------------------\n");
    printf("Process listing complete.\n");
}
// ******************************************************

// Add service to linked list and BST
void add_service_to_list(const char* name, ServiceStatus status, int pid) {
    Service* new_service = (Service*)malloc(sizeof(Service));
    if (new_service == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    
    strncpy(new_service->name, name, MAX_SERVICE_NAME - 1);
    new_service->name[MAX_SERVICE_NAME - 1] = '\0';
    new_service->status = status;
    new_service->pid = pid;
    
    // Set current time as last started
    time_t now = time(NULL);
    strftime(new_service->last_started, sizeof(new_service->last_started), 
             "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    // Add to linked list
    new_service->next = service_list;
    service_list = new_service;
    
    // Add to BST for fast searching
    service_bst = insert_bst(service_bst, new_service);
}

// BST insertion
BSTNode* insert_bst(BSTNode* node, Service* service) {
    if (node == NULL) {
        BSTNode* new_node = (BSTNode*)malloc(sizeof(BSTNode));
        if (new_node == NULL) {
            printf("Memory allocation failed!\n");
            return NULL;
        }
        new_node->service = service;
        new_node->left = new_node->right = NULL;
        return new_node;
    }
    
    int cmp = strcmp(service->name, node->service->name);
    if (cmp < 0) {
        node->left = insert_bst(node->left, service);
    } else if (cmp > 0) {
        node->right = insert_bst(node->right, service);
    }
    
    return node;
}

// BST search
Service* search_bst(BSTNode* node, const char* name) {
    if (node == NULL) return NULL;
    
    int cmp = strcmp(name, node->service->name);
    if (cmp == 0) {
        return node->service;
    } else if (cmp < 0) {
        return search_bst(node->left, name);
    } else {
        return search_bst(node->right, name);
    }
}

// Add log entry (stack implementation - LIFO)
void add_log_entry(const char* service_name, const char* action) {
    LogEntry* new_log = (LogEntry*)malloc(sizeof(LogEntry));
    if (new_log == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    
    strncpy(new_log->service_name, service_name, MAX_SERVICE_NAME - 1);
    new_log->service_name[MAX_SERVICE_NAME - 1] = '\0';
    strncpy(new_log->action, action, 63);
    new_log->action[63] = '\0';
    
    // Add timestamp
    time_t now = time(NULL);
    strftime(new_log->timestamp, sizeof(new_log->timestamp), 
             "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    // Push to stack
    new_log->next = log_stack;
    log_stack = new_log;
    
    printf("LOG: %s - %s - %s\n", new_log->timestamp, service_name, action);
}

// Display all services
void display_all_services() {
    printf("\n=== All Services (from internal list) ===\n");
    printf("%-40s %-12s %-8s %-20s\n", "SERVICE NAME", "STATUS", "PID", "LAST STARTED");
    printf("--------------------------------------------------------------------------------\n");
    
    Service* current = service_list;
    int count = 0;
    
    while (current != NULL) {
        printf("%-40s %-12s %-8d %-20s\n", 
               current->name, 
               status_to_string(current->status),
               current->pid,
               current->last_started);
        current = current->next;
        count++;
    }
    
    printf("\nTotal services: %d\n", count);
}

// Display logs (stack - most recent first)
void display_logs() {
    printf("\n=== Service Logs (Most Recent First) ===\n");
    printf("%-20s %-30s %-40s\n", "TIMESTAMP", "ACTION", "SERVICE");
    printf("--------------------------------------------------------------------------------\n");
    
    LogEntry* current = log_stack;
    int count = 0;
    
    while (current != NULL) {
        printf("%-20s %-30s %-40s\n", 
               current->timestamp, 
               current->action,
               current->service_name);
        current = current->next;
        count++;
    }
    
    if (count == 0) {
        printf("No logs available.\n");
    }
}

// Search service by name using BST
void search_service_by_name(const char* name) {
    Service* found = search_bst(service_bst, name);
    
    if (found) {
        printf("\nService Found:\n");
        printf("Name: %s\n", found->name);
        printf("Status: %s\n", status_to_string(found->status));
        printf("PID: %d\n", found->pid);
        printf("Last Started: %s\n", found->last_started);
    } else {
        printf("Service '%s' not found.\n", name);
    }
}

// Filter services by status
void filter_services_by_status(ServiceStatus status) {
    printf("\n=== Services with Status: %s ===\n", status_to_string(status));
    printf("%-40s %-8s %-20s\n", "SERVICE NAME", "PID", "LAST STARTED");
    printf("--------------------------------------------------------------------------------\n");
    
    Service* current = service_list;
    int count = 0;
    
    while (current != NULL) {
        if (current->status == status) {
            printf("%-40s %-8d %-20s\n", 
                   current->name, 
                   current->pid,
                   current->last_started);
            count++;
        }
        current = current->next;
    }
    
    printf("\nFound %d services with status '%s'\n", count, status_to_string(status));
}

// Start a service
void start_service(const char* service_name) {
    Service* service = search_bst(service_bst, service_name);
    
    if (service) {
        char command[512];
        snprintf(command, sizeof(command), "systemctl start %s.service", service_name);
        
        int result = system(command);
        if (result == 0) {
            service->status = STATUS_ACTIVE;
            service->pid = getpid(); // Simplified - in real implementation, get actual PID
            time_t now = time(NULL);
            strftime(service->last_started, sizeof(service->last_started), 
                     "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            add_log_entry(service_name, "STARTED");
            printf("Service '%s' started successfully.\n", service_name);
        } else {
            service->status = STATUS_FAILED;
            add_log_entry(service_name, "START FAILED");
            add_to_failed_queue(service_name);
            printf("Failed to start service '%s'.\n", service_name);
        }
    } else {
        printf("Service '%s' not found.\n", service_name);
    }
}

// Stop a service
void stop_service(const char* service_name) {
    Service* service = search_bst(service_bst, service_name);
    
    if (service) {
        char command[512];
        snprintf(command, sizeof(command), "systemctl stop %s.service", service_name);
        
        int result = system(command);
        if (result == 0) {
            service->status = STATUS_INACTIVE;
            service->pid = 0;
            add_log_entry(service_name, "STOPPED");
            printf("Service '%s' stopped successfully.\n", service_name);
        } else {
            add_log_entry(service_name, "STOP FAILED");
            printf("Failed to stop service '%s'.\n", service_name);
        }
    } else {
        printf("Service '%s' not found.\n", service_name);
    }
}

// Restart a service
void restart_service(const char* service_name) {
    Service* service = search_bst(service_bst, service_name);
    
    if (service) {
        char command[512];
        snprintf(command, sizeof(command), "systemctl restart %s.service", service_name);
        
        int result = system(command);
        if (result == 0) {
            service->status = STATUS_ACTIVE;
            service->pid = getpid(); // Simplified
            time_t now = time(NULL);
            strftime(service->last_started, sizeof(service->last_started), 
                     "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            add_log_entry(service_name, "RESTARTED");
            printf("Service '%s' restarted successfully.\n", service_name);
        } else {
            service->status = STATUS_FAILED;
            add_log_entry(service_name, "RESTART FAILED");
            add_to_failed_queue(service_name);
            printf("Failed to restart service '%s'.\n", service_name);
        }
    } else {
        printf("Service '%s' not found.\n", service_name);
    }
}

// Detect failed services
void detect_failed_services() {
    printf("Detecting failed/unresponsive services...\n");
    
    FILE *fp = popen("systemctl list-units --type=service --state=failed --no-pager --no-legend", "r");
    if (fp == NULL) {
        perror("Failed to detect failed services");
        return;
    }
    
    char line[1024];
    char service_name[MAX_SERVICE_NAME];
    int failed_count = 0;
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "%255s", service_name) == 1) {
            // Remove .service suffix
            char* dot = strstr(service_name, ".service");
            if (dot) *dot = '\0';
            
            // Update service status
            Service* service = search_bst(service_bst, service_name);
            if (service) {
                service->status = STATUS_FAILED;
                add_to_failed_queue(service_name);
                failed_count++;
            }
        }
    }
    pclose(fp);
    
    printf("Detection complete. Found %d failed services.\n", failed_count);
}

// Add to failed services queue
void add_to_failed_queue(const char* service_name) {
    if (failed_queue_size >= MAX_FAILED_QUEUE) {
        printf("Failed services queue is full!\n");
        return;
    }
    
    FailedService* new_failed = (FailedService*)malloc(sizeof(FailedService));
    if (new_failed == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    
    strncpy(new_failed->name, service_name, MAX_SERVICE_NAME - 1);
    new_failed->name[MAX_SERVICE_NAME - 1] = '\0';
    new_failed->failure_count = 1;
    new_failed->last_failure = time(NULL);
    new_failed->next = NULL;
    
    if (failed_queue_rear == NULL) {
        failed_queue_front = failed_queue_rear = new_failed;
    } else {
        failed_queue_rear->next = new_failed;
        failed_queue_rear = new_failed;
    }
    
    failed_queue_size++;
    add_log_entry(service_name, "ADDED TO FAILED QUEUE");
}

// Process failed services queue
void process_failed_services() {
    printf("Processing failed services queue...\n");
    
    if (failed_queue_front == NULL) {
        printf("No failed services in queue.\n");
        return;
    }
    
    FailedService* current = failed_queue_front;
    int processed = 0;
    
    while (current != NULL) {
        printf("Attempting to restart failed service: %s (Failure count: %d)\n", 
               current->name, current->failure_count);
        
        // Attempt to restart
        char command[512];
        snprintf(command, sizeof(command), "systemctl restart %s.service", current->name);
        
        int result = system(command);
        if (result == 0) {
            printf("Successfully restarted: %s\n", current->name);
            add_log_entry(current->name, "AUTO-RESTARTED FROM FAILED QUEUE");
            
            // Update service status
            Service* service = search_bst(service_bst, current->name);
            if (service) {
                service->status = STATUS_ACTIVE;
            }
        } else {
            printf("Failed to restart: %s\n", current->name);
            current->failure_count++;
            add_log_entry(current->name, "AUTO-RESTART FAILED");
        }
        
        processed++;
        current = current->next;
    }
    
    printf("Processed %d failed services.\n", processed);
}

// Monitor services with auto-refresh
void monitor_services() {
    printf("Starting service monitor (Auto-refresh every 5 seconds, press Ctrl+C to stop)...\n");
    
    for (int i = 0; i < 3; i++) { // Monitor for 3 cycles
        printf("\n=== Monitor Cycle %d ===\n", i + 1);
        printf("Time: %s\n", _TIME_);
        
        // Reload services to get current status
        load_services_from_system();
        display_all_services();
        
        // Detect failed services
        detect_failed_services();
        
        if (i < 2) { // Don't sleep after last cycle
            sleep(5); // Wait 5 seconds
        }
    }
    
    printf("Monitoring completed.\n");
}

// Redundant code
const char* status_to_string(ServiceStatus status) {
    switch (status) {
        case STATUS_ACTIVE: return "ACTIVE";
        case STATUS_INACTIVE: return "INACTIVE";
        case STATUS_FAILED: return "FAILED";
        case STATUS_SUSPENDED: return "SUSPENDED";
        case STATUS_RUNNING: return "RUNNING";
        case STATUS_STOPPED: return "STOPPED";
        default: return "UNKNOWN";
    }
}

ServiceStatus string_to_status(const char* status_str) {
    if (strstr(status_str, "active") != NULL && strstr(status_str, "running") != NULL) 
        return STATUS_RUNNING;
    if (strstr(status_str, "active") != NULL) 
        return STATUS_ACTIVE;
    if (strstr(status_str, "inactive") != NULL || strstr(status_str, "dead") != NULL) 
        return STATUS_INACTIVE;
    if (strstr(status_str, "failed") != NULL) 
        return STATUS_FAILED;
    if (strstr(status_str, "exited") != NULL) 
        return STATUS_STOPPED;
    if (strstr(status_str, "suspended") != NULL) 
        return STATUS_SUSPENDED;
    
    // For systemd daemons, also check the state more carefully
    if (strcmp(status_str, "running") == 0) 
        return STATUS_RUNNING;
    if (strcmp(status_str, "stopped") == 0) 
        return STATUS_STOPPED;
    
    return STATUS_INACTIVE; // Default to inactive instead of stopped
}

// Free allocated memory
void free_memory() {
    // Free service list
    Service* service_current = service_list;
    while (service_current != NULL) {
        Service* temp = service_current;
        service_current = service_current->next;
        free(temp);
    }
    service_list = NULL;
    
    // Free log stack
    LogEntry* log_current = log_stack;
    while (log_current != NULL) {
        LogEntry* temp = log_current;
        log_current = log_current->next;
        free(temp);
    }
    log_stack = NULL;
    
    // Free failed queue
    FailedService* failed_current = failed_queue_front;
    while (failed_current != NULL) {
        FailedService* temp = failed_current;
        failed_current = failed_current->next;
        free(temp);
    }
    failed_queue_front = failed_queue_rear = NULL;
    failed_queue_size = 0;
    
    // Note: BST nodes point to Service objects that are already freed above
    // so we don't need to free them separately
    // A proper recursive free_bst function would be better in a production setting
    service_bst = NULL; 
}


// *** SAMPLE MAIN FUNCTION TO DEMONSTRATE THE CHOICE ***
int main() {
    int choice = 0;

    while (1) {
        printf("\n============================================\n");
        printf("           Service Monitor Utility\n");
        printf("============================================\n");
        printf("1. View/Manage Services (Using Data Structures)\n");
        printf("2. View All System Processes (ps aux - Real-time)\n");
        printf("3. Monitor Services (Automatic Refresh)\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            // Clear input buffer
            while (getchar() != '\n');
            choice = -1;
        }

        switch (choice) {
            case 1:
                // Option to manage Services (retains existing functionality)
                // First, ensure service list is up to date (or load if empty)
                if (service_list == NULL) {
                    load_services_from_system();
                }
                display_all_services();
                // You would add a submenu here for start/stop/restart/search/filter
                break;
            case 2:
                // Option to view ALL processes (the new requirement)
                list_all_processes();
                break;
            case 3:
                monitor_services();
                break;
            case 0:
                printf("Exiting utility. Freeing memory...\n");
                free_memory();
                return 0;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
    
    return 0;
}
