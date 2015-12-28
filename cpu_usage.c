#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

struct string {
    char *buf;
    size_t size;
};

#define FILENAME_S (512)
struct stat_entry {
    char filename[FILENAME_S];
    struct string data;
};

struct entry_data {
    struct stat_entry system_e;
    struct stat_entry process_e;
};

struct cpu_info {
    unsigned long long total;
};

struct cpu_usage {
    struct cpu_info system;
    struct cpu_info process;
};

int string_init(struct string *str, size_t claim)
{
    char *buf = malloc(claim);

    if (!buf) {
        printf("Allocate buffer fail: %s\n", strerror(errno));
        return -1;
    }

    buf[0] = '\0';
    str->buf = buf;

    return 0;
}

int read_first_line(char *fn, struct string *str)
{
    FILE *fp = NULL;

    fp = fopen(fn, "r");

    if (!fp) {
        printf("Open %s fail: %s\n", fn, strerror(errno));
        return -1;
    }

    /* Read one line */
    if (fgets(str->buf, str->size, fp) == NULL) {
        printf("Read line data fail: %s filename: %s\n", strerror(errno), fn);
        return -1;
    }

    fclose(fp);

    return 0;
}

int parse_system(struct string *str, struct cpu_info *info)
{
    unsigned long long num1, num2, num3, num4, num5, num6, num7, num8, num9, num10;
    int rc = sscanf(str->buf,
                    "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                    &num1, &num2, &num3, &num4, &num5, &num6, &num7, &num8, &num9, &num10);

    if (rc != 10) {
        printf("sscanf error!\n");
        return -1;
    }

    info->total = num1 + num2 + num3 + num4 + num5 + num6 + num7 + num8 + num9 + num10;

    return 0;
}

int parse_process(struct string *str, struct cpu_info *info)
{
    unsigned long long num1, num2;
    int rc = sscanf(str->buf,
                    "%*d %*s %*c %*d" //pid,command,state,ppid

                    "%*d %*d %*d %*d %*u %*u %*u %*u %*u"

                    "%llu %llu" //usertime,systemtime

                    "%*d %*d %*d %*d %*d %*d %*u"

                    "%*u", //virtual memory size in bytes
                    &num1, &num2); 

    if (rc != 2) {
        printf("sscanf error!\n");
        return -1;
    }

    info->total = num1 + num2;

    return 0;
}

int fetch_data(struct entry_data *en)
{
    read_first_line(en->system_e.filename, &en->system_e.data);
    read_first_line(en->process_e.filename, &en->process_e.data);

    return 0;
}

int analyze(struct entry_data *en, struct cpu_usage *usage)
{
    parse_system(&en->system_e.data, &usage->system);
    parse_process(&en->process_e.data, &usage->process);

    return 0;
}

int show_usage(struct cpu_usage *first, struct cpu_usage *second)
{
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    float res = num_cores * (second->process.total - first->process.total) / (float)(
                    second->system.total - first->system.total);
    printf("Usage(%d): %f\n", num_cores, res);

    return 0;
}

int init(int pid, struct entry_data *en)
{
    snprintf(en->system_e.filename, FILENAME_S, "/proc/stat");
    snprintf(en->process_e.filename, FILENAME_S, "/proc/%d/stat", pid);

    if (string_init(&en->system_e.data, 2048) < 0)
        return -1;

    if (string_init(&en->process_e.data, 2048) < 0)
        return -1;

    return 0;
}

int main(int argc, const char *argv[])
{
    int mpid = 0;
    struct entry_data data;
    struct cpu_usage first;
    struct cpu_usage second;

    mpid = atoi(argv[1]);
    init(mpid, &data);

    while (1) {
        fetch_data(&data);
        analyze(&data, &first);

        sleep(1);
        fetch_data(&data);
        analyze(&data, &second);

        show_usage(&first, &second);
        sleep(1);
    }
    
    return 0;
}
