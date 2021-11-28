#include <stdbool.h>
#include <signal.h>
#include <unistd.h>


bool stop = false;

static void signal_handler(int sig)
{
    switch (sig) {
    case SIGUSR1:
        stop = true;
        break;
    }
}

void victim(void)
{
    unsigned int remains = 10;

    signal(SIGUSR1, signal_handler);

    /* Just sleeping here, waiting to be victimized. */

    while (remains > 0 && !stop) {
        remains = sleep(remains);
    }
}
