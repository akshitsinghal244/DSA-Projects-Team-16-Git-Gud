#include "func.h"

int main() {
    int choice;
    char service_name[MAX_SERVICE_NAME];
    int filter_choice;
    
    printf("=== Advanced Service Management System ===\n");
    load_services_from_system();
    
    while (1) {
        printf("\n=== Main Menu ===\n");
        printf("1. Display All Services and Their Status\n");
        printf("2. Search Service by Name\n");
        printf("3. Filter Services by Status\n");
        printf("4. Start a Service\n");
        printf("5. Stop a Service\n");
        printf("6. Restart a Service\n");
        printf("7. Detect Failed/Unresponsive Services\n");
        printf("8. Process Failed Services Queue\n");
        printf("9. View Service Logs and History\n");
        printf("10. Monitor Services (Auto-refresh)\n");
        printf("11. Exit\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input!\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        getchar(); // Consume newline
        
        switch (choice) {
            case 1:
                display_all_services();
                break;
                
            case 2:
                printf("Enter service name to search: ");
                fgets(service_name, sizeof(service_name), stdin);
                service_name[strcspn(service_name, "\n")] = 0;
                search_service_by_name(service_name);
                break;
                
            case 3:
                printf("Filter by status:\n");
                printf("1. Active\n2. Inactive\n3. Failed\n4. Running\n5. Stopped\n");
                printf("Enter choice: ");
                if (scanf("%d", &filter_choice) != 1) {
                    printf("Invalid input!\n");
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                
                switch (filter_choice) {
                    case 1: filter_services_by_status(STATUS_ACTIVE); break;
                    case 2: filter_services_by_status(STATUS_INACTIVE); break;
                    case 3: filter_services_by_status(STATUS_FAILED); break;
                    case 4: filter_services_by_status(STATUS_RUNNING); break;
                    case 5: filter_services_by_status(STATUS_STOPPED); break;
                    default: printf("Invalid choice!\n");
                }
                break;
                
            case 4:
                printf("Enter service name to start: ");
                fgets(service_name, sizeof(service_name), stdin);
                service_name[strcspn(service_name, "\n")] = 0;
                start_service(service_name);
                break;
                
            case 5:
                printf("Enter service name to stop: ");
                fgets(service_name, sizeof(service_name), stdin);
                service_name[strcspn(service_name, "\n")] = 0;
                stop_service(service_name);
                break;
                
            case 6:
                printf("Enter service name to restart: ");
                fgets(service_name, sizeof(service_name), stdin);
                service_name[strcspn(service_name, "\n")] = 0;
                restart_service(service_name);
                break;
                
            case 7:
                detect_failed_services();
                break;
                
            case 8:
                process_failed_services();
                break;
                
            case 9:
                display_logs();
                break;
                
            case 10:
                monitor_services();
                break;
                
            case 11:
                free_memory();
                printf("Exiting... Goodbye!\n");
                return 0;
                
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
    
    return 0;
}