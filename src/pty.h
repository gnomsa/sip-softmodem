#ifndef SIP_SOFTMODEM_PTY_H
#define SIP_SOFTMODEM_PTY_H
#include <stddef.h>
int pty_open_link(const char *link_path, char *slave_name, size_t capacity);
int pty_replace_link(int old_master,const char *link_path,char *slave_name,
                     size_t capacity);
#endif
