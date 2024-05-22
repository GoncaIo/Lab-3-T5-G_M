/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <linklayer.h>


#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

//roles
#define transmitter 0
#define receiver 1

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

llopen(int role)
{
    if (role == transmitter)
    {
        printf("Transmitter\n");
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

    frame[0] = F; // FLAG
    frame[1] = A; // Address
    frame[2] = C; // Control (SET)
    frame[3] = BCC; // BCC
    frame[4] = F; // FLAG

    /*testing
    buf[25] = '\n';
    */

    //debug
    printf("Bytes sent\n F A C BCC F\n");
    for (int i = 0; i < 5; i++)
    {
        printf("%02x ", frame[i]);
    }
    printf("\n\n");

    res = write(fd,frame,5);
    printf("%d bytes written\n", res);


    //Maquina de estados resposta UA
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
        if (frame[0] == AU)
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
         if (frame[0] == CU)
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
            if (frame[0] == BCCU)
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
   //UA valido recebido


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    }



    else if (role == receiver)
    {
        printf("Receiver\n");
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


   


    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    }
    
    return 0;
}

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0, estado = 0;

    unsigned char frame[5], frameAU[5];

    if ( (argc < 3) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort and role\n\tex: nserial /dev/ttyS1\n   1");
        exit(1);
    }

    llopen(argv[3]);
    
    return 0;
}
