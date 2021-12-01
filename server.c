#include "server.h"

size_t port = 8080;
const char* PUBLIC_PATH = "./public";

void create_socket()
{
        // 0 for TCP connection
        connFd.server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (connFd.server_fd < 0) {
                perror("create socket");
        }
}

void init_socket()
{
        bzero(&server_sock, sizeof(server_sock));
        bzero(&client_sock, sizeof(client_sock));

        server_sock.sin_family = PF_INET;
        server_sock.sin_addr.s_addr = INADDR_ANY;
        server_sock.sin_port = htons(port);
        int on = 1;
        setsockopt(connFd.server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

        if (bind(connFd.server_fd, (struct sockaddr *)&server_sock, sizeof(server_sock)) < 0) {
                perror("bind");
        }

        if (listen(connFd.server_fd, MAX_CONNECTION) < 0) {
                perror("listen");
        } else {
            printf("===========================================================\n");
            printf("*                                                         *\n");
            printf("*    Simple HTTP Server now listening on port %ld...     *\n", port);
            printf("*                                                         *\n");
            printf("* Please place the static assets in '$CODE_FOLDER/public' *\n");
            printf("*                                                         *\n");
            printf("===========================================================\n\n");
        }
}

void server_routine()
{
        int len = sizeof(client_sock);
        int client_fd;

        threadpool_t *threadpool = create_threadpool(THREAD_MAX_SIZE);

        while (client_fd = accept(connFd.server_fd, (struct sockaddr *)&client_sock, &len)) {
                size_t *fd = malloc(sizeof(size_t));
                *fd = client_fd;
                threadpool_task_t *task = malloc(sizeof(threadpool_task_t));
                task->function = http_response_handler;
                task->args = (void *) fd;
                threadpool_add_tasks(threadpool, task);
        }
}

void parse_request(struct request_header *req, char *inputBuffer)
{
        int order = 1, idx = 0, f_type_dx = 0, start_file = 0;
        for (int i = 0; i < strlen(inputBuffer); i++) {
                if (inputBuffer[i] == '\n') {
                        break;
                } else if (inputBuffer[i] == ' ') {
                        if (order == 1) 
                                req->http_method[idx++] = '\0';
                        else if (order == 2)
                                req->url[idx++] = '\0';
                        order += 1;
                        idx = 0;
                } else {
                        if (order == 1)
                                req->http_method[idx++] = inputBuffer[i];
                        else if (order == 2) {
                                req->url[idx++] = inputBuffer[i];
                                if (start_file)
                                        req->req_file_type[f_type_dx++] = inputBuffer[i];
                        }       
                }
                if (inputBuffer[i] == '.')
                        start_file = 1;
        }
        if (f_type_dx == 0)
                req->req_file_type[f_type_dx++] = '\0';
        if (strcmp(req->url, "/") == 0) 
                strcpy(req->url, "/index.html");
}

void http_response_handler(void *void_client_fd)
{
        size_t *client_fd = (size_t *)void_client_fd;   // 轉型

        char *request_header = malloc(sizeof(char) * MAX_BUFFER_SIZE);     // 如何處理更大的req header
        char *response_header;  // http response header(100 byte is enough)
        struct request_header req;      // save request information

        /* 接收http request */
        recv(*client_fd, request_header, MAX_BUFFER_SIZE, 0);
        parse_request(&req, request_header);

        /* 處理及設定filepath */
        char *filepath = malloc(sizeof(char) * (strlen(PUBLIC_PATH) + strlen(req.url) + 1));
        strcpy(filepath, PUBLIC_PATH);
        strcat(filepath, req.url);
        req.filepath = filepath;

        printf("thread %ld is now handling %s from fd %ld\n", pthread_self(), req.filepath, *(client_fd));

        /* 產生http response header */
        response_header = create_response_header(&req);

        /* Server response to client */
        write_http_header(*client_fd, response_header);
        write_http_body(*client_fd, &req);

        close(*client_fd);
        
        /* free dynamically allocated memory */
        free(request_header);
        free(response_header);
        free(filepath);
}

void run_server()
{
        create_socket();
        init_socket();

        while (TRUE) {
                server_routine();
        }
}

char *create_response_header(struct request_header *req)
{
        char *response_header;
        // Response with html files
        if (strcmp(req->req_file_type, "\0") == 0 || strcmp(req->req_file_type, "html") == 0) {  
                response_header = strdup(HTML_HEADER);
        // Response with css files
        } else if (strcmp(req->req_file_type, "css") == 0) {
                response_header = strdup(CSS_HEADER);
        // Response with js files
        } else if (strcmp(req->req_file_type, "js") == 0) {
                response_header = strdup(JS_HEADER);
        // Response with png files
        } else if (strcmp(req->req_file_type, "png") == 0 || strcmp(req->req_file_type, "jpg") == 0) {
                response_header = malloc(sizeof(char) * (strlen(PNG_HEADER) + 40));
                strcpy(response_header, PNG_HEADER);
                struct stat filestat;
                stat(req->filepath, &filestat);

                char *num_str = malloc(sizeof(char) * 40);
                sprintf(num_str, "%ld", filestat.st_size);
                strcat(response_header, num_str);
                strcat(response_header, "\r\n\r\n");
        // Other files
        } else {
                response_header = strdup(DEFAULT_HEADER);
        }
        return response_header;
}

void write_http_header(size_t fd, char *buf)
{
        // 會不會像body出現問題?
        write(fd, buf, strlen(buf));
}

void write_http_body(size_t fd, struct request_header *req)
{
        // 處理圖片(只能回應logo.png，cause header content-length is a fixed value)
        if (strcmp(req->req_file_type, "png") == 0 || strcmp(req->req_file_type, "jpg") == 0) {
                // https://stackoverflow.com/questions/36393223/c-reading-bytes-from-binary-file
                int total_byte_write = 0;
                FILE *src = fopen(req->filepath, "rb");
                if (src <= 0) {
                        perror("fopen png");
                }

                int src_size;
                fseek(src, 0, SEEK_END);
                src_size = ftell(src);
                fseek(src, 0, SEEK_SET);
                unsigned char *img_buf = malloc(sizeof(unsigned char) * src_size);

                size_t bytes;
                // while ((bytes = fread(img_buf, sizeof(*img_buf), sizeof(img_buf), src)) == sizeof(img_buf)) {
                while (bytes = fread(img_buf, sizeof(*img_buf), sizeof(img_buf), src)) {
                        for (int i = 0; i < bytes; i++) {
                                write(fd, (void *) &img_buf[i], sizeof(img_buf[i]));
                                total_byte_write += sizeof(img_buf[i]);
                        }
                }
                if (src != stdin)
                        fclose(src);
                free(img_buf);
        // 處理其他檔案
        } else {
                /* write by read from file */
                FILE *fp = fopen(req->filepath, "r");
                if (fp == NULL) // error handling
                        return;
                int src_size;
                fseek(fp, 0, SEEK_END);
                src_size = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                
                char c;
                while ((c = fgetc(fp)) != EOF) {
                        write(fd, &c, 1);
                }
        }
}