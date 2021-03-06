#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#define DATE_MAX_LEN 30


typedef struct {
    ngx_uint_t    gmt;
    u_char       *date_fmt;
} ngx_http_strftime_ctx_t;


static char *ngx_http_strftime(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
ngx_int_t    ngx_http_strftime_time_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);


static ngx_command_t ngx_http_strftime_commands[] = {
    {ngx_string("strftime"),
     NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE23,
     ngx_http_strftime,
     0,
     0,
     NULL
    },

    ngx_null_command
};


static ngx_http_module_t ngx_http_strftime_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

ngx_module_t ngx_http_strftime_module = {
    NGX_MODULE_V1,
    &ngx_http_strftime_module_ctx,
    ngx_http_strftime_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};


static char *
ngx_http_strftime(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_strftime_ctx_t *ctx;
    ngx_http_variable_t     *v;
    ngx_str_t               *args;
    ngx_str_t                var_name;
    ngx_str_t                date_fmt;
    ngx_str_t                zone = ngx_string("local");

    if (cf->args->nelts < 2) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "Incorrect use of strftime. Try 'strftime variable_name date_time_format [local|gmt (local is the default)]'");
        return NGX_CONF_ERROR;
    }

    args = (ngx_str_t *) cf->args->elts;
    var_name = args[1];
    date_fmt = args[2];
    if (cf->args->nelts > 3) {
        zone = args[3];
    }

    ctx = ngx_palloc(cf->pool, sizeof(ngx_http_strftime_ctx_t));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    ctx->date_fmt = ngx_palloc(cf->pool, date_fmt.len + 1);
    if (ctx->date_fmt == NULL) {
        return NGX_CONF_ERROR;
    }

    ctx->gmt = ((zone.len == 3) && (ngx_strncasecmp(zone.data, (u_char *) "gmt", 3) == 0));
    ngx_memcpy(ctx->date_fmt, date_fmt.data, date_fmt.len);
    ctx->date_fmt[date_fmt.len] = '\0';

    v = ngx_http_add_variable(cf, &var_name, NGX_HTTP_VAR_NOCACHEABLE);
    v->get_handler = ngx_http_strftime_time_variable;
    v->data = (uintptr_t) ctx;

    return NGX_CONF_OK;
}


ngx_int_t
strftime_now(ngx_http_variable_value_t *var, u_char *date_fmt, ngx_uint_t gmt, ngx_pool_t *pool)
{
    struct tm    tm;
    time_t       now = ngx_time();
    char         buf[DATE_MAX_LEN];

    if (var->len != 0) {
        return NGX_OK;
    }

    if (gmt) {
        ngx_libc_gmtime(now, &tm);
    } else {
        ngx_libc_localtime(now, &tm);
    }

    var->len = strftime(buf, DATE_MAX_LEN, (char *) date_fmt, &tm);
    if (var->len == 0) {
        return NGX_ERROR;
    }

    var->data = ngx_palloc(pool, var->len);
    if (var->data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(var->data, buf, var->len);
    var->valid = 1;
    var->no_cacheable = 0;
    var->not_found = 0;

    return NGX_OK;
}


ngx_int_t
ngx_http_strftime_time_variable(ngx_http_request_t *r, ngx_http_variable_value_t *var, uintptr_t data)
{
    ngx_http_strftime_ctx_t *ctx = (ngx_http_strftime_ctx_t *) data;
    return strftime_now(var, ctx->date_fmt, ctx->gmt, r->pool);
}
