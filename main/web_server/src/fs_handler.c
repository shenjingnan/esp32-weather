/*
 * Static file handler - serves files from LittleFS VFS
 */
#include <esp_http_server.h>
#include <esp_log.h>
#include <sys/stat.h>
#include <fcntl.h>

static const char *TAG = "fs_handler";

/* MIME type mapping for common file extensions */
static const char *get_content_type(const char *path)
{
    if (strstr(path, ".html") || strstr(path, ".htm")) return "text/html";
    if (strstr(path, ".css"))  return "text/css";
    if (strstr(path, ".js"))   return "application/javascript";
    if (strstr(path, ".json")) return "application/json";
    if (strstr(path, ".png"))  return "image/png";
    if (strstr(path, ".jpg") || strstr(path, ".jpeg")) return "image/jpeg";
    if (strstr(path, ".svg"))  return "image/svg+xml";
    if (strstr(path, ".ico"))  return "image/x-icon";
    if (strstr(path, ".woff")) return "font/woff";
    if (strstr(path, ".woff2"))return "font/woff2";
    return "application/octet-stream";
}

esp_err_t fs_static_get_handler(httpd_req_t *req)
{
    const char *mount_point = (const char *)req->user_ctx;
    char filepath[256];

    /* Construct full file path from URI */
    if (strcmp(req->uri, "/") == 0) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", mount_point);
    } else {
        snprintf(filepath, sizeof(filepath), "%s%s", mount_point, req->uri);
    }

    struct stat st;
    if (stat(filepath, &st) != 0) {
        /* SPA fallback: serve index.html for unknown routes */
        snprintf(filepath, sizeof(filepath), "%s/index.html", mount_point);
        if (stat(filepath, &st) != 0) {
            ESP_LOGW(TAG, "File not found: %s (%s)", req->uri, filepath);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
    }

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    /* Set Content-Type header */
    httpd_resp_set_type(req, get_content_type(filepath));

    /* Enable caching with ETag-like approach using file size + mtime */
    char cache_header[64];
    snprintf(cache_header, sizeof(cache_header),
             "max-age=3600, public");
    httpd_resp_set_hdr(req, "Cache-Control", cache_header);

    /* Stream file content in chunks */
    char buf[1024];
    ssize_t read_bytes;
    while ((read_bytes = read(fd, buf, sizeof(buf))) > 0) {
        if (httpd_resp_send_chunk(req, buf, read_bytes) != ESP_OK) {
            close(fd);
            return ESP_FAIL;
        }
    }
    close(fd);

    /* Send empty chunk to finalize response */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}
