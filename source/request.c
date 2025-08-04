#include "request.h"
#define MAX_QUEUE 10

static Request request_queue[MAX_QUEUE];
static int request_count = 0;
static char *sprite_memory = NULL;
static size_t sprite_memory_size = 0;
static LightLock request_lock;
static bool request_running = true;
static Thread request_thread;
static LightEvent request_event;

bool youfuckedup = false;
bool czasfuckup = false;
bool requestdone = false;
bool need_to_load_image = false;
bool loadingshit = false;
long response_code = 0;

LightLock global_response_lock;

C2D_SpriteSheet kuponobraz;
C2D_Image kuponkurwa;
bool obrazekdone = false;

static inline u32 next_pow2(u32 n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}
static inline u32 clamp(u32 n, u32 lower, u32 upper) {
    return n < lower ? lower : (n > upper ? upper : n);
}
static inline u32 rgba_to_abgr(u32 px) {
    u8 r = px & 0xff;
    u8 g = (px >> 8) & 0xff;
    u8 b = (px >> 16) & 0xff;
    u8 a = (px >> 24) & 0xff;
    return (a << 24) | (b << 16) | (g << 8) | r;
}

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    if (!ptr || !userdata) {
        log_to_file("[write_callback] ERROR: NULL input");
        return 0;
    }

    size_t total_size = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userdata;

    LightLock_Lock(&global_response_lock);
    char *new_data = realloc(buf->data, buf->size + total_size + 1);
    if (!new_data) {
        log_to_file("[write_callback] ERROR: realloc failed");
        LightLock_Unlock(&global_response_lock);
        return 0;
    }

    buf->data = new_data;
    memcpy(buf->data + buf->size, ptr, total_size);
    buf->size += total_size;
    buf->data[buf->size] = '\0';
    LightLock_Unlock(&global_response_lock);

    return total_size;
}

void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void log_request_to_file(const char *url, const char *data, struct curl_slist *headers, char *response) {
    FILE *log_file = fopen("request_log.txt", "a");
    if (log_file) {
        fprintf(log_file, "URL: %s\n", url);
        fprintf(log_file, "Data: %s\n", data ? data : "(None)");
        fprintf(log_file, "Headers:\n");
        struct curl_slist *header = headers;
        while (header) {
            fprintf(log_file, "  %s\n", header->data);
            header = header->next;
        }
        fprintf(log_file, "Response: %s\n", response ? response : "(None)");
        fprintf(log_file, "------------------------\n");
        fclose(log_file);
    } else {
        printf("Failed to open log file for writing.\n");
    }
}

int my_curl_debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr) {
    switch (type) {
        case CURLINFO_TEXT:
        case CURLINFO_HEADER_IN:
        case CURLINFO_HEADER_OUT:
        case CURLINFO_DATA_IN:
        case CURLINFO_DATA_OUT:
            log_to_file("[curl_debug] %.*s", (int)size, data);
            break;
        default:
            break;
    }
    return 0;
}

bool refresh_data(const char *url, const char *data, struct curl_slist *headers, ResponseBuffer *response_buf) {
    bool request_failed = false;
    youfuckedup = false;
    czasfuckup = false;
    requestdone = false;
    loadingshit = true;

    if (!url || url[0] == '\0' || !response_buf) {
        log_to_file("[refresh_data] ERROR: Missing URL or response buffer.");
        return true;
    }

    log_to_file("[refresh_data] --- HTTP REQUEST BEGIN ---");
    log_to_file("[refresh_data] URL: %s", url);

    struct curl_slist *header_iter = headers;
    while (header_iter) {
        log_to_file("  %s", header_iter->data);
        header_iter = header_iter->next;
    }

    if (data && strcmp(url, "https://zabka-snrs.zabka.pl/v4/server/time") != 0) {
        log_to_file("[refresh_data] Body:\n%s", data);
    } else {
        log_to_file("[refresh_data] Body: (none or skipped)");
    }

    if (response_buf->data) {
        free(response_buf->data);
        response_buf->data = NULL;
        response_buf->size = 0;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        log_to_file("[refresh_data] ERROR: curl_easy_init() failed.");
        return true;
    }

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_curl_debug_callback);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "romfs:/cacert.pem");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if (data && strcmp(url, "https://api.szprink.xyz/t3x/convert") == 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    }


    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_buf);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code == 401) youfuckedup = true;
    if (response_code == 0) czasfuckup = true;

    if (res != CURLE_OK) {
        log_to_file("[refresh_data] ERROR: %s", curl_easy_strerror(res));
        request_failed = true;
    } else {
        log_to_file("[refresh_data] Success. Code: %ld", response_code);
    }

    curl_easy_cleanup(curl);
    loadingshit = false;
    requestdone = true;

	if ((strstr(url, ".png") || strstr(url, ".jpeg")) &&
		response_buf->data && response_buf->size > 0) {
		need_to_load_image = true;
	}


    log_to_file("[refresh_data] Request Done");
    return request_failed;
}

