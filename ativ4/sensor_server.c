#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define TCP_PORT_DEFAULT 9000
#define MAX_BUF 4096
#define NUM_SENSORS 3

typedef struct {
        char name[32];
        double value;
        double min_val;
        double max_val;
        char unit[8];
        int interval_ms;
        int active;
        int seq;
        pthread_t thread;
        pthread_mutex_t mutex;
        int udp_sock;
        struct sockaddr_in client_addr;
        int stop_flag;
} Sensor;

static Sensor sensors[NUM_SENSORS];

void init_sensors(void) {
        strcpy(sensors[0].name, "temperatura");
        sensors[0].value    = 25.0;
        sensors[0].min_val  = 15.0;
        sensors[0].max_val  = 40.0;
        strcpy(sensors[0].unit, "C");
        sensors[0].interval_ms = 100;

        strcpy(sensors[1].name, "umidade");
        sensors[1].value    = 60.0;
        sensors[1].min_val  = 30.0;
        sensors[1].max_val  = 90.0;
        strcpy(sensors[1].unit, "%");
        sensors[1].interval_ms = 200;

        strcpy(sensors[2].name, "pressao");
        sensors[2].value    = 1010.0;
        sensors[2].min_val  = 990.0;
        sensors[2].max_val  = 1030.0;
        strcpy(sensors[2].unit, "hPa");
        sensors[2].interval_ms = 500;

        for (int i = 0; i < NUM_SENSORS; i++) {
                sensors[i].active    = 0;
                sensors[i].seq       = 0;
                sensors[i].stop_flag = 0;
                pthread_mutex_init(&sensors[i].mutex, NULL);
        }
}

Sensor *find_sensor(const char *name) {
        for (int i = 0; i < NUM_SENSORS; i++)
                if (strcmp(sensors[i].name, name) == 0)
                        return &sensors[i];
        return NULL;
}

void *stream_thread(void *arg) {
        Sensor *s = (Sensor *)arg;
        char buf[256];
        srand((unsigned)time(NULL) ^ (unsigned)(size_t)arg);

        while (1) {
                pthread_mutex_lock(&s->mutex);
                if (s->stop_flag) {
                        pthread_mutex_unlock(&s->mutex);
                        break;
                }

                /* random walk */
                double delta = ((rand() % 201) - 100) / 200.0;
                s->value += delta;
                if (s->value < s->min_val) s->value = s->min_val;
                if (s->value > s->max_val) s->value = s->max_val;

                s->seq++;
                int seq       = s->seq;
                double val    = s->value;
                time_t ts     = time(NULL);
                int udp_sock  = s->udp_sock;
                struct sockaddr_in addr = s->client_addr;
                pthread_mutex_unlock(&s->mutex);

                snprintf(buf, sizeof(buf),
                        "SEQ:%d|SENSOR:%s|VALUE:%.2f|UNIT:%s|TS:%ld",
                        seq, s->name, val, s->unit, (long)ts);

                sendto(udp_sock, buf, strlen(buf), 0,
                        (struct sockaddr *)&addr, sizeof(addr));

                usleep(s->interval_ms * 1000);
        }

        return NULL;
}

typedef struct {
        char method[16];
        char resource[64];
        char hkeys[8][32];
        char hvals[8][64];
        int  nheaders;
} Request;

int parse_request(const char *raw, Request *req) {
        char buf[MAX_BUF];
        strncpy(buf, raw, sizeof(buf) - 1);

        /* first line: METHOD resource */
        char *line = strtok(buf, "\r\n");
        if (!line) return -1;
        if (sscanf(line, "%15s %63s", req->method, req->resource) != 2)
                return -1;

        req->nheaders = 0;
        while ((line = strtok(NULL, "\r\n")) != NULL) {
                if (strlen(line) == 0) break;
                char *colon = strchr(line, ':');
                if (!colon) continue;
                *colon = '\0';
                char *key = line;
                char *val = colon + 1;
                while (*val == ' ') val++;
                if (req->nheaders < 8) {
                        strncpy(req->hkeys[req->nheaders], key, 31);
                        strncpy(req->hvals[req->nheaders], val, 63);
                        req->nheaders++;
                }
        }
        return 0;
}

const char *get_header(const Request *req, const char *key) {
        for (int i = 0; i < req->nheaders; i++)
                if (strcasecmp(req->hkeys[i], key) == 0)
                        return req->hvals[i];
        return NULL;
}

void send_response(int fd, const char *resp) {
        send(fd, resp, strlen(resp), 0);
}

/* ---------- command handlers ---------- */

