#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <linux/rpmsg.h>
#include <glob.h>
#include <time.h>

#define LOG_RAM     "/tmp/datalog.csv"
#define LOG_SD      "/var/log/datalog.csv"
#define FLUSH_COUNT 20
#define FLUSH_SEC   60

int main(void)
{
    char buf[128];
    int  n;
    char ctrl_dev[64];
    int  entry_count = 0;
    time_t last_flush = 0;

    /* Tự tìm rpmsg_ctrl device */
    glob_t g;
    if (glob("/dev/rpmsg_ctrl*", 0, NULL, &g) != 0 || g.gl_pathc == 0) {
        fprintf(stderr, "No rpmsg_ctrl device found\n");
        return 1;
    }
    snprintf(ctrl_dev, sizeof(ctrl_dev), "%s", g.gl_pathv[g.gl_pathc - 1]);
    globfree(&g);
    printf("[Logger] Using %s\n", ctrl_dev);

    int fd_ctrl = open(ctrl_dev, O_RDWR);
    if (fd_ctrl < 0) {
        perror("open rpmsg_ctrl");
        return 1;
    }

    struct rpmsg_endpoint_info ept_info = {
        .name = "rpmsg-datalogger-channel",
        .src  = 0,
        .dst  = 0x400,
    };

    if (ioctl(fd_ctrl, RPMSG_CREATE_EPT_IOCTL, &ept_info) < 0) {
        perror("ioctl create endpoint");
        close(fd_ctrl);
        return 1;
    }

    sleep(1);
    glob_t g2;
    char rpmsg_dev[64] = "/dev/rpmsg0";
    if (glob("/dev/rpmsg[0-9]*", 0, NULL, &g2) == 0 && g2.gl_pathc > 0) {
        snprintf(rpmsg_dev, sizeof(rpmsg_dev), "%s", g2.gl_pathv[g2.gl_pathc - 1]);
        globfree(&g2);
    }
    printf("[Logger] Using %s\n", rpmsg_dev);

    int fd_rpmsg = open(rpmsg_dev, O_RDWR);
    if (fd_rpmsg < 0) {
        perror("open rpmsg");
        close(fd_ctrl);
        return 1;
    }
    write(fd_rpmsg, "ping", 4);

    /* Mở file RAM */
    FILE *fp = fopen(LOG_RAM, "a");
    if (!fp) {
        perror("open log RAM");
        close(fd_rpmsg);
        close(fd_ctrl);
        return 1;
    }

    /* Header nếu file mới */
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0)
        fprintf(fp, "real_time,tick_ms,sensor,value1,value2\n");    

    /* Session marker */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestr[32];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);
    fprintf(fp, "# Session started: %s\n", timestr);
    fflush(fp);
    last_flush = now;

    printf("[Logger] Started at %s\n", timestr);
    printf("[Logger] RAM: %s | SD: %s\n", LOG_RAM, LOG_SD);
    fflush(stdout);

    while (1)
    {
        n = read(fd_rpmsg, buf, sizeof(buf) - 1);
        if (n > 0)
        {
            buf[n] = '\0';

            now = time(NULL);
            t = localtime(&now);
            strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);

            /* In terminal */
            printf("[%s] %s", timestr, buf);
            fflush(stdout);

            /* Ghi vào RAM ngay */
            fprintf(fp, "%s,%s", timestr, buf);
            fflush(fp);
            entry_count++;

            /* Flush ra SD theo điều kiện */
            if (entry_count % FLUSH_COUNT == 0 ||
                (now - last_flush) >= FLUSH_SEC)
            {
                /* Check disk full */
                struct statvfs st;
                if (statvfs("/var/log/", &st) == 0)
                {
                    long free_mb = (long)(st.f_bavail * st.f_frsize)
                                   / (1024 * 1024);
                    if (free_mb < 10)
                        printf("[Logger] WARNING: SD nearly full"
                               " (%ldMB left)\n", free_mb);
                }

                /* Flush RAM → SD */
                int ret = system("cp /tmp/datalog.csv"
                                 " /var/log/datalog.csv");
                if (ret != 0)
                    printf("[Logger] WARNING: flush failed!"
                           " Data safe in RAM.\n");
                else
                    printf("[Logger] Flushed to SD (%d entries)\n",
                           entry_count);

                last_flush = now;
                fflush(stdout);
            }
        }
    }

    fclose(fp);
    close(fd_rpmsg);
    close(fd_ctrl);
    return 0;
}