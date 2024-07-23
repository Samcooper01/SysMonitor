/*
 * Program Name: Sam's SysMonitor
 * Description: Monitors CPU, Memory and Network stats
 * Author: Samuel Cooper
 * Date: [Date of creation or last update]
 * Version: 1.0
 *
 * Compilation: gcc -o sys_mon main.c
 * Usage: ./sys_mon usage
 *
 *
 * Notes:
 *      None.
 *
 * Change Log:
 *      None.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXIT_FAILED                 1
#define EXIT_SUCCESS                0

#define CPU_STATS_FILEPATH          "/proc/stat"
#define MEM_INFO_FILEPATH           "/proc/meminfo"
#define NETWORK_ACTIVITY_FILEPATH   "/proc/net/dev"

#define MAX_PROCFILE_LINE_LENGTH    8192
#define MAX_PROCFILE_TOKEN_AMOUNT   12
#define NUM_CPU_LINES               17 //This may change depending on number of cores CPU has
#define MAX_CPU_DATA_SIZE           34
#define MAX_MEM_DATA_SIZE           42
#define MAX_NETWORK_DATA_SIZE       42
#define MAX_NETWORK_DEVICES         8

typedef char* string_t;

FILE *proc_stats_file;
FILE *mem_info_file;
FILE *network_activity_file;

struct cpu_line
{
    string_t name;
    string_t user_mode;
    string_t nice_time;
    string_t system_mode_time;
    string_t idle_time;
    string_t I_O_wait_time;
    string_t IRQ_time;
    string_t soft_IRQ_time;
    string_t steal_time;
    string_t guest_time;
    string_t guest_nice_time;
};

struct cpu_stats
{
    struct cpu_line cpu[NUM_CPU_LINES]; //There are 16 lines for each cpu core and 1 line for entire cpu
    string_t num_context_switches;
    string_t boot_time;
    string_t num_proccesses_created;
    string_t proccesses_running;
    string_t proccesses_blocked;
};

struct mem_info
{
    string_t mem_total;
    string_t mem_free;
    string_t mem_available;
    string_t buffers;
    string_t cached;
    string_t active;
    string_t inactive;
    string_t dirty;
    string_t page_tables;
    string_t percpu;
    string_t hardware_corrupted;
};

struct network_device
{
    string_t face;
    string_t r_bytes;
    string_t r_packets;
    string_t r_errs;
    string_t r_drop;
    string_t r_fifo;
    string_t r_frame;
    string_t r_compressed;
    string_t r_multicast;
    string_t t_bytes;
    string_t t_packets;
    string_t t_errs;
    string_t t_drop;
    string_t t_fifo;
    string_t t_frame;
    string_t t_compressed;
};

struct network_info
{
    struct network_device devices[MAX_NETWORK_DEVICES];
    int num_devices;
};

struct cpu_stats cpu_stats;
struct mem_info mem_info;
struct network_info network_info;

/*
* @brief Prints error message from caller and exits program with exit failure.
*/
int fatal_error(char * error_msg, char * additional_text)
{
    printf("ERROR: ");
    printf("%s", error_msg);
    printf("%s.\n", additional_text);
    exit(EXIT_FAILED);
}

/*
* @brief Opens main files for system monitoring.
*/
void open_proc_files()
{
    proc_stats_file = fopen(CPU_STATS_FILEPATH, "r");
    if(proc_stats_file == NULL) fatal_error("proc stats failed to open at", CPU_STATS_FILEPATH);

    mem_info_file = fopen(MEM_INFO_FILEPATH, "r");
    if(mem_info_file == NULL) fatal_error("mem info failed to open at", MEM_INFO_FILEPATH);

    network_activity_file = fopen(NETWORK_ACTIVITY_FILEPATH, "r");
    if(network_activity_file == NULL) fatal_error("net dev failed to open at", NETWORK_ACTIVITY_FILEPATH);
}

/*
* @breif Closes main files for system monitoring.
*/
void close_proc_files()
{
    if(proc_stats_file != NULL) fclose(proc_stats_file);
    if(mem_info_file != NULL) fclose(mem_info_file);
    if(network_activity_file != NULL) fclose(network_activity_file);
}