void request_worker(void* arg) {
    ResponseBuffer local_buf = {NULL, 0, false};  // declare once

    while (request_running) {
        LightLock_Lock(&request_lock);
        if (request_count == 0) {
            LightLock_Unlock(&request_lock);
            LightEvent_Wait(&request_event);
            continue;
        }

        Request req = request_queue[0];
        memmove(request_queue, request_queue + 1, (request_count - 1) * sizeof(Request));
        request_count--;
        LightLock_Unlock(&request_lock);

        // Reset local_buf each iteration
        local_buf.data = NULL;
        local_buf.size = 0;
        local_buf.done = false;

        ResponseBuffer *buf = req.response_buf;
        if (!buf) {
            buf = &local_buf;
        }

        if (req.url[0] == '\0') {
            log_to_file("[request_worker] ERROR: request has no URL");
            continue;
        }

        refresh_data(req.url, req.data, req.headers, buf);

		if (req.response && req.response_size && buf->data) {
			*req.response = buf->data;
			*req.response_size = buf->size;
		} else if (!req.response_buf && buf->data) {
			// Only free it if this was a temporary buffer, not owned externally
			free(buf->data);
		}


        if (req.owns_data && req.data) free(req.data);
        if (req.headers) curl_slist_free_all(req.headers);

		if (requestdone && need_to_load_image && buf->data && buf->size > 0) {
			unsigned char* decoded = NULL;
			unsigned width = 0, height = 0;

			unsigned error = lodepng_decode32(&decoded, &width, &height,
											  (const unsigned char*)buf->data, buf->size);
											  
			if (error) {
				log_to_file("PNG decode failed: %s", lodepng_error_text(error));

				// Try JPEG decode
				struct jpeg_decompress_struct cinfo;
				struct my_error_mgr jerr;

				JSAMPARRAY buffer;  // Output row buffer
				int row_stride;

				cinfo.err = jpeg_std_error(&jerr.pub);
				jerr.pub.error_exit = my_error_exit;

				if (setjmp(jerr.setjmp_buffer)) {
					// libjpeg error
					jpeg_destroy_decompress(&cinfo);
					log_to_file("JPEG decode also failed.");
					need_to_load_image = false;
					continue;
				}

				jpeg_create_decompress(&cinfo);
				jpeg_mem_src(&cinfo, (unsigned char*)buf->data, buf->size);
				jpeg_read_header(&cinfo, TRUE);
				jpeg_start_decompress(&cinfo);

				width = cinfo.output_width;
				height = cinfo.output_height;
				int channels = cinfo.output_components;

				// JPEG is usually RGB, so channels should be 3
				if (channels != 3) {
					log_to_file("Unexpected JPEG channel count: %d", channels);
					jpeg_finish_decompress(&cinfo);
					jpeg_destroy_decompress(&cinfo);
					need_to_load_image = false;
					continue;
				}

				row_stride = width * channels;

				unsigned char* rgb = (unsigned char*)malloc(width * height * channels);
				if (!rgb) {
					log_to_file("Failed to allocate temporary RGB buffer.");
					jpeg_finish_decompress(&cinfo);
					jpeg_destroy_decompress(&cinfo);
					need_to_load_image = false;
					continue;
				}

				buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
				unsigned char* p = rgb;

				while (cinfo.output_scanline < height) {
					jpeg_read_scanlines(&cinfo, buffer, 1);
					memcpy(p, buffer[0], row_stride);
					p += row_stride;
				}

				jpeg_finish_decompress(&cinfo);
				jpeg_destroy_decompress(&cinfo);

				// Convert RGB to RGBA
				decoded = (unsigned char*)malloc(width * height * 4);
				if (!decoded) {
					free(rgb);
					log_to_file("Failed to allocate RGBA buffer.");
					need_to_load_image = false;
					continue;
				}

				for (unsigned y = 0; y < height; ++y) {
					for (unsigned x = 0; x < width; ++x) {
						size_t src_i = (y * width + x) * 3;
						size_t dst_i = (y * width + x) * 4;
						decoded[dst_i + 0] = rgb[src_i + 0];  // R
						decoded[dst_i + 1] = rgb[src_i + 1];  // G
						decoded[dst_i + 2] = rgb[src_i + 2];  // B
						decoded[dst_i + 3] = 255;             // A
					}
				}

				free(rgb);
			}


			unsigned orig_w = width;
			unsigned orig_h = height;

			// For 90° rotation, swap width and height for texture size:
			u32 tex_w = clamp(next_pow2(orig_h), 64, 1024);
			u32 tex_h = clamp(next_pow2(orig_w), 64, 1024);

			C3D_Tex* tex = malloc(sizeof(C3D_Tex));
			if (!tex || !C3D_TexInit(tex, tex_w, tex_h, GPU_RGBA8)) {
				log_to_file("Texture creation failed");
				free(decoded);
				if (tex) free(tex);
				need_to_load_image = false;
				continue;
			}

			C3D_TexSetFilter(tex, GPU_LINEAR, GPU_NEAREST);
			memset(tex->data, 0, tex_w * tex_h * 4);

			
			for (u32 y = 0; y < orig_h; ++y) {
				for (u32 x = 0; x < orig_w; ++x) {
					u32 src_i = (y * orig_w + x) * 4;
					u8 r = decoded[src_i];
					u8 g = decoded[src_i + 1];
					u8 b = decoded[src_i + 2];
					u8 a = decoded[src_i + 3];

					// Pack RGBA and convert to ABGR with function (if you have it)
					u32 rgba = (r << 24) | (g << 16) | (b << 8) | a;
					u32 abgr = rgba_to_abgr(rgba);

					// Rotate 90° right, no horizontal flip
					u32 new_x = y;
					u32 new_y = x;

					u32 dst_offset = (((new_x >> 3) * (tex_w >> 3) + (new_y >> 3)) << 6) +
									 ((new_y & 1) | ((new_x & 1) << 1) | ((new_y & 2) << 1) |
									  ((new_x & 2) << 2) | ((new_y & 4) << 2) | ((new_x & 4) << 3));

					((u32*)tex->data)[dst_offset] = abgr;
				}
			}


			Tex3DS_SubTexture* subtex = malloc(sizeof(Tex3DS_SubTexture));
			if (!subtex) {
				C3D_TexDelete(tex);
				free(tex);
				free(decoded);
				log_to_file("Failed to allocate subtexture");
				need_to_load_image = false;
				continue;
			}

			subtex->width = orig_h;    // swapped width/height
			subtex->height = orig_w;
			subtex->left = 0.0f;
			subtex->top = 1.0f;
			subtex->right = (float)orig_h / tex_w;   // width swapped
			subtex->bottom = 1.0f - ((float)orig_w / tex_h);

			kuponkurwa.tex = tex;
			kuponkurwa.subtex = subtex;

			obrazekdone = true;
			need_to_load_image = false;

			log_to_file("PNG decoded and image loaded successfully.");
			free(decoded);
		}



        if (req.response_buf) {
            req.response_buf->done = true;
        }
    }
}