void handle_start(int client_fd, const Request *req,
                  const struct sockaddr_in *client_ip, int udp_sock) {
        /* resource: /sensor/<tipo> */
        const char *slash = strrchr(req->resource, '/');
        if (!slash || *(slash + 1) == '\0') {
                send_response(client_fd,
                        "400 Bad Request\r\nError: recurso invalido\r\n\r\n");
                return;
        }
        const char *tipo = slash + 1;

        Sensor *s = find_sensor(tipo);
        if (!s) {
                char resp[256];
                snprintf(resp, sizeof(resp),
                        "404 Not Found\r\nError: sensor '%s' nao existe\r\n\r\n", tipo);
                send_response(client_fd, resp);
                return;
        }

        const char *port_str = get_header(req, "UdpPort");
        if (!port_str) {
                send_response(client_fd,
                        "400 Bad Request\r\nError: UdpPort ausente\r\n\r\n");
                return;
        }
        int udp_port = atoi(port_str);

        pthread_mutex_lock(&s->mutex);
        if (s->active) {
                pthread_mutex_unlock(&s->mutex);
                send_response(client_fd,
                        "409 Conflict\r\nError: sensor ja esta ativo\r\n\r\n");
                return;
        }

        s->udp_sock = udp_sock;
        s->client_addr = *client_ip;
        s->client_addr.sin_port = htons(udp_port);
        s->stop_flag = 0;
        s->active    = 1;
        s->seq       = 0;
        pthread_mutex_unlock(&s->mutex);

        pthread_create(&s->thread, NULL, stream_thread, s);

        char resp[256];
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_ip->sin_addr, ip_str, sizeof(ip_str));
        snprintf(resp, sizeof(resp),
                "200 OK\r\nSensor: %s\r\nStatus: streaming\r\n"
                "UdpTarget: %s:%d\r\nInterval: %dms\r\n\r\n",
                s->name, ip_str, udp_port, s->interval_ms);
        send_response(client_fd, resp);

        printf("[LOG] Fluxo %s INICIADO (intervalo: %dms)\n",
               s->name, s->interval_ms);
}

void handle_stop(int client_fd, const Request *req) {
        const char *slash = strrchr(req->resource, '/');
        if (!slash || *(slash + 1) == '\0') {
                send_response(client_fd,
                        "400 Bad Request\r\nError: recurso invalido\r\n\r\n");
                return;
        }
        const char *tipo = slash + 1;

        Sensor *s = find_sensor(tipo);
        if (!s) {
                char resp[256];
                snprintf(resp, sizeof(resp),
                        "404 Not Found\r\nError: sensor '%s' nao existe\r\n\r\n", tipo);
                send_response(client_fd, resp);
                return;
        }

        pthread_mutex_lock(&s->mutex);
        if (!s->active) {
                pthread_mutex_unlock(&s->mutex);
                send_response(client_fd,
                        "409 Conflict\r\nError: sensor ja esta inativo\r\n\r\n");
                return;
        }
        int total_seq = s->seq;
        s->stop_flag  = 1;
        s->active     = 0;
        pthread_mutex_unlock(&s->mutex);

        pthread_join(s->thread, NULL);

        char resp[128];
        snprintf(resp, sizeof(resp),
                "200 OK\r\nSensor: %s\r\nStatus: stopped\r\n\r\n", s->name);
        send_response(client_fd, resp);

        printf("[LOG] Fluxo %s PARADO (total enviados: %d pacotes)\n",
               s->name, total_seq);
}