/*
* @brief Gets list of tokens from line
*
* @returns the first token from the line
*/
char* tokens_from_line(char *token_list[], char * line, int *num_tokens)
{
    if (line == NULL) return NULL;

    char *first_token = strtok(line, " ");
    if(strcmp(first_token, "intr") == 0) return NULL; //We skip this one
                                           //as it is 4000 tokens

    int index = 0;
    token_list[index] = first_token;
    char *token;
    while((token = strtok(NULL, " ")) != NULL)
    {
        token_list[index] = token;
        index++;
    }

    *num_tokens = index + 1;
    return first_token;
}

/*
* @breif Update the cpu line indicated by cpu_index with the list of tokens
*/
void update_cpu_index(int cpu_index, char * first_token, char *token_list[])
{
    strcpy(cpu_stats.cpu[cpu_index].name, first_token);
    strcpy(cpu_stats.cpu[cpu_index].user_mode, token_list[0]);
    strcpy(cpu_stats.cpu[cpu_index].nice_time, token_list[1]);
    strcpy(cpu_stats.cpu[cpu_index].system_mode_time, token_list[2]);
    strcpy(cpu_stats.cpu[cpu_index].idle_time, token_list[3]);
    strcpy(cpu_stats.cpu[cpu_index].I_O_wait_time, token_list[4]);
    strcpy(cpu_stats.cpu[cpu_index].IRQ_time, token_list[5]);
    strcpy(cpu_stats.cpu[cpu_index].soft_IRQ_time, token_list[6]);
    strcpy(cpu_stats.cpu[cpu_index].steal_time, token_list[7]);
    strcpy(cpu_stats.cpu[cpu_index].guest_time, token_list[8]);
    strcpy(cpu_stats.cpu[cpu_index].guest_nice_time, token_list[9]);
}

/*
* @breif Updates cpu_stats struct with latest from proc file.
*/
void update_cpu_stats()
{
    char proc_line_buffer[MAX_PROCFILE_LINE_LENGTH];

    while(fgets(proc_line_buffer, sizeof(proc_line_buffer), proc_stats_file) != NULL)
    {
        char * token_list[MAX_PROCFILE_TOKEN_AMOUNT];
        int num_tokens;
        char *first_token = tokens_from_line(token_list, proc_line_buffer, &num_tokens);
        if(first_token == NULL) continue;
        if(num_tokens < 1) continue;

        if(strcmp(first_token, "cpu") == 0) update_cpu_index(0, first_token, token_list);
        if(strcmp(first_token, "cpu0") == 0) update_cpu_index(1, first_token, token_list);
        if(strcmp(first_token, "cpu1") == 0) update_cpu_index(2, first_token, token_list);
        if(strcmp(first_token, "cpu2") == 0) update_cpu_index(3, first_token, token_list);
        if(strcmp(first_token, "cpu3") == 0) update_cpu_index(4, first_token, token_list);
        if(strcmp(first_token, "cpu4") == 0) update_cpu_index(5, first_token, token_list);
        if(strcmp(first_token, "cpu5") == 0) update_cpu_index(6, first_token, token_list);
        if(strcmp(first_token, "cpu6") == 0) update_cpu_index(7, first_token, token_list);
        if(strcmp(first_token, "cpu7") == 0) update_cpu_index(8, first_token, token_list);
        if(strcmp(first_token, "cpu8") == 0) update_cpu_index(9, first_token, token_list);
        if(strcmp(first_token, "cpu9") == 0) update_cpu_index(10, first_token, token_list);
        if(strcmp(first_token, "cpu10") == 0) update_cpu_index(11, first_token, token_list);
        if(strcmp(first_token, "cpu11") == 0) update_cpu_index(12, first_token, token_list);
        if(strcmp(first_token, "cpu12") == 0) update_cpu_index(13, first_token, token_list);
        if(strcmp(first_token, "cpu13") == 0) update_cpu_index(14, first_token, token_list);
        if(strcmp(first_token, "cpu14") == 0) update_cpu_index(15, first_token, token_list);
        if(strcmp(first_token, "cpu15") == 0) update_cpu_index(16, first_token, token_list);
        if(strcmp(first_token, "ctxt") == 0) strcpy(cpu_stats.num_context_switches, token_list[0]);
        if(strcmp(first_token, "btime") == 0) strcpy(cpu_stats.boot_time, token_list[0]);
        if(strcmp(first_token, "processes") == 0) strcpy(cpu_stats.num_proccesses_created, token_list[0]);
        if(strcmp(first_token, "procs_running") == 0) strcpy(cpu_stats.proccesses_running, token_list[0]);
        if(strcmp(first_token, "procs_blocked") == 0) strcpy(cpu_stats.proccesses_blocked, token_list[0]);
    }

}

