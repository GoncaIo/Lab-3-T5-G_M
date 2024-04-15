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

#define F 0x5c
#define A 0x03
#define C 0x08
#define BCC A^C

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
    
     while (STOP == FALSE) {       /* loop for input */
        res = read(fd, frame, 1);   /* returns after 5 chars have been input */

        switch (estado)
        {
        case START:
            if (buffer[0] == F)
            {
                estado = FLAG_RCV;
                printf("leu a flag - estado é %d\n", estado);//debug
         
            }           
            
            break;  

        case FLAG_RCV: 
        if (res == 0x03)
        {
            estado = 2;
            printf("leu A\n");//debug
   
        }
        else if (res == 0xc5)
        {
            estado = 1;
        }
        else
        {
            estado = 0;
        }
        printf("flag rcv\n");//debug
            
            break;

        case A_RCV: 
         if (res == 0x08)
        {
            estado = C_RCV;
              printf("leu C\n");//debug
           
        }
        else if (res == 0xc5)
        {
            estado = 1;
        }
        else
        {
            estado = 0;
        }
            
            break;

        case C_RCV: 
            if (res == BCC)
        {
            estado = BCC_RCV;
              printf("leu BCC\n");//debug
          
        }
        else if (res == 0xc5)
        {
            estado = 1;
        }
        else
        {
            estado = 0;
        }
            break;

        case BCC_RCV: 
          if (res == 0xc5)
        {
            estado = STOPS;
              printf("leu BCC ok fim\n");//debug
     
        }
        else
        {
            estado = 0;
        }
            break;

   }
        
        //debug
         printf("Bytes recived\n F A C BCC F\n");
        for (int i = 0; i < 5; i++)
        {
         printf("%02x ", frame[i]);
        }
        printf("\n");

        // Verificar se o SET é válido, comprar bcc
        if (res == 5 && frame[0] == 0x5c && frame[1] == 0x03 && frame[4] == 0x5c && cal_bcc(frame[1], frame[2]) == frame[3]) {
            // SET valido
            unsigned char frameUA[5];
            frameUA[0] = 0x5c; // FLAG
            frameUA[1] = 0x01; // Address
            frameUA[2] = 0x06; // Control UA
            frameUA[3] = cal_bcc(frameUA[1], frameUA[2]); // BCC
            frameUA[4] = 0x5c; // FLAG

            // enviar resposta UA
            res = write(fd, frameUA, 5);
            printf("Sent UA frame\n");
        }

        if (frame[0] == 'z') STOP = TRUE;
    }


    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
