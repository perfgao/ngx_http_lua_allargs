Name
===
ngx_lua_allargs_patch -- Provide a patch file for lua-nginx-module.
Mainly used to solve when key arguments is empty, can not get value arguments.

Table of Contents
===

* [Name](#name)
* [Description](#description)
* [Synopsis](#synopsis)
* [New API](new-api)
  * [ngx.req.get_uri_allargs](#ngxreqget_uri_allargs)
  * [ngx.req.get_post_allargs](#ngxreqget_post_allargs)
* [Installation](#installation)

Description
===
Because the api ngx.req.get_uri_args and ngx.req.gett_post_args:
> Empty key arguments are discarded. GET /test?=hello&=world will yield an empty output for instance.

> Empty key arguments are discarded. POST /test with body =hello&=world will yield empty outputs for instance.

So, this patch provide new api: **ngx.req.get_uri_allargs** and **ngx.req.get_post_allargs**.
Through these API calls, you can get the value arguments when key arguments is empty.

It is worth noting that, when both key and value are empty at the same time, will discarded.

Synopsis
===
```
  lua_package_path "/path/to/lua-resty-http/lib/?.lua;;";

  server {
      location /test {
          content_by_lua '
              local function print_args(args)
                for k, v in pairs(args) do
                    count = count + 1
                    if type(v) == 'table' then
                        for _, val in ipairs(v) do
                            ngx.say("key:[" .. k .. "] = ", val)
                        end
                    else
                        ngx.say("key:[" .. k .. "] = ", v)
                    end
                end
              end

              local uriargs = ngx.req.get_uri_allargs()
              print_args(uriargs)

              ngx.req.read_body()

              local bodyargs = ngx.req.get_post_allargs()
              print_args(bodyargs)
          ';
      }
  }
```

New API
===
ngx.req.get_uri_allargs
---
**syntax:** *args = ngx.req.get_uri_allargs(max_args?)*
**context:** *set_by_lua&#42;, rewrite_by_lua&#42;, access_by_lua&#42;, content_by_lua&#42;, header_filter_by_lua&#42;, body_filter_by_lua&#42;, log_by_lua&#42;, balancer_by_lua&#42;*

Returns a Lua table holding all the current request URL query arguments.

It's very similar to **ngx.req.get_uri_args**, and the only difference is that key arguments is empty, but when value arguments is not empty, you can still get value arguments.

```nginx
location = /test {
    content_by_lua_block {
        local args = ngx.req.get_uri_allargs()
        for key, val in pairs(args) do
            if type(val) == "table" then
                for _, v in ipairs(val) do
                    ngx.say("[" .. key .. "]: ", val)
                end
            else
                ngx.say("[" .. key .. "]: ", val)
            end
        end
    }
}
```

Then `GET /test?=value` will yield the response body

```bash

[]: value
```

When both key and value are empty at the same time, will discarded.
`GET /test?=value&`

```bash

[]: value
```

ngx.req.get_post_allargs
---
**syntax:** *args, err = ngx.req.get_post_allargs(max_args?)*

**context:** *rewrite_by_lua&#42;, access_by_lua&#42;, content_by_lua&#42;, header_filter_by_lua&#42;, body_filter_by_lua&#42;, log_by_lua&#42;*

Returns a Lua table holding all the current request POST query arguments (of the MIME type `application/x-www-form-urlencoded`). Call ngx.req.read_body to read the request body first or turn on the lua_need_request_body directive to avoid errors.

It's very similar to **ngx.req.get_post_args**, and the only difference is that key arguments is empty, but when value arguments is not empty, you can still get value arguments.

Installation
===

```
$ wget https://github.com/openresty/lua-nginx-module/archive/v0.10.11.tar.gz
$ git clone https://github.com/perfgao/ngx_http_lua_allargs.git
$ tar -zxvf v0.10.11.tar.gz
$ cd v0.10.11
$ patch -p1 < path/ngx_http_lua_allargs/ngx_http_lua_allargs-0.10.11.patch
```