/*
* @breif Updates mem_info struct with newest data from file
*/
void update_meminfo()
{
    char proc_line_buffer[MAX_PROCFILE_LINE_LENGTH];
    while(fgets(proc_line_buffer, sizeof(proc_line_buffer), mem_info_file) != NULL)
    {
        char * token_list[MAX_PROCFILE_TOKEN_AMOUNT];
        int num_tokens;
        char *first_token = tokens_from_line(token_list, proc_line_buffer, &num_tokens);
        //printf("TOKEN %s\n", first_token);
        if(strcmp(first_token, "MemTotal:") == 0) strcpy(mem_info.mem_total, token_list[0]);
        if(strcmp(first_token, "MemFree:") == 0) strcpy(mem_info.mem_free, token_list[0]);
        if(strcmp(first_token, "MemAvailable:") == 0) strcpy(mem_info.mem_available, token_list[0]);
        if(strcmp(first_token, "Buffers:") == 0) strcpy(mem_info.buffers, token_list[0]);
        if(strcmp(first_token, "Cached:") == 0) strcpy(mem_info.cached, token_list[0]);
        if(strcmp(first_token, "Active:") == 0) strcpy(mem_info.active, token_list[0]);
        if(strcmp(first_token, "Inactive:") == 0) strcpy(mem_info.inactive, token_list[0]);
        if(strcmp(first_token, "Dirty:") == 0) strcpy(mem_info.dirty, token_list[0]);
        if(strcmp(first_token, "PageTables:") == 0) strcpy(mem_info.page_tables, token_list[0]);
        if(strcmp(first_token, "Percpu:") == 0) strcpy(mem_info.percpu, token_list[0]);
        if(strcmp(first_token, "HardwareCorrupted:") == 0) strcpy(mem_info.hardware_corrupted, token_list[0]);
    }
}

/*
* @breif helper for function update_network_info
*        copies line_buffer to device struct in netwwork struct at index device_index
*/
void copy_to_network_struct(int device_index, char * line_buffer)
{
    strcpy(network_info.devices[device_index].face, strtok(line_buffer, " "));
    strcpy(network_info.devices[device_index].r_bytes, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].r_packets, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].r_errs, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].r_drop, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].r_fifo, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].r_frame, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].r_compressed, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].r_multicast, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].t_bytes, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].t_packets, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].t_errs, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].t_drop, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].t_fifo, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].t_frame, strtok(NULL, " "));
    strcpy(network_info.devices[device_index].t_compressed, strtok(NULL, " "));
}

/*
* @breif updates network info struct
*/
void update_network_info()
{
    char proc_line_buffer[MAX_PROCFILE_LINE_LENGTH];
    int line_index = 0;
    int device_index = 0;
    while(fgets(proc_line_buffer, sizeof(proc_line_buffer), network_activity_file) != NULL)
    {
        if(line_index < 2) {line_index++; continue;}

        copy_to_network_struct(device_index, proc_line_buffer);

        device_index++;
        if(device_index >= MAX_NETWORK_DEVICES) break; //This application only supports 8 network devices
    }
    network_info.num_devices = device_index;
}

/*
* @breif prints mem_info struct
*/
void display_mem_info()
{
    printf("MemTotal: %s\n", mem_info.mem_total);
    printf("MemFree: %s\n", mem_info.mem_free);
    printf("MemAvailable: %s\n", mem_info.mem_available);
    printf("Buffers: %s\n", mem_info.buffers);
    printf("Cached: %s\n", mem_info.cached);
    printf("Active: %s\n", mem_info.active);
    printf("Inactive: %s\n", mem_info.inactive);
    printf("Dirty: %s\n", mem_info.dirty);
    printf("PageTables: %s\n", mem_info.page_tables);
    printf("percpu: %s\n", mem_info.percpu);
    printf("HardwareCorrupted: %s\n\n", mem_info.hardware_corrupted);
}

