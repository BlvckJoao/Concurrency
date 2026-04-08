#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define MAX_BUF     4096
#define NUM_SENSORS 3

/* ---------- per-sensor stats ---------- */
typedef struct {
        char   name[32];
        int    last_seq;
        long   received;
        long   lost;
        double min_val;
        double max_val;
        double sum;
} SensorStats;

static SensorStats stats[NUM_SENSORS];
static int         nstats = 0;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

SensorStats *find_or_create_stats(const char *name) {
        for (int i = 0; i < nstats; i++)
                if (strcmp(stats[i].name, name) == 0)
                        return &stats[i];
        if (nstats >= NUM_SENSORS) return NULL;
        SensorStats *s = &stats[nstats++];
        memset(s, 0, sizeof(*s));
        strncpy(s->name, name, 31);
        s->min_val = 1e9;
        s->max_val = -1e9;
        s->last_seq = 0;
        return s;
}

/* ---------- UDP datagram parser ---------- */
/*
 * Format: SEQ:<n>|SENSOR:<tipo>|VALUE:<v>|UNIT:<u>|TS:<ts>
 */
void parse_and_display_udp(const char *buf) {
        int    seq = 0;
        char   sensor[32] = "", unit[16] = "";
        double value = 0.0;
        long   ts    = 0;

        char tmp[512];
        strncpy(tmp, buf, sizeof(tmp) - 1);

        char *tok = strtok(tmp, "|");
        while (tok) {
                if (strncmp(tok, "SEQ:",    4) == 0) seq   = atoi(tok + 4);
                else if (strncmp(tok, "SENSOR:", 7) == 0) strncpy(sensor, tok + 7, 31);
                else if (strncmp(tok, "VALUE:",  6) == 0) value = atof(tok + 6);
                else if (strncmp(tok, "UNIT:",   5) == 0) strncpy(unit,   tok + 5, 15);
                else if (strncmp(tok, "TS:",     3) == 0) ts    = atol(tok + 3);
                tok = strtok(NULL, "|");
        }

        printf("[UDP] SEQ:%d | %s | %.2f %s | TS:%ld\n",
               seq, sensor, value, unit, ts);
        fflush(stdout);

        pthread_mutex_lock(&stats_mutex);
        SensorStats *s = find_or_create_stats(sensor);
        if (s) {
                if (s->last_seq > 0 && seq > s->last_seq + 1)
                        s->lost += seq - s->last_seq - 1;
                s->last_seq = seq;
                s->received++;
                if (value < s->min_val) s->min_val = value;
                if (value > s->max_val) s->max_val = value;
                s->sum += value;
        }
        pthread_mutex_unlock(&stats_mutex);
}

/* ---------- send a TCP command ---------- */
void send_command(int tcp_fd, const char *line) {
        /* parse local command aliases → TCP protocol */
        char upper[128];
        strncpy(upper, line, 127);
        /* trim newline */
        char *nl = strchr(upper, '\n'); if (nl) *nl = '\0';
        nl = strchr(upper, '\r'); if (nl) *nl = '\0';

        char out[MAX_BUF] = "";

        if (strncmp(upper, "START /sensor/", 14) == 0) {
                /* expect "START /sensor/<tipo> [UdpPort]" — we add UdpPort via arg */
                /* already well-formed; caller appended UdpPort header */
                snprintf(out, sizeof(out), "%s\r\n\r\n", upper);
        } else if (strncmp(upper, "STOP /sensor/", 13) == 0) {
                snprintf(out, sizeof(out), "%s\r\n\r\n", upper);
        } else if (strncmp(upper, "STATUS", 6) == 0) {
                snprintf(out, sizeof(out), "%s\r\n\r\n", upper);
        } else if (strcmp(upper, "EXIT /") == 0 || strcmp(upper, "EXIT") == 0) {
                snprintf(out, sizeof(out), "EXIT /\r\n\r\n");
        } else {
                /* pass as-is (user typed full protocol line) */
                snprintf(out, sizeof(out), "%s\r\n\r\n", upper);
        }

        send(tcp_fd, out, strlen(out), 0);
}