void handle_status(int client_fd, const Request *req) {
        if (strcmp(req->resource, "/sensors") == 0) {
                char body[1024] = "";
                for (int i = 0; i < NUM_SENSORS; i++) {
                        char line[64];
                        pthread_mutex_lock(&sensors[i].mutex);
                        if (sensors[i].active)
                                snprintf(line, sizeof(line),
                                        "%s: streaming (seq=%d)\r\n",
                                        sensors[i].name, sensors[i].seq);
                        else
                                snprintf(line, sizeof(line),
                                        "%s: inactive\r\n", sensors[i].name);
                        pthread_mutex_unlock(&sensors[i].mutex);
                        strncat(body, line, sizeof(body) - strlen(body) - 1);
                }
                char resp[1200];
                snprintf(resp, sizeof(resp),
                        "200 OK\r\nCount: %d\r\n\r\n%s", NUM_SENSORS, body);
                send_response(client_fd, resp);
                return;
        }

        /* /sensor/<tipo> */
        const char *slash = strrchr(req->resource, '/');
        if (!slash || *(slash + 1) == '\0') {
                send_response(client_fd,
                        "400 Bad Request\r\nError: recurso invalido\r\n\r\n");
                return;
        }
        const char *tipo = slash + 1;
        Sensor *s = find_sensor(tipo);
        if (!s) {
                char resp[256];
                snprintf(resp, sizeof(resp),
                        "404 Not Found\r\nError: sensor '%s' nao existe\r\n\r\n", tipo);
                send_response(client_fd, resp);
                return;
        }

        pthread_mutex_lock(&s->mutex);
        char resp[256];
        snprintf(resp, sizeof(resp),
                "200 OK\r\nSensor: %s\r\nStatus: %s\r\nSeq: %d\r\n"
                "Value: %.2f\r\nUnit: %s\r\n\r\n",
                s->name,
                s->active ? "streaming" : "inactive",
                s->seq, s->value, s->unit);
        pthread_mutex_unlock(&s->mutex);
        send_response(client_fd, resp);
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
        int tcp_port = (argc >= 2) ? atoi(argv[1]) : TCP_PORT_DEFAULT;

        srand((unsigned)time(NULL));
        init_sensors();

        /* TCP socket */
        int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_sock < 0) { perror("socket TCP"); exit(1); }

        int opt = 1;
        setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in srv_addr = {0};
        srv_addr.sin_family      = AF_INET;
        srv_addr.sin_addr.s_addr = INADDR_ANY;
        srv_addr.sin_port        = htons(tcp_port);

        if (bind(tcp_sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
                perror("bind TCP"); exit(1);
        }
        listen(tcp_sock, 5);
        printf("[LOG] Servidor TCP escutando na porta %d...\n", tcp_port);

        /* UDP socket (para enviar dados) */
        int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_sock < 0) { perror("socket UDP"); exit(1); }

        /* accept loop */
        while (1) {
                struct sockaddr_in cli_addr = {0};
                socklen_t cli_len = sizeof(cli_addr);
                int cli_fd = accept(tcp_sock,
                        (struct sockaddr *)&cli_addr, &cli_len);
                if (cli_fd < 0) { perror("accept"); continue; }

                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cli_addr.sin_addr, ip_str, sizeof(ip_str));
                printf("[LOG] Cliente conectado: %s\n", ip_str);

                /* command loop */
                char raw[MAX_BUF];
                int  raw_len = 0;

                while (1) {
                        char tmp[512];
                        int n = recv(cli_fd, tmp, sizeof(tmp) - 1, 0);
                        if (n <= 0) break;
                        tmp[n] = '\0';

                        /* accumulate until \r\n\r\n */
                        if (raw_len + n < (int)sizeof(raw) - 1) {
                                memcpy(raw + raw_len, tmp, n);
                                raw_len += n;
                                raw[raw_len] = '\0';
                        }

                        if (strstr(raw, "\r\n\r\n") == NULL) continue;

                        Request req = {0};
                        if (parse_request(raw, &req) < 0) {
                                send_response(cli_fd,
                                        "400 Bad Request\r\nError: parse falhou\r\n\r\n");
                                raw_len = 0;
                                continue;
                        }
                        raw_len = 0;

                        printf("[LOG] Comando: %s %s\n", req.method, req.resource);

                        if (strcmp(req.method, "START") == 0) {
                                handle_start(cli_fd, &req, &cli_addr, udp_sock);
                        } else if (strcmp(req.method, "STOP") == 0) {
                                handle_stop(cli_fd, &req);
                        } else if (strcmp(req.method, "STATUS") == 0) {
                                handle_status(cli_fd, &req);
                        } else if (strcmp(req.method, "EXIT") == 0) {
                                send_response(cli_fd, "200 OK\r\nStatus: bye\r\n\r\n");
                                goto disconnect;
                        } else {
                                send_response(cli_fd,
                                        "400 Bad Request\r\nError: metodo desconhecido\r\n\r\n");
                        }
                }

disconnect:
                /* stop all active sensors for this client */
                for (int i = 0; i < NUM_SENSORS; i++) {
                        pthread_mutex_lock(&sensors[i].mutex);
                        if (sensors[i].active) {
                                sensors[i].stop_flag = 1;
                                sensors[i].active    = 0;
                                pthread_mutex_unlock(&sensors[i].mutex);
                                pthread_join(sensors[i].thread, NULL);
                        } else {
                                pthread_mutex_unlock(&sensors[i].mutex);
                        }
                }
                close(cli_fd);
                printf("[LOG] Cliente desconectado.\n");
        }

        close(tcp_sock);
        close(udp_sock);
        return 0;
}