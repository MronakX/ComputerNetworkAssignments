#include "util.h"

ssize_t read_line(int fd, char *buffer, size_t n)
{
    ssize_t numRead;    /* # of bytes fetched by last read()*/
    size_t  totRead;    /* Total bytes read so far*/
    char *buf;
    char ch;

    if (n <= 0 || buffer == NULL){
        return -1;
    }

    buf = buffer;
    totRead = 0;
    for(;;){
        numRead = read(fd, &ch, 1);
        if (numRead == -1)
                return -1;          /* some other error*/
        else if (numRead == 0)
        {    /* EOF*/
            if (totRead == 0)       /* No bytes read so far, return 0*/
                return 0;
            else                    /* Some bytes read so far, add '\0'*/
                break;
        }else{                      
            if (totRead < n-1){     
                totRead++;
                *buf++=ch;
            }

            if (ch == '\n')
                break;
        }
    }

    *buf = '\0';
    return totRead;
}