/*
* @breif Prints cpu_stats struct
*/
void display_cpu_proc()
{
    //Table Header
    printf("Name | ");
    printf("User mode | ");
    printf("Nice Time | ");
    printf("System Mode time | ");
    printf("Idle Time     | ");
    printf("I/O Wait Time | ");
    printf("IRQ Time | ");
    printf("Soft IRQ Time | ");
    printf("Steal Time | ");
    printf("Guest Time | ");
    printf("Guest Nice Time\n");
    //CPU lines
    for(int i = 0; i < NUM_CPU_LINES; i++) {
        printf("%4s | ", cpu_stats.cpu[i].name);
        printf(" %8s | ", cpu_stats.cpu[i].user_mode);
        printf(" %8s | ", cpu_stats.cpu[i].nice_time);
        printf(" %15s | ", cpu_stats.cpu[i].system_mode_time);
        printf(" %12s | ", cpu_stats.cpu[i].idle_time);
        printf(" %12s | ", cpu_stats.cpu[i].I_O_wait_time);
        printf(" %7s | ", cpu_stats.cpu[i].IRQ_time);
        printf(" %12s | ", cpu_stats.cpu[i].soft_IRQ_time);
        printf(" %9s | ", cpu_stats.cpu[i].steal_time);
        printf(" %9s | ", cpu_stats.cpu[i].guest_time);
        printf(" %15s", cpu_stats.cpu[i].guest_nice_time);
    }
    printf("Context Switches: %s", cpu_stats.num_context_switches);
    printf("Boot Time: %s", cpu_stats.boot_time);
    printf("Total processes Created: %s", cpu_stats.num_proccesses_created);
    printf("Processes Running: %s", cpu_stats.proccesses_running);
    printf("Processes Blocked: %s\n\n", cpu_stats.proccesses_blocked);
}

/*
* @breif display network info struct
*/
void display_network_info()
{
    printf("-------------------------------------------------------------------");
    printf("-------------------------------------------------------------------\n");
    printf("Face         | ");
    printf("R Bytes      | ");
    printf("R Packets    | ");
    printf("R errs       | ");
    printf("R drop       | ");
    printf("R fifo       | ");
    printf("R frame      | ");
    printf("R compressed | ");
    printf("R multicast  |");
    printf("\n             ");
    printf("| T Bytes      | ");
    printf("T Packets    | ");
    printf("T errs       | ");
    printf("T drop       | ");
    printf("T fifo       | ");
    printf("T frame      | ");
    printf("T compressed | \n");
    printf("-------------------------------------------------------------------");
    printf("-------------------------------------------------------------------\n");
    for(int i = 0; i < network_info.num_devices;i++)
    {
        printf("%12s |", network_info.devices[i].face);
        printf("%13s |", network_info.devices[i].r_bytes);
        printf("%13s |", network_info.devices[i].r_packets);
        printf("%13s |", network_info.devices[i].r_errs);
        printf("%13s |", network_info.devices[i].r_drop);
        printf("%13s |", network_info.devices[i].r_fifo);
        printf("%13s |", network_info.devices[i].r_frame);
        printf("%13s |", network_info.devices[i].r_compressed);
        printf("%13s |", network_info.devices[i].r_multicast);
        printf("\n             |");
        printf("%13s |", network_info.devices[i].t_bytes);
        printf("%13s |", network_info.devices[i].t_packets);
        printf("%13s |", network_info.devices[i].t_errs);
        printf("%13s |", network_info.devices[i].t_drop);
        printf("%13s |", network_info.devices[i].t_fifo);
        printf("%13s |", network_info.devices[i].t_frame);
        printf("%13s |\n", network_info.devices[i].t_compressed);
        printf("-------------------------------------------------------------------");
        printf("-------------------------------------------------------------------\n\n");
    }
}

/*
* @breif allocates space for cpu_struct dynamically
*/
void alloc_cpu_struct()
{
    for(int i = 0; i < NUM_CPU_LINES; i++) {
        cpu_stats.cpu[i].name = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].user_mode = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].nice_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].system_mode_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].idle_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].I_O_wait_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].IRQ_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].soft_IRQ_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].steal_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].guest_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
        cpu_stats.cpu[i].guest_nice_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
    }
    cpu_stats.num_context_switches = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
    cpu_stats.boot_time = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
    cpu_stats.num_proccesses_created = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
    cpu_stats.proccesses_running = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
    cpu_stats.proccesses_blocked = (char*)malloc(MAX_CPU_DATA_SIZE * sizeof(char));
}

