#include "pty.h"
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    char directory[]="/tmp/sip-softmodem-pty-XXXXXX";
    assert(mkdtemp(directory));
    char link[256],first_name[256],second_name[256];
    assert(snprintf(link,sizeof link,"%s/modem",directory)>0);
    int first_master=pty_open_link(link,first_name,sizeof first_name);
    assert(first_master>=0);
    int old_slave=open(link,O_RDWR|O_NOCTTY|O_NONBLOCK);
    assert(old_slave>=0);
    int second_master=pty_replace_link(first_master,link,second_name,
                                       sizeof second_name);
    assert(second_master>=0&&strcmp(first_name,second_name));
    struct pollfd old_poll={old_slave,POLLIN|POLLHUP,0};
    assert(poll(&old_poll,1,1000)==1);
    assert(old_poll.revents&(POLLHUP|POLLERR));
    int new_slave=open(link,O_RDWR|O_NOCTTY|O_NONBLOCK);
    assert(new_slave>=0);
    static const char message[]="new carrier";
    assert(write(second_master,message,sizeof message)==(ssize_t)sizeof message);
    char received[sizeof message];
    assert(read(new_slave,received,sizeof received)==(ssize_t)sizeof received);
    assert(!memcmp(message,received,sizeof message));
    close(old_slave);close(new_slave);close(second_master);
    unlink(link);assert(rmdir(directory)==0);
    puts("PTY carrier drop and replacement passed");
    return 0;
}
