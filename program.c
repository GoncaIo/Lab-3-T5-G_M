/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
//#include <linklayer.h> //falta incluir esta biblioteca     
#include <stdlib.h>      
#include <string.h>                           


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

  typedef struct linkLayer{
    char serialPort[50];
    int role; //defines the role of the program: 0==Transmitter, 1=Receiver
    int baudRate;
    int numTries;
    int timeOut;
} linkLayer;

int llopen(linkLayer connectionParameters)
{
    int fd, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0, estado = 0;

    unsigned char frame[5], frameAU[5];

    if (connectionParameters.role == transmitter)
    {
        printf("\nTransmitter\n\n");
        /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(connectionParameters.serialPort); return(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        return(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */
  

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        return(-1);
    }

    printf("New termios structure set\n\n");

    frame[0] = F; 
    frame[1] = A;
    frame[2] = C; 
    frame[3] = BCC; 
    frame[4] = F; 

    //debug
    printf("Bytes sent\n F   A   C      BCC  F\n");
    for (int i = 0; i < 5; i++)
    {
        printf("%02x  ", frame[i]);
    }
    printf("\n\n");

    res = write(fd,frame,5);
    printf("%d bytes written\n\n", res);

    printf("Reciving UA:\n\n");
    //Maquina de estados resposta UA
   while (estado != STOPS) {       /* loop for input */
        res = read(fd, frame, 1);   /* returns after 1 chars have been input */
        printf("Entrou no while\n");
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
              printf("leu BCC ok fim UA\n");//debug
     
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
        return(-1);
    }


    close(fd);
    }

    else if (connectionParameters.role == receiver)
    {
        printf("\nReceiver\n\n");
        /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(connectionParameters.serialPort); return(-1); }

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        return(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        return(-1);
    }

   printf("New termios structure set\n\n");


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
    }
    
    return 1;
}

int main(int argc, char** argv)
{
    if ( (argc < 3) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort and role\n\tex: nserial /dev/ttyS1\n   1");
        exit(1);
    }

  

    linkLayer myLinkLayer;

    strcpy(myLinkLayer.serialPort, argv[1]);
    myLinkLayer.role = atoi(argv[2]);
    myLinkLayer.baudRate = BAUDRATE;
    myLinkLayer.numTries = 3;
    myLinkLayer.timeOut = 4;

    
    llopen(myLinkLayer);

    //llread()
    //receber uma trama com Ns = 0
    //fazer maquina de estados para receber trama
    //testar com uma string de 50 caracteres

    //llwrite()
    //mandar uma trama com Ns = 1

    //llclose()
    
    return 0;
}
