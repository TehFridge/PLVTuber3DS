#ifndef REQUEST_H
#define REQUEST_H

#include <3ds.h>
#include <citro2d.h>
#include <curl/curl.h>
#include "logs.h"
#include "lodepng.h"
#include <jpeglib.h>
#include <setjmp.h>

// Error handler for libjpeg
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

METHODDEF(void) my_error_exit(j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

typedef struct {
    char *data;
    size_t size;
    volatile bool done; // ✅ Signals completion
} ResponseBuffer;

typedef struct {
    char url[512];
    char *data;
    struct curl_slist *headers;
    bool owns_data;
    void **response;
    size_t *response_size;
    bool is_binary;
    ResponseBuffer *response_buf; // Must be set before queuing
} Request;


// Image handling (used externally)
extern C2D_SpriteSheet kuponobraz;
extern C2D_Image kuponkurwa;
extern bool obrazekdone;

// Request system status
extern bool requestdone;
extern bool loadingshit;
extern bool czasfuckup;
extern long response_code;
extern LightLock global_response_lock;

// Core functions
void start_request_thread();
void stop_request_thread();
void queue_request(const char *url, const char *data, struct curl_slist *headers,
                   ResponseBuffer *response_buf, bool is_binary);

void request_worker(void* arg);
bool refresh_data(const char *url, const char *data, struct curl_slist *headers, ResponseBuffer *buf);
void load_image();
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);
void log_request_to_file(const char *url, const char *data, struct curl_slist *headers, char *response);
void log_message(const char *format, ...);

#endif // REQUEST_H
