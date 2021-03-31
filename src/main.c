#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "uv.h"
#include "duktape.h"
#include "llhttp.h"

#define CHECK(result) \
  do { \
    if (!(result)) { \
      fprintf(stderr, "Check failed at line: %d\n", __LINE__); \
      abort(); \
    } \
  } while (0)

#define CHECK_EQ(expected, actual) \
  do { \
    int res = (actual); \
    if (res != (expected)) { \
      fprintf(stderr, "Expected: %d, but got %d at line: %d\n", \
          (expected), res, __LINE__); \
      abort(); \
    } \
  } while (0)

/* Typedefs */

typedef struct bytecode_s bytecode_t;
struct bytecode_s {
  void* buffer;
  duk_size_t size;
};

typedef struct conn_s conn_t;
struct conn_s {
  uv_tcp_t tcp_client;
  char read_buf[1024];

  uv_buf_t url;
  uv_buf_t header_field;
  uv_buf_t header_value;

  llhttp_t http;

  duk_context* duk_ctx;
  duk_idx_t headers_obj;
};

/* Some static vars */

static const int BACKLOG = 511;
static const int FILE_READ_CHUNK_LEN = 4096;

static uv_loop_t loop;
static uv_tcp_t tcp_server;
static llhttp_settings_t http_settings;
static bytecode_t bytecode;

/* Callbacks */

static void conn_on_close(uv_handle_t* handle) {
  conn_t* conn = handle->data;

  handle->data = NULL;

  free(conn->url.base);
  conn->url = uv_buf_init(NULL, 0);
  free(conn->header_field.base);
  conn->header_field = uv_buf_init(NULL, 0);
  free(conn->header_value.base);
  conn->header_value = uv_buf_init(NULL, 0);

  if (conn->duk_ctx != NULL) {
    duk_destroy_heap(conn->duk_ctx);
    conn->duk_ctx = NULL;
  }

  free(conn);
}

static void conn_on_fatal_error(void* udata, const char* message) {
  conn_t* conn = udata;

  fprintf(stderr, "Runtime error: %s\n", message);

  uv_close((uv_handle_t*) &conn->tcp_client, conn_on_close);
}

static void conn_alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
  (void) size;

  conn_t* conn = handle->data;

  buf->base = conn->read_buf;
  buf->len = sizeof(conn->read_buf);
}

static void conn_read_cb(uv_stream_t* stream, ssize_t nread,
                         const uv_buf_t* buf) {
  conn_t* conn = stream->data;

  if (nread == UV_EOF) {
    uv_close((uv_handle_t*) stream, conn_on_close);
    return;
  }

  if (HPE_OK != llhttp_execute(&conn->http, buf->base, nread)) {
    fprintf(stderr, "parsing error: %s at pos: %d\n",
        llhttp_get_error_reason(&conn->http),
        (int) (llhttp_get_error_pos(&conn->http) - buf->base));

    uv_close((uv_handle_t*) stream, conn_on_close);
    return;
  }
}

static void conn_write_cb(uv_write_t* req, int status) {
  conn_t* conn = req->data;
  req->data = NULL;

  free(req);

  /* Error */
  if (status != 0) {
    /* TODO(indutny): I forgot if we should ignore this. I think we should? */
    if (status == UV_EPIPE) {
      return;
    }

    uv_close((uv_handle_t*) &conn->tcp_client, conn_on_close);
    return;
  }
}

static void on_connection(uv_stream_t* server, int status) {
  conn_t* conn;

  CHECK_EQ(0, status);

  conn = malloc(sizeof(*conn));
  CHECK(conn != NULL);

  memset(conn, 0, sizeof(*conn));

  /* Accept connection */
  CHECK_EQ(0, uv_tcp_init(&loop, &conn->tcp_client));
  conn->tcp_client.data = conn;

  CHECK_EQ(0, uv_accept(server, (uv_stream_t*) &conn->tcp_client));

  /* Initialize llhttp */
  llhttp_init(&conn->http, HTTP_REQUEST, &http_settings);
  conn->http.data = conn;

  /* Initialize duktape */
  conn->duk_ctx = duk_create_heap(NULL, NULL, NULL, conn, conn_on_fatal_error);
  CHECK(conn->duk_ctx != NULL);

  duk_push_external_buffer(conn->duk_ctx);
  duk_config_buffer(conn->duk_ctx, -1, bytecode.buffer, bytecode.size);
  duk_load_function(conn->duk_ctx);

  /* Start reading */
  CHECK_EQ(0, uv_read_start(
        (uv_stream_t*) &conn->tcp_client,
        conn_alloc_cb,
        conn_read_cb));
}

static int conn_on_message_begin(llhttp_t* http) {
  conn_t* conn = http->data;

  /* Duplicate function which should be on the stack */
  duk_dup(conn->duk_ctx, -1);

  /* Create headers object */
  conn->headers_obj = duk_push_object(conn->duk_ctx);

  return HPE_OK;
}

static void append_to_buffer(uv_buf_t* buf,
                             const char* data,
                             size_t len) {
  char* new_buffer = realloc(buf->base, buf->len + len);
  CHECK(new_buffer != NULL);

  memcpy(new_buffer + buf->len, data, len);
  buf->base = new_buffer;
  buf->len += len;
}

static int conn_on_url(llhttp_t* http, const char* p, size_t len) {
  conn_t* conn = http->data;

  append_to_buffer(&conn->url, p, len);

  return HPE_OK;
}

static void conn_add_headers(conn_t* conn) {
  if (conn->header_value.base == NULL) {
    return;
  }

  CHECK(conn->header_field.base != NULL);

  duk_push_lstring(conn->duk_ctx,
      conn->header_value.base, conn->header_value.len);
  duk_put_prop_lstring(conn->duk_ctx,
      conn->headers_obj,
      conn->header_field.base, conn->header_field.len);

  free(conn->header_field.base);
  free(conn->header_value.base);
  conn->header_field = uv_buf_init(NULL, 0);
  conn->header_value = uv_buf_init(NULL, 0);
}

