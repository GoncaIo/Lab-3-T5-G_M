/*Read Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
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

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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

    
   // while (STOP==FALSE) {       /* loop for input */
   //     res = read(fd,buf,255);   /* returns after 5 chars have been input */
   //     buf[res]=0;               /* so we can printf... */
   //     printf(":%s:%d\n", buf, res);
   //     if (buf[0]=='z') STOP=TRUE;
   // }
    
     while (STOP == FALSE) {       /* loop for input */
        res = read(fd, frame, 5);   /* returns after 5 chars have been input */

        //debug
         printf("Bytes recived\n F A C BCC F\n")
        for (int i = 0; i < 5; i++)
        {
         printf("%02x ", frame[i]);
        }
        printf("\n");

        // Verificar se o SET é válido
        if (res == 5 && frame[0] == 0x5c && frame[2] == 0x08 && frame[4] == 0x5c) {
            // Quadro SET recebido, enviar resposta UA
            unsigned char frameUA[5];
            frameUA[0] = 0x5c; // FLAG
            frameUA[1] = 0x01; // Address
            frameUA[2] = 0x06; // Control UA
            frameUA[3] = cal_bcc(frameUA[1], frameUA[2]); // BCC
            frameUA[4] = 0x5c; // FLAG

            // Escrever o quadro UA na porta serial
            res = write(fd, frameUA, 5);
            printf("Sent UA frame\n");
        }

        if (frame[0] == 'z') STOP = TRUE;
    }


    /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião
    */

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
