/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

//XOR para calcular o BCC1
unsigned char cal_bcc(unsigned char address, unsigned char control)
{
    unsigned char bcc = 0;

    bcc = address^control;

    return bcc;
}

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

    unsigned char frame[5];

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");


    /*
    for (i = 0; i < 255; i++) {
        buf[i] = 'a';
    }
    */

    frame[0] = 0x5c; // FLAG
    frame[1] = 0x03; // Address
    frame[2] = 0x08; // Control (SET)
    frame[3] = cal_bcc(frame[1], frame[2]); // BCC
    frame[4] = 0x5c; // FLAG

    /*testing
    buf[25] = '\n';
    */

    //debug
    printf("Bytes sent\n F A C BCC F\n")
    for (int i = 0; i < 5; i++)
    {
        printf("%02x ", frame[i]);
    }
    printf("\n");

    res = write(fd,frame,5);
    printf("%d bytes written\n", res);

    // UA recebida
    res = read(fd, frameUA, 5);
    printf("%d bytes received\n", res);

    // Verificar se o UA é válido, comprar bcc
        if (res == 5 && frameUA[0] == 0x5c && frameUA[1] == 0x01 && frameUA[4] == 0x5c && cal_bcc(frameUA[1], frameUA[2]) == frameUA[3]) {
            // SET valido
            printf("Received correct UA frame\n");
        }


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 0;
}