/* ---------- print local stats ---------- */
void print_stats(void) {
        pthread_mutex_lock(&stats_mutex);
        printf("=== Estatisticas Locais ===\n");
        for (int i = 0; i < nstats; i++) {
                SensorStats *s = &stats[i];
                long total    = s->received + s->lost;
                double pct    = total > 0 ? (100.0 * s->lost / total) : 0.0;
                double media  = s->received > 0 ? s->sum / s->received : 0.0;
                printf("%s: recebidos=%ld, perdidos=%ld (%.1f%%), "
                       "min=%.2f, max=%.2f, media=%.2f\n",
                       s->name, s->received, s->lost, pct,
                       s->min_val < 1e8 ? s->min_val : 0.0,
                       s->max_val > -1e8 ? s->max_val : 0.0,
                       media);
        }
        pthread_mutex_unlock(&stats_mutex);
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
        if (argc < 4) {
                fprintf(stderr,
                        "Uso: %s <servidor_ip> <tcp_port> <udp_port>\n", argv[0]);
                return 1;
        }

        const char *srv_ip  = argv[1];
        int         tcp_port = atoi(argv[2]);
        int         udp_port = atoi(argv[3]);

        /* TCP socket */
        int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_fd < 0) { perror("socket TCP"); return 1; }

        struct sockaddr_in srv_addr = {0};
        srv_addr.sin_family = AF_INET;
        srv_addr.sin_port   = htons(tcp_port);
        if (inet_pton(AF_INET, srv_ip, &srv_addr.sin_addr) <= 0) {
                fprintf(stderr, "IP invalido: %s\n", srv_ip);
                return 1;
        }

        if (connect(tcp_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
                perror("connect TCP"); return 1;
        }
        printf("Conectado ao servidor %s:%d\n", srv_ip, tcp_port);

        /* UDP socket */
        int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_fd < 0) { perror("socket UDP"); return 1; }

        struct sockaddr_in udp_addr = {0};
        udp_addr.sin_family      = AF_INET;
        udp_addr.sin_addr.s_addr = INADDR_ANY;
        udp_addr.sin_port        = htons(udp_port);

        if (bind(udp_fd, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
                perror("bind UDP"); return 1;
        }
        printf("Socket UDP escutando na porta %d\n", udp_port);
        printf("Digite comandos (START /sensor/<tipo>, STOP /sensor/<tipo>, "
               "STATUS /sensors, stats, quit):\n");

        /* accumulation buffer for TCP responses */
        char tcp_buf[MAX_BUF];
        int  tcp_len = 0;

        int running = 1;

        while (running) {
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(tcp_fd,      &rfds);
                FD_SET(udp_fd,      &rfds);
                FD_SET(STDIN_FILENO, &rfds);

                int maxfd = tcp_fd > udp_fd ? tcp_fd : udp_fd;
                if (STDIN_FILENO > maxfd) maxfd = STDIN_FILENO;

                struct timeval tv = {0, 100000}; /* 100ms */
                int ret = select(maxfd + 1, &rfds, NULL, NULL, &tv);
                if (ret < 0) { perror("select"); break; }

                /* --- TCP response --- */
                if (FD_ISSET(tcp_fd, &rfds)) {
                        char tmp[512];
                        int n = recv(tcp_fd, tmp, sizeof(tmp) - 1, 0);
                        if (n <= 0) { printf("Servidor fechou conexao.\n"); break; }
                        tmp[n] = '\0';

                        if (tcp_len + n < (int)sizeof(tcp_buf) - 1) {
                                memcpy(tcp_buf + tcp_len, tmp, n);
                                tcp_len += n;
                                tcp_buf[tcp_len] = '\0';
                        }

                        /* print complete responses */
                        char *end;
                        while ((end = strstr(tcp_buf, "\r\n\r\n")) != NULL) {
                                *end = '\0';
                                /* condense response to one line for display */
                                char display[MAX_BUF];
                                strncpy(display, tcp_buf, sizeof(display) - 1);
                                /* replace \r\n with " | " */
                                char *p;
                                while ((p = strstr(display, "\r\n")) != NULL) {
                                        memmove(p + 3, p + 2, strlen(p + 2) + 1);
                                        p[0] = ' '; p[1] = '|'; p[2] = ' ';
                                }
                                printf("Resposta: %s\n", display);
                                fflush(stdout);

                                /* shift buffer */
                                int consumed = (int)(end - tcp_buf) + 4;
                                tcp_len -= consumed;
                                if (tcp_len > 0)
                                        memmove(tcp_buf, tcp_buf + consumed, tcp_len);
                                tcp_buf[tcp_len] = '\0';
                        }
                }

                /* --- UDP data --- */
                if (FD_ISSET(udp_fd, &rfds)) {
                        char udp_buf[512];
                        int n = recv(udp_fd, udp_buf, sizeof(udp_buf) - 1, 0);
                        if (n > 0) {
                                udp_buf[n] = '\0';
                                parse_and_display_udp(udp_buf);
                        }
                }

                /* --- stdin command --- */
                if (FD_ISSET(STDIN_FILENO, &rfds)) {
                        char line[256];
                        if (!fgets(line, sizeof(line), stdin)) break;

                        /* trim */
                        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
                        nl = strchr(line, '\r'); if (nl) *nl = '\0';

                        if (strlen(line) == 0) { printf("> "); fflush(stdout); continue; }

                        printf("> %s\n", line);

                        if (strcmp(line, "stats") == 0) {
                                print_stats();
                                continue;
                        }

                        if (strcmp(line, "quit") == 0) {
                                printf("Enviando EXIT ao servidor...\n");
                                send(tcp_fd, "EXIT /\r\n\r\n",
                                     strlen("EXIT /\r\n\r\n"), 0);
                                running = 0;
                                continue;
                        }

                        /* inject UdpPort header for START commands */
                        char out[512];
                        if (strncmp(line, "START /sensor/", 14) == 0) {
                                snprintf(out, sizeof(out),
                                        "%s\r\nUdpPort: %d\r\n\r\n", line, udp_port);
                        } else {
                                snprintf(out, sizeof(out), "%s\r\n\r\n", line);
                        }

                        send(tcp_fd, out, strlen(out), 0);
                }
        }

        close(tcp_fd);
        close(udp_fd);
        printf("Conexao encerrada.\n");
        return 0;
}