/*
* @breif Frees space allocated to cpu_struct
*/
void free_cpu_struct()
{
    for(int i = 0; i < NUM_CPU_LINES; i++) {
        if(cpu_stats.cpu[i].name != NULL) free(cpu_stats.cpu[i].name);
        if(cpu_stats.cpu[i].user_mode != NULL) free(cpu_stats.cpu[i].user_mode);
        if(cpu_stats.cpu[i].nice_time != NULL) free(cpu_stats.cpu[i].nice_time);
        if(cpu_stats.cpu[i].system_mode_time != NULL) free(cpu_stats.cpu[i].system_mode_time);
        if(cpu_stats.cpu[i].idle_time != NULL) free(cpu_stats.cpu[i].idle_time);
        if(cpu_stats.cpu[i].I_O_wait_time != NULL) free(cpu_stats.cpu[i].I_O_wait_time);
        if(cpu_stats.cpu[i].IRQ_time != NULL) free(cpu_stats.cpu[i].IRQ_time);
        if(cpu_stats.cpu[i].soft_IRQ_time != NULL) free(cpu_stats.cpu[i].soft_IRQ_time);
        if(cpu_stats.cpu[i].steal_time != NULL) free(cpu_stats.cpu[i].steal_time);
        if(cpu_stats.cpu[i].guest_time != NULL) free(cpu_stats.cpu[i].guest_time);
        if(cpu_stats.cpu[i].guest_nice_time != NULL) free(cpu_stats.cpu[i].guest_nice_time);
    }
    if(cpu_stats.num_context_switches != NULL) free(cpu_stats.num_context_switches);
    if(cpu_stats.boot_time != NULL) free(cpu_stats.boot_time);
    if(cpu_stats.num_proccesses_created != NULL) free(cpu_stats.num_proccesses_created);
    if(cpu_stats.proccesses_running != NULL) free(cpu_stats.proccesses_running);
    if(cpu_stats.proccesses_blocked != NULL) free(cpu_stats.proccesses_blocked);
}

/*
* @breif allocates space for mem_info struct
*/
void alloc_mem_info_struct()
{
    mem_info.active = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.buffers = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.cached = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.dirty = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.hardware_corrupted = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.inactive = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.mem_available = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.mem_free = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.mem_total = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.page_tables = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
    mem_info.percpu = (char *)malloc(MAX_MEM_DATA_SIZE * sizeof(char));
}

/*
* @breif Frees the mem_info struct
*/
void free_mem_info_struct()
{
    if(mem_info.active != NULL) free(mem_info.active);
    if(mem_info.buffers != NULL) free(mem_info.buffers);
    if(mem_info.cached != NULL) free(mem_info.cached);
    if(mem_info.dirty != NULL) free(mem_info.dirty);
    if(mem_info.hardware_corrupted != NULL) free(mem_info.hardware_corrupted);
    if(mem_info.inactive != NULL) free(mem_info.inactive);
    if(mem_info.mem_available != NULL) free(mem_info.mem_available);
    if(mem_info.mem_free != NULL) free(mem_info.mem_free);
    if(mem_info.mem_total != NULL) free(mem_info.mem_total);
    if(mem_info.page_tables != NULL) free(mem_info.page_tables);
    if(mem_info.percpu != NULL) free(mem_info.percpu);
}

/*
* @breif allocate network info struct
*/
void alloc_network_info_struct()
{
    for(int i = 0; i < MAX_NETWORK_DEVICES; i++)
    {
        network_info.devices[i].face = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].r_bytes = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].r_packets = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].r_errs = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].r_drop = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].r_fifo = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].r_frame = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].r_compressed = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].r_multicast = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].t_bytes = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].t_packets = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].t_errs = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].t_drop = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].t_fifo = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].t_frame = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
        network_info.devices[i].t_compressed = (char*)malloc(MAX_NETWORK_DATA_SIZE * sizeof(char));
    }
}