static int conn_on_header_field(llhttp_t* http, const char* p, size_t len) {
  conn_t* conn = http->data;

  conn_add_headers(conn);

  append_to_buffer(&conn->header_field, p, len);

  return HPE_OK;
}

static int conn_on_header_value(llhttp_t* http, const char* p, size_t len) {
  conn_t* conn = http->data;

  append_to_buffer(&conn->header_value, p, len);

  return HPE_OK;
}

static int conn_on_message_complete(llhttp_t* http) {
  conn_t* conn = http->data;
  duk_context* ctx = conn->duk_ctx;

  CHECK(conn->url.base != NULL);

  conn_add_headers(conn);

  duk_push_lstring(ctx, conn->url.base, conn->url.len);
  duk_push_string(ctx, llhttp_method_name(http->method));

  duk_call(ctx, 3);

  /* The result of execution must be an object */
  duk_require_object(ctx, -1);

  /* Get res.code */
  duk_push_string(ctx, "code");
  duk_get_prop(ctx, -2);

  duk_int_t code = duk_require_int(ctx, -1);
  duk_pop(ctx);

  /* Get res.body */
  duk_push_string(ctx, "body");
  duk_get_prop(ctx, -2);

  duk_size_t body_len;
  const char* body = duk_require_lstring(ctx, -1, &body_len);
  CHECK(body != NULL);
  duk_pop(ctx);

  /* TODO(indutny): check body length? */

  /* Pop the result itself */
  duk_pop(ctx);

  int response_len = snprintf(NULL, 0,
      "HTTP/1.1 %d HTTP/1.1 WHATEVER\r\n"
      "Content-Length: %lld\r\n"
      "\r\n",
      code,
      (long long) body_len);

  uv_write_t* req;
  req = malloc(sizeof(*req) + response_len + body_len + 1);
  CHECK(req != NULL);

  char* response = ((char*) req) + sizeof(*req);

  CHECK_EQ(response_len, snprintf(response, response_len + 1,
      "HTTP/1.1 %d HTTP/1.1 WHATEVER\r\n"
      "Content-Length: %lld\r\n"
      "\r\n",
      code,
      (long long) body_len));

  memcpy(response + response_len, body, body_len);

  req->data = conn;

  uv_buf_t bufs[1] = { uv_buf_init(response, response_len + body_len) };

  CHECK_EQ(0, uv_write(
        req,
        (uv_stream_t*) &conn->tcp_client,
        bufs,
        1,
        conn_write_cb));

  /* Finally free request url */
  free(conn->url.base);
  conn->url = uv_buf_init(NULL, 0);

  return HPE_OK;
}

static void bytecode_on_fatal_error(void* udata, const char* message) {
  (void) udata;

  fprintf(stderr, "Compilation error: %s\n", message);
}

bytecode_t compile_bytecode(const char* filename) {
  duk_context* ctx;

  /* Read file */
  FILE* f = fopen(filename, "r");
  CHECK(f != NULL);

  int max_code_len = FILE_READ_CHUNK_LEN;
  char* code = malloc(max_code_len);
  CHECK(code != NULL);

  int code_len = 0;

  for (;;) {
    int chunk = fread(code + code_len, 1, max_code_len - code_len, f);
    if (chunk == 0) {
      CHECK_EQ(1, feof(f));
      break;
    }

    code_len += chunk;
    if (code_len == max_code_len) {
      max_code_len += FILE_READ_CHUNK_LEN;
      char* new_code = realloc(code, max_code_len);
      CHECK(new_code != NULL);
      code = new_code;
    }
  }

  fclose(f);

  /* Compile bytecode */
  ctx = duk_create_heap(NULL, NULL, NULL, NULL, bytecode_on_fatal_error);

  duk_eval_lstring(ctx, code, code_len);
  duk_dump_function(ctx);

  free(code);
  code = NULL;

  bytecode_t res;
  void* buffer = duk_require_buffer(ctx, -1, &res.size);
  CHECK(buffer != NULL);

  res.buffer = malloc(res.size);
  CHECK(res.buffer != NULL);

  memcpy(res.buffer, buffer, res.size);

  duk_destroy_heap(ctx);

  return res;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr,
      "Usage:\n"
      "./dukhttp handler.js\r\n");
    return 1;
  }

  bytecode = compile_bytecode(argv[1]);

  llhttp_settings_init(&http_settings);

  http_settings.on_message_begin = conn_on_message_begin;
  http_settings.on_url = conn_on_url;
  http_settings.on_message_complete = conn_on_message_complete;
  http_settings.on_header_field = conn_on_header_field;
  http_settings.on_header_value = conn_on_header_value;

  CHECK_EQ(0, uv_loop_init(&loop));

  CHECK_EQ(0, uv_tcp_init(&loop, &tcp_server));

  struct sockaddr_in6 addr;
  CHECK_EQ(0, uv_ip6_addr("::", 6007, &addr));

  CHECK_EQ(0, uv_tcp_bind(&tcp_server, (const struct sockaddr*) &addr, 0));
  CHECK_EQ(0, uv_listen((uv_stream_t*) &tcp_server, BACKLOG, on_connection));

  fprintf(stderr, "Listening on http://[::]:6007\r\n");

#ifndef _WIN32
  /* Ignore SIGPIPE */
  {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL);
  }
#endif

  CHECK_EQ(0, uv_run(&loop, UV_RUN_DEFAULT));

  /* NOTE: unreachable */

  return 0;
}