void queue_request(const char *url, const char *data, struct curl_slist *headers,
                   ResponseBuffer *response_buf, bool is_binary) {
    if (!url || url[0] == '\0' || !response_buf) {
        log_to_file("[queue_request] ERROR: Missing critical args");
        return;
    }

    LightLock_Lock(&request_lock);

    if (request_count >= MAX_QUEUE) {
        log_to_file("[queue_request] ERROR: Request queue full");
        LightLock_Unlock(&request_lock);
        return;
    }

    Request *req = &request_queue[request_count];
    memset(req, 0, sizeof(Request));

    strncpy(req->url, url, sizeof(req->url) - 1);
    req->url[sizeof(req->url) - 1] = '\0';

    if (data) {
        req->data = strdup(data);
        if (!req->data) {
            log_to_file("[queue_request] ERROR: strdup(data) failed");
            LightLock_Unlock(&request_lock);
            return;
        }
        req->owns_data = true;
    }

    req->headers = headers;
    req->response_buf = response_buf;
    req->is_binary = is_binary;

    request_count++;
    LightEvent_Signal(&request_event);
    LightLock_Unlock(&request_lock);
}


void start_request_thread() {
    LightLock_Init(&request_lock);
    LightLock_Init(&global_response_lock);
    LightEvent_Init(&request_event, RESET_ONESHOT);
    request_thread = threadCreate(request_worker, NULL, 32 * 1024, 0x30, -2, false);
}

void stop_request_thread() {
    LightLock_Lock(&request_lock);
    request_running = false;
    LightEvent_Signal(&request_event);
    LightLock_Unlock(&request_lock);

    threadJoin(request_thread, UINT64_MAX);
    threadFree(request_thread);
}
