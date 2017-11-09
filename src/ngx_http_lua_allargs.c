
/*
 * Copyright (C) perfgao (perfgao)
 * Copyright (C) Xiaozhe Wang (chaoslawful)
 * Copyright (C) Yichun Zhang (agentzh)
 */


#ifndef DDEBUG
#define DDEBUG 0
#endif
#include "ddebug.h"


#include "ngx_http_lua_allargs.h"
#include "ngx_http_lua_util.h"


static int ngx_http_lua_ngx_req_get_uri_allargs(lua_State *L);
static int ngx_http_lua_ngx_req_get_post_allargs(lua_State *L);


static int
ngx_http_lua_ngx_req_get_uri_allargs(lua_State *L)
{
    ngx_http_request_t          *r;
    u_char                      *buf;
    u_char                      *last;
    int                          retval;
    int                          n;
    int                          max;

    n = lua_gettop(L);

    if (n != 0 && n != 1) {
        return luaL_error(L, "expecting 0 or 1 arguments but seen %d", n);
    }

    if (n == 1) {
        max = luaL_checkinteger(L, 1);
        lua_pop(L, 1);

    } else {
        max = NGX_HTTP_LUA_MAX_ARGS;
    }

    r = ngx_http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "no request object found");
    }

    ngx_http_lua_check_fake_request(L, r);

    if (r->args.len == 0) {
        lua_createtable(L, 0, 0);
        return 1;
    }

    /* we copy r->args over to buf to simplify
     * unescaping query arg keys and values */

    buf = ngx_palloc(r->pool, r->args.len);
    if (buf == NULL) {
        return luaL_error(L, "no memory");
    }

    lua_createtable(L, 0, 4);

    ngx_memcpy(buf, r->args.data, r->args.len);

    last = buf + r->args.len;

    retval = ngx_http_lua_parse_allargs(L, buf, last, max);

    ngx_pfree(r->pool, buf);

    return retval;
}


static int
ngx_http_lua_ngx_req_get_post_allargs(lua_State *L)
{
    ngx_http_request_t          *r;
    u_char                      *buf;
    int                          retval;
    size_t                       len;
    ngx_chain_t                 *cl;
    u_char                      *p;
    u_char                      *last;
    int                          n;
    int                          max;

    n = lua_gettop(L);

    if (n != 0 && n != 1) {
        return luaL_error(L, "expecting 0 or 1 arguments but seen %d", n);
    }

    if (n == 1) {
        max = luaL_checkinteger(L, 1);
        lua_pop(L, 1);

    } else {
        max = NGX_HTTP_LUA_MAX_ARGS;
    }

    r = ngx_http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "no request object found");
    }

    ngx_http_lua_check_fake_request(L, r);

    if (r->discard_body) {
        lua_createtable(L, 0, 0);
        return 1;
    }

    if (r->request_body == NULL) {
        return luaL_error(L, "no request body found; "
                          "maybe you should turn on lua_need_request_body?");
    }

    if (r->request_body->temp_file) {
        lua_pushnil(L);
        lua_pushliteral(L, "request body in temp file not supported");
        return 2;
    }

    if (r->request_body->bufs == NULL) {
        lua_createtable(L, 0, 0);
        return 1;
    }

    /* we copy r->request_body->bufs over to buf to simplify
     * unescaping query arg keys and values */

    len = 0;
    for (cl = r->request_body->bufs; cl; cl = cl->next) {
        len += cl->buf->last - cl->buf->pos;
    }

    dd("post body length: %d", (int) len);

    if (len == 0) {
        lua_createtable(L, 0, 0);
        return 1;
    }

    buf = ngx_palloc(r->pool, len);
    if (buf == NULL) {
        return luaL_error(L, "no memory");
    }

    lua_createtable(L, 0, 4);

    p = buf;
    for (cl = r->request_body->bufs; cl; cl = cl->next) {
        p = ngx_copy(p, cl->buf->pos, cl->buf->last - cl->buf->pos);
    }

    dd("post body: %.*s", (int) len, buf);

    last = buf + len;

    retval = ngx_http_lua_parse_allargs(L, buf, last, max);

    ngx_pfree(r->pool, buf);

    return retval;
}


