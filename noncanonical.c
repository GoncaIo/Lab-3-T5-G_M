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

//SET
#define F 0x5c
#define A 0x03
#define C 0x08
#define BCC A^C

//UA
#define F 0x5c
#define AU 0x01
#define CU 0x06
#define BCCU AU^CU

//estados
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_RCV 4
#define STOPS 5

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd, res, estado=0;
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

   printf("antes while\n"); // debug


    //Maquina de estados SET
     while (estado != STOPS) {       /* loop for input */
        res = read(fd, frame, 1);   /* returns after 1 chars have been input */

        switch (estado)
        {
        case START:
            if (frame[0] == F)
            {
                estado = FLAG_RCV;
                printf("leu a flag - estado %d\n", estado);//debug
         
            }           
            
            break;  

        case FLAG_RCV: 
        if (frame[0] == A)
        {
            estado = A_RCV;
            printf("leu A\n");//debug
   
        }
        else if (frame[0] == F)
        {
            estado = FLAG_RCV;
        }
        else
        {
            estado = START;
        }
        printf("flag rcv\n");//debug
            
            break;

        case A_RCV: 
         if (frame[0] == C)
        {
            estado = C_RCV;
              printf("leu C\n");//debug
           
        }
        else if (frame[0] == F)
        {
            estado = FLAG_RCV;
        }
        else
        {
            estado = START;
        }
            
            break;

        case C_RCV: 
            if (frame[0] == BCC)
        {
            estado = BCC_RCV;
              printf("leu BCC\n");//debug
          
        }
        else if (frame[0] == F)
        {
            estado = FLAG_RCV;
        }
        else
        {
            estado = START;
        }
            break;

        case BCC_RCV: 
          if (frame[0] == F)
        {
            estado = STOPS;
              printf("leu BCC ok fim\n");//debug
     
        }
        else
        {
            estado = START;
        }
            break;

        }
     }
    //SET valido recebido

    //enviar resposta UA
    unsigned char frameUA[5];
    frameUA[0] = F; // FLAG
    frameUA[1] = AU; // Address
    frameUA[2] = CU; // Control UA
    frameUA[3] = BCCU; // BCC
    frameUA[4] = F; // FLAG

    res = write(fd, frameUA, 5);
    printf("Sent UA frame\n");   


    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