/*
* @breif free network info struct
*/
void free_network_info_struct()
{
    for(int i = 0; i < MAX_NETWORK_DEVICES; i++)
    {
        if(network_info.devices[i].face != NULL) free(network_info.devices[i].face);
        if(network_info.devices[i].r_bytes != NULL) free(network_info.devices[i].r_bytes);
        if(network_info.devices[i].r_packets != NULL) free(network_info.devices[i].r_packets);
        if(network_info.devices[i].r_errs != NULL) free(network_info.devices[i].r_errs);
        if(network_info.devices[i].r_drop != NULL) free(network_info.devices[i].r_drop);
        if(network_info.devices[i].r_fifo != NULL) free(network_info.devices[i].r_fifo);
        if(network_info.devices[i].r_frame != NULL) free(network_info.devices[i].r_frame);
        if(network_info.devices[i].r_compressed != NULL) free(network_info.devices[i].r_compressed);
        if(network_info.devices[i].r_multicast != NULL) free(network_info.devices[i].r_multicast);
        if(network_info.devices[i].t_bytes != NULL) free(network_info.devices[i].t_bytes);
        if(network_info.devices[i].t_packets != NULL) free(network_info.devices[i].t_packets);
        if(network_info.devices[i].t_errs != NULL) free(network_info.devices[i].t_errs);
        if(network_info.devices[i].t_drop != NULL) free(network_info.devices[i].t_drop);
        if(network_info.devices[i].t_fifo != NULL) free(network_info.devices[i].t_fifo);
        if(network_info.devices[i].t_frame != NULL) free(network_info.devices[i].t_frame);
        if(network_info.devices[i].t_compressed != NULL) free(network_info.devices[i].t_compressed);
    }
}

/*
* @breif Inits globals and allocates space for structs
*/
void init_progam()
{

    open_proc_files();

    alloc_cpu_struct();
    alloc_mem_info_struct();
    alloc_network_info_struct();
}

/*
* @breif Closes open files and frees space from structs
*/
void cleanup_program()
{

    free_cpu_struct();
    free_mem_info_struct();
    free_network_info_struct();

    close_proc_files();

}

/*
* @breif display program usage
*/
void print_usage()
{
    printf("Welcome to Sam's SysMonitor\n\n");
    printf("Run with one or more of the following arguments:\n");
    printf("cpu-stats            Displays cpu stats\n");
    printf("mem-info             Displays information on memory usage\n");
    printf("network-info         Display information on network info\n\n");
    printf("Run with only one of these arguments\n");
    printf("cpu-status-loop      Displays cpu stats on loop\n");
    printf("mem-info-loop        Displays information on memory usage on loop\n");
    printf("network-info-loop    Display information on network info on loop\n");
}

void cpu_status()
{
    open_proc_files();
    alloc_cpu_struct();

    update_cpu_stats();
    display_cpu_proc();

    free_cpu_struct();
    close_proc_files();
}

void mem_status()
{
    open_proc_files();
    alloc_mem_info_struct();

    update_meminfo();
    display_mem_info();

    free_mem_info_struct();
    close_proc_files();
}

void network_status()
{
    open_proc_files();
    alloc_network_info_struct();

    update_network_info();
    display_network_info();

    free_network_info_struct();
    close_proc_files();
}

void cpu_status_loop()
{
    while(1)
    {
        cpu_status();
        sleep(1);
        for(int i = 0; i < 25; i++) {printf("\033[A");} //Moves the cursor up
    }
}

void mem_info_loop()
{
    while(1)
    {
        mem_status();
        sleep(1);
        for(int i = 0; i < 12; i++) {printf("\033[A");} //Moves the cursor up
    }
}

void network_info_loop()
{
    while(1)
    {
        network_status();
        sleep(1);
        for(int i = 0; i < (4* network_info.num_devices + 4); i++) {printf("\033[A");}
        //Moves the cursor up by number of devices + header
    }
}

/*
* @breif execute argument
*/
void execute_arg(char * arg)
{
    if(strcmp(arg, "cpu-stats") == 0) {cpu_status();}
    else if(strcmp(arg, "mem-info") == 0) {update_meminfo(); display_mem_info();}
    else if(strcmp(arg, "network-info") == 0) {update_network_info(); display_network_info();}
    else if(strcmp(arg, "cpu-status-loop") == 0) {cpu_status_loop();}
    else if(strcmp(arg, "mem-info-loop") == 0) {mem_info_loop();}
    else if(strcmp(arg, "network-info-loop") == 0) {network_info_loop();}
    else {
        printf("Argument '%s' not recognized.\n", arg);
    }
}

/*
* @breif Main entry point
*/
int main(int argc, char **argv)
{
    if(argc <= 1) {print_usage(); exit(EXIT_SUCCESS);}

    init_progam();

    for(int i = 1; i < argc;i++)
    {
        execute_arg(argv[i]);
    }

    exit(EXIT_SUCCESS);
}