int
ngx_http_lua_parse_allargs(lua_State *L, u_char *buf, u_char *last, int max)
{
    u_char                      *p, *q;
    u_char                      *src, *dst;
    unsigned                     parsing_value;
    size_t                       len, vallen;
    int                          count = 0;
    int                          top;

    top = lua_gettop(L);

    p = buf;

    parsing_value = 0;
    q = p;

    while (p != last) {
        if (*p == '=' && ! parsing_value) {
            /* key data is between p and q */

            src = q; dst = q;

            ngx_http_lua_unescape_uri(&dst, &src, p - q,
                                      NGX_UNESCAPE_URI_COMPONENT);

            dd("pushing key %.*s", (int) (dst - q), q);

            /* push the key */
            lua_pushlstring(L, (char *) q, dst - q);

            /* skip the current '=' char */
            p++;

            q = p;
            parsing_value = 1;

        } else if (*p == '&') {
            /* reached the end of a key or a value, just save it */
            src = q; dst = q;

            ngx_http_lua_unescape_uri(&dst, &src, p - q,
                                      NGX_UNESCAPE_URI_COMPONENT);

            dd("pushing key or value %.*s", (int) (dst - q), q);

            /* push the value or key */
            lua_pushlstring(L, (char *) q, dst - q);

            /* skip the current '&' char */
            p++;

            q = p;

            if (parsing_value) {
                /* end of the current pair's value */
                parsing_value = 0;

            } else {
                /* the current parsing pair takes no value,
                 * just push the value "true" */
                dd("pushing boolean true");

                lua_pushboolean(L, 1);
            }

            (void) lua_tolstring(L, -2, &len);
            (void) lua_tolstring(L, -1, &vallen);

            if (len == 0 && vallen == 0) {
                /* ignore empty string key pairs */
                dd("popping key and value...");
                lua_pop(L, 2);

            } else {
                dd("setting table...");
                ngx_http_lua_set_multi_value_table(L, top);
            }

            if (max > 0 && ++count == max) {
                ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0,
                               "lua hit query args limit %d", max);

                return 1;
            }

        } else {
            p++;
        }
    }

    if (p != q || parsing_value) {
        src = q; dst = q;

        ngx_http_lua_unescape_uri(&dst, &src, p - q,
                                  NGX_UNESCAPE_URI_COMPONENT);

        dd("pushing key or value %.*s", (int) (dst - q), q);

        lua_pushlstring(L, (char *) q, dst - q);

        if (!parsing_value) {
            dd("pushing boolean true...");
            lua_pushboolean(L, 1);
        }

        (void) lua_tolstring(L, -2, &len);
        (void) lua_tolstring(L, -1, &vallen);

        if (len == 0 && vallen == 0) {
            /* ignore empty string key pairs */
            dd("popping key and value...");
            lua_pop(L, 2);

        } else {
            dd("setting table...");
            ngx_http_lua_set_multi_value_table(L, top);
        }
    }

    dd("gettop: %d", lua_gettop(L));
    dd("type: %s", lua_typename(L, lua_type(L, 1)));

    if (lua_gettop(L) != top) {
        return luaL_error(L, "internal error: stack in bad state");
    }

    return 1;
}


void
ngx_http_lua_inject_req_allargs_api(lua_State *L)
{
    lua_pushcfunction(L, ngx_http_lua_ngx_req_get_uri_allargs);
    lua_setfield(L, -2, "get_uri_allargs");

    lua_pushcfunction(L, ngx_http_lua_ngx_req_get_post_allargs);
    lua_setfield(L, -2, "get_post_allargs");
}
