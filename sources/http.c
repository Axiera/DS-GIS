#include "../headers/http.h"

typedef struct {
    size_t content_total_length;
    size_t content_length;
    const char* content;
} http_response_t;

static SOCKET init_socket(const char* hostname);
static void deinit_socket(SOCKET sock);
static char* make_request(const char* hostname, const char* path);
static int http_send(SOCKET sock, const char* request);
static size_t http_receive(SOCKET sock, void* buffer, size_t buffer_size);
static http_response_t parse_response(const char* response, size_t size);

/* ---------------------- header functions definition ---------------------- */

int http_init(void) {
    WORD version = MAKEWORD(2, 2);
    int error = WSAStartup(version, &(WSADATA){});
    if (error)
        SDL_SetError("windows socket initialization failed");
    return error;
}

void http_deinit(void) {
    WSACleanup();
}

response_t http_get(const char* hostname, const char* path) {
    response_t response = { 0, NULL };

    SOCKET sock = init_socket(hostname);
    if (sock == INVALID_SOCKET)
        return response;

    char* request = make_request(hostname, path);
    if (request == NULL || http_send(sock, request)) {
        deinit_socket(sock);
        free(request);
        return response;
    }
    free(request);

    const size_t RESPONSE_BUF_SIZE = 4*1024;
    char response_buf[RESPONSE_BUF_SIZE];
    int response_size = http_receive(sock, response_buf, RESPONSE_BUF_SIZE);
    if (!response_size) {
        deinit_socket(sock);
        return response;
    }

    http_response_t http_response = parse_response(response_buf, response_size);

    response.data = malloc(http_response.content_total_length);
    if (response.data == NULL) {
        deinit_socket(sock);
        return response;
    }
    response.size = http_response.content_length;
    memcpy(response.data, http_response.content, http_response.content_length);

    while (response.size < http_response.content_total_length) {
        response_size = http_receive(sock, response_buf, RESPONSE_BUF_SIZE);
        if (!response_size) {
            deinit_socket(sock);
            return response;
        }
        memcpy(response.data + response.size, response_buf, response_size);
        response.size += response_size;
    }

    deinit_socket(sock);
    return response;
}

/* ---------------------- static functions definition ---------------------- */

static SOCKET init_socket(const char* hostname) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
        return INVALID_SOCKET;

    struct hostent* host = gethostbyname(hostname);
    if (host == NULL)
        return INVALID_SOCKET;

    struct sockaddr_in host_address = {
        .sin_family         = AF_INET,
        .sin_port           = htons(80),
        .sin_addr.s_addr    = *(ULONG*)host->h_addr
    };

    if (connect(sock, (struct sockaddr*)&host_address, sizeof(host_address)))
        return INVALID_SOCKET;

    return sock;
}

static void deinit_socket(SOCKET sock) {
    closesocket(sock);
}

static char* make_request(const char* hostname, const char* path) {
    static const char* BASE = "GET  HTTP/1.1\r\nHost: \r\n\r\n";
    size_t request_length = strlen(BASE) + strlen(hostname) + strlen(path) + 1;

    char* request = malloc(request_length * sizeof(char));
    if (request == NULL)
        return NULL;
    request[0] = '\0';

    const size_t COMPONENT_COUNT = 5;
    const char* COMPONENTS[] =
        {"GET ", path, " HTTP/1.1\r\nHost: ", hostname, "\r\n\r\n"};
    for (int i = 0; i < COMPONENT_COUNT; i++)
        strcat(request, COMPONENTS[i]);

    return request;
}

static int http_send(SOCKET sock, const char* request) {
    return SOCKET_ERROR == send(sock, request, strlen(request), 0);
}

static size_t http_receive(SOCKET sock, void* buffer, size_t buffer_size) {
    int response_size = recv(sock, buffer, buffer_size, 0);
    if (response_size == 0 || response_size == SOCKET_ERROR)
        return 0;
    return response_size;
}

static http_response_t parse_response(const char* response, size_t size) {
    http_response_t http_response = { 0, 0, response };
    const char* end = response + size;

    if (strncmp(response, "HTTP", 4)) {
        http_response.content_total_length = size;
        http_response.content_length = size;
        return http_response;
    }

    for (; http_response.content <= end-4; http_response.content++) {
        if (strncmp(http_response.content, "\r\n\r\n", 4))
            continue;
        http_response.content += 4;
        http_response.content_length = end - http_response.content;
        break;
    }

    http_response.content_total_length = http_response.content_length;
    for (int i = 0; i < size - http_response.content_length - 1; i++) {
        if (strncmp(response+i, "\r\n", 2))
            continue;
        i += 2;

        int parameter_length = 0;
        for (int j = i; *(response+j) != '\r'; j++)
            parameter_length++;

        char* parameter = malloc(parameter_length + 1);
        if (parameter == NULL)
            return http_response;
        for (int j = 0; j < parameter_length; j++)
            parameter[j] = tolower(*(response+i+j));
        parameter[parameter_length] = '\0';

        const char* key = "content-length:";
        if (!strncmp(parameter, key, strlen(key))) {
            http_response.content_total_length = atoi(parameter + strlen(key));
            free(parameter);
            break;
        }

        free(parameter);
    }

    return http_response;
}
