// Copyright (c) 2022 Cesanta Software Limited
// All rights reserved

#include "mongoose.h"

const char *s_listening_url = "http://localhost:8000";
int g_logined = 0;

char *g_netIp = "192.168.100.100";
char *g_netMask = "255.255.0.0";
char *g_netGateway = "192.168.100.1";

static struct net_config
{
  char *ip, *mask, *gateway;
} s_netConfig;

// Authenticated user.
// A user can be authenticated by:
//   - a name:pass pair
//   - a token
// When a user is shown a login screen, they enter a user:pass. If successful,
// the server returns the user info, which includes the token. From that point
// on, the client can use the token for authentication. Tokens could be
// refreshed/changed at server side, forcing clients to re-login.
struct user
{
  const char *name, *pass, *token;
};

// Parse HTTP requests, return authenticated user or NULL
static struct user *getuser(struct mg_http_message *hm)
{
  // In production, make passwords strong and tokens randomly generated
  // In this example, user list is kept in RAM. In production, it can
  // be backed by file, database, or some other method.
  static struct user users[] = {
      {"admin", "12345678", "admin_token"},
      {"user1", "pass1", "user1_token"},
      {"user2", "pass2", "user2_token"},
      {NULL, NULL, NULL},
  };
  char user[256], pass[256];
  struct user *u;
  mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass));
  if (user[0] != '\0' && pass[0] != '\0')
  {
    // Both user and password are set, search by user/password
    for (u = users; u->name != NULL; u++)
      if (strcmp(user, u->name) == 0 && strcmp(pass, u->pass) == 0)
        return u;
  }
  else if (user[0] == '\0')
  {
    // Only password is set, search by token
    for (u = users; u->name != NULL; u++)
      if (strcmp(pass, u->token) == 0)
        return u;
  }
  return NULL;
}

// Try to update a single configuration value
static void update_config(struct mg_str json, const char *path, char **value)
{
  char *jval;
  if ((jval = mg_json_get_str(json, path)) != NULL)
  {
    free(*value);
    *value = strdup(jval);
  }
}

void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    if (0 == g_logined)
    {
      if (mg_http_match_uri(hm, "/api/login"))
      {
        struct user *u = getuser(hm);
        if (u == NULL)
        {
          // All URIs starting with /api/ must be authenticated
          mg_http_reply(c, 403, "", "Denied\n");
        }
        else
        {
          mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                        "{%Q:%Q,%Q:%Q}\n", "user", u->name, "token", u->token);
          g_logined = 1;
        }
      }
      else
      {
        struct mg_http_serve_opts opts = {.root_dir = "web_root"};
        mg_http_serve_dir(c, ev_data, &opts);
      }
    }
    if (1 == g_logined)
    {
      if (mg_http_match_uri(hm, "/api/deviceinfo/get"))
      {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                      "{%Q:%Q,%Q:%Q}\n", "mac", "01:12:34:56:ab:cd",
                      "version", "V0.0");
      }
      else if (mg_http_match_uri(hm, "/api/network/get"))
      {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                      "{%Q:%Q,%Q:%Q,%Q:%Q}\n", "ip", s_netConfig.ip,
                      "netmask", s_netConfig.mask, "gateway", s_netConfig.gateway);
      }
      else if (mg_http_match_uri(hm, "/api/network/set"))
      {
        struct mg_str json = hm->body;
        update_config(json, "$.ip", &s_netConfig.ip);
        update_config(json, "$.netmask", &s_netConfig.mask);
        update_config(json, "$.gateway", &s_netConfig.gateway);
        MG_DEBUG(("ip %s netmask %s gateway %s", s_netConfig.ip, s_netConfig.mask, s_netConfig.gateway));
        mg_http_reply(c, 200, "", "ok\n");
      }
      else if (mg_http_match_uri(hm, "/upload/versionfile")) // POST  http://localhost:8000/upload/versionfile?offset=0&name=kill_something.sh
      {
        char path[80], name[64];
        mg_http_get_var(&hm->query, "name", name, sizeof(name));
        if (name[0] == '\0')
        {
          mg_http_reply(c, 400, "", "%s", "name required");
        }
        else
        {
          mg_snprintf(path, sizeof(path), "/tmp/%s", name);
          mg_http_upload(c, hm, &mg_fs_posix, mg_remove_double_dots(path), MG_MAX_RECV_SIZE);
        }
      }else if (mg_http_match_uri(hm, "/rebootcmd")) 
      {
        mg_http_reply(c, 200, "", "ok\n");
      }
    }
    else
    {
      struct mg_http_serve_opts opts = {.root_dir = "web_root"};
      mg_http_serve_dir(c, ev_data, &opts);
    }
  }
  (void)fn_data;
}

int main(void)
{
  struct mg_mgr mgr;

  s_netConfig.ip = strdup(g_netIp);
  s_netConfig.mask = strdup(g_netMask);
  s_netConfig.gateway = strdup(g_netGateway);

  mg_log_set(MG_LL_DEBUG);
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, s_listening_url, fn, &mgr);
  while (mgr.conns != NULL)
    mg_mgr_poll(&mgr, 50);
  mg_mgr_free(&mgr);
  return 0;
}
