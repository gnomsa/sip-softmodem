#include "pty.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pty.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int pty_open_link(const char *link_path,char *slave_name,size_t cap) {
    int master=-1, slave=-1; char name[PATH_MAX]; struct termios term;
    if (openpty(&master,&slave,name,NULL,NULL)<0) return -1;
    if (tcgetattr(slave,&term)<0) goto fail;
    cfmakeraw(&term); term.c_cflag |= CLOCAL|CREAD;
    if (tcsetattr(slave,TCSANOW,&term)<0) goto fail;
    if (link_path && *link_path) {
        unlink(link_path);
        if (symlink(name,link_path)<0) goto fail;
    }
    if (slave_name && cap) snprintf(slave_name,cap,"%s",name);
    close(slave); return master;
fail:
    { int saved=errno; if(master>=0)close(master); if(slave>=0)close(slave); errno=saved; return -1; }
}
