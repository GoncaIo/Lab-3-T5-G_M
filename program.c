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

int ns = 0;

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

int llwrite(char* buf, int bufSize)
 {  
    unsigned char newbuf[bufSize+bufSize+6];
    int bcc2 = 0;

    newbuf[0] = F;
    newbuf[1] = A;
    newbuf[2] = 10000000;

    if (ns == 0)
    {
        newbuf[3] = A^10000000;
    }
    else
    {
        newbuf[3] = A^1100000;
    }
    

    for (int i = 4; i < bufSize+bufSize+5; i++)
    {
        bcc2 = buf[i] ^ bcc2;

        if (buf[i] == F)
        {
            newbuf[i] = //???
        }
        
        newbuf[i] = buf[i];
    }
    
    newbuf[bufSize+bufSize+6] = bcc2;

    res = write(fd, newbuf, [bufSize+5]);
    printf("Sent information\n"); 

    if (ns == 0)
    {
        ns = 1;
    }
    else
    {
        ns = 0;
    }
    
    //maquina de estados para receber RR ou REJ e mandar novamente?

    return 1;
 }

int llread(char* packet) // nomear estados e acabar maquina de estados
{
    unsigned char frame[5];
    int estado = 0;
    int res;
    int bcc2 = 0;
    int bufferSize = 6;
    unsigned char buffer[bufferSize];

    while (estado != STOPS) {      
        res = read(fd, frame, 1);   
        switch (estado)
        {
        case 0:
            if (frame[0] == F)
            {
                estado = 1;
                printf("leu a flag - estado %d\n", estado);//debug
            }           
            
            break;  

        case 1: 
        if (frame[0] == A)
        {
            estado = 2;
            printf("leu A\n");//debug
   
        }
        else if (frame[0] == F)
        {
            estado = 1;
        }
        else
        {
            estado = 0;
        }
        printf("flag rcv\n");//debug
            
            break;

        case 2: 
         if (frame[0] == 10000000)
        {
            estado = 3;
              printf("leu C\n");//debug
           
        }
        else if (frame[0] == F)
        {
            estado = 1;
        }
        else
        {
            estado = 0;
        }
            
            break;

        case 3: 
            if (frame[0] == A^10000000)
        {
            estado = 4;
              printf("leu BCC\n");//debug
          
        }
        else if (frame[0] == F)
        {
            estado = 1;
        }
        else
        {
            estado = 0;
        }
            break;

        case 4: 
          if (frame[0] == F)
        {
            estado = 5;
              printf("leu BCC ok fim\n");//debug
     
        }
        else
        {
            estado = 0;
        }
            break;

        case 5:
        if (frame[0] == F)
        {
            estado = 6;
        }
        else
        {
            
        }
        case 6:
        if (frame[0] == bbc2)
        {
            estado = 7;
        }

        bcc2 = bcc2 ^ frame[0];
        
    }

    //enviar RR ou REJ

    return 1;
}

int main(int argc, char** argv)
{
    if ( (argc < 4) ||
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

    int bytes_read = 0;
        int write_result = 0;
        const int buf_size = MAX_PAYLOAD_SIZE;
        unsigned char buffer[buf_size];
        int total_bytes = 0;

        while (bytes_read >= 0)
        {
            bytes_read = llread(buffer);
            if(bytes_read < 0) {
                fprintf(stderr, "Error receiving from link layer\n");
                break;
            }
            else if (bytes_read > 0) {
                if (buffer[0] == 1) {
                    write_result = write(file_desc, buffer+1, bytes_read-1);
                    if(write_result < 0) {
                        fprintf(stderr, "Error writing to file\n");
                        break;
                    }
                    total_bytes = total_bytes + write_result;
                    printf("read from file -> write to link layer, %d %d %d\n", bytes_read, write_result, total_bytes);
                }
                else if (buffer[0] == 0) {
                    printf("App layer: done receiving file\n");
                    break;
                }
            }
        }

    // open file to read
        char *file_path = argv[3];
        int file_desc = open(file_path, O_RDONLY);
        if(file_desc < 0) {
            fprintf(stderr, "Error opening file: %s\n", file_path);
            exit(1);
        }

        // cycle through
        const int buf_size = MAX_PAYLOAD_SIZE-1;
        unsigned char buffer[buf_size+1];
        int write_result = 0;
        int bytes_read = 1;
        while (bytes_read > 0)
        {
            bytes_read = read(file_desc, buffer+1, buf_size);
            if(bytes_read < 0) {
                    fprintf(stderr, "Error receiving from link layer\n");
                    break;
            }
            else if (bytes_read > 0) {
                // continue sending data
                buffer[0] = 1;
                write_result = llwrite(buffer, bytes_read+1);
                if(write_result < 0) {
                    fprintf(stderr, "Error sending data to link layer\n");
                    break;
                }
                printf("read from file -> write to link layer, %d\n", bytes_read);
            }
            else if (bytes_read == 0) {
                // stop receiver
                buffer[0] = 0;
                llwrite(buffer, 1);
                printf("App layer: done reading and sending file\n");
                break;
            }

            sleep(1);
   

    //llclose()
    
    return 0;
}
