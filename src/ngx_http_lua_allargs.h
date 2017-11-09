
/*
 * Copyright (C) perfgao (perfgao)
 * Copyright (C) Yichun Zhang (agentzh)
 */


#ifndef _NGX_HTTP_LUA_ALLARGS_H_INCLUDED_
#define _NGX_HTTP_LUA_ALLARGS_H_INCLUDED_


#include "ngx_http_lua_common.h"


void ngx_http_lua_inject_req_allargs_api(lua_State *L);
int ngx_http_lua_parse_allargs(lua_State *L, u_char *buf, u_char *last, int max);


#endif /* _NGX_HTTP_LUA_ALLARGS_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
