#include "linklayer.h"

// Frame Specifications
#define F 0x5c
#define At 0x01
#define Ar 0x03

// Control Field
#define SET 0x07
#define UA 0x06
#define I0 0x80
#define I1 0xC0
#define RR0 0x01
#define RR1 0x11
#define REJ0 0x05
#define REJ1 0x15
#define DISC 0x0A

// Byte Stuffing
#define ESC 0x5d

// Estados
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_RCV 4
#define STOPS 5
#define DATA_RCV 6
#define BCC2_RCV 7

#define transmitter 0
#define receiver 1

volatile int STOP = FALSE;
int ns = 0, nr = 0, estado = 0, bytesRead = 0, bytesWritten = 0;

// Opens a connection using the "port" parameters defined in struct linkLayer, returns "-1" on error and "1" on sucess
int llopen(linkLayer connectionParameters)
{
    printf("- Opening connection - llopen()\n\n");
    int fd, res;
    struct termios oldtio, newtio;
    unsigned char frame[5], frameAU[5];

    /*
   Open serial port device for reading and writing and not as controlling tty
   because we don't want to get killed if linenoise sends CTRL-C.
   */

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        return (-1);
    }

    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        return (-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return (-1);
    }

    printf("New termios structure set\n\n");

    if (connectionParameters.role == transmitter)
    {
        frame[0] = F;
        frame[1] = At;
        frame[2] = SET;
        frame[3] = At ^ SET;
        frame[4] = F;

        printf("Bytes sent\n");
        for (int i = 0; i < 5; i++)
        {
            printf("%02x  ", frame[i]);
        }
        printf("\n\n");

        res = write(fd, frame, 5);

        // State Machine UA
        printf("State Machine UA:\n\n");
        while (STOP == FALSE)
            {    
                sleep(0.1);                            /* loop for input */
                res = read(fd, frameAU, 1); /* returns after 1 chars have been input */

                switch (estado)
                {
                case START:
                    if (frameAU[0] == F)
                    {
                        estado = FLAG_RCV;
                        printf("estado %d - flag\n", estado); // debug
                    }

                    break;

                case FLAG_RCV:
                    if (frameAU[0] == Ar)
                    {
                        estado = A_RCV;
                        printf("estado %d - address\n", estado); // debug
                    }
                    else if (frameAU[0] == F)
                    {
                        estado = FLAG_RCV;
                    }
                    else
                    {
                        estado = START;
                    }
                    break;

                case A_RCV:
                    if (frameAU[0] == UA)
                    {
                        estado = C_RCV;
                        printf("estado %d - control\n", estado); // debug
                    }
                    else if (frameAU[0] == F)
                    {
                        estado = FLAG_RCV;
                    }
                    else
                    {
                        estado = START;
                    }

                    break;

                case C_RCV:
                    if (frameAU[0] == (Ar ^ UA))
                    {
                        estado = BCC_RCV;
                        printf("estado %d - BCC\n", estado); // debug
                    }
                    else if (frameAU[0] == F)
                    {
                        estado = FLAG_RCV;
                    }
                    else
                    {
                        estado = START;
                    }
                    break;

                case BCC_RCV:
                    if (frameAU[0] == F)
                    {
                        STOP = TRUE;
                        printf("estado %d - valid UA\n\n", estado); // debug
                    }
                    else
                    {
                        estado = START;
                    }
                    break;
                }
            }
            // UA valido recebido
        
        estado = START;
        STOP = FALSE;
    }
    else if (connectionParameters.role == receiver)
    {
        // State Machine SET
        printf("State Machine SET:\n\n"); // debug
        while (STOP == FALSE)
        {   
            sleep(0.1); 
            res = read(fd, frame, 1);

            switch (estado)
            {
            case START:
                if (frame[0] == F)
                {
                    estado = FLAG_RCV;
                    printf("estado %d - flag\n", estado); // debug
                }

                break;

            case FLAG_RCV:
                if (frame[0] == At)
                {
                    estado = A_RCV;
                    printf("estado %d - address\n", estado); // debug
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

            case A_RCV:
                if (frame[0] == SET)
                {
                    estado = C_RCV;
                    printf("estado %d - control\n", estado); // debug
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
                if (frame[0] == (At ^ SET))
                {
                    estado = BCC_RCV;
                    printf("estado %d - BCC\n", estado); // debug
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
                    STOP = TRUE;
                    printf("estado %d - valid SET\n\n", estado); // debug
                }
                else
                {
                    estado = START;
                }
                break;
            }
        }
        estado = START;
        STOP = FALSE;
        // SET valido recebido

        // enviar resposta UA
        unsigned char frameUA[5];
        frameUA[0] = F;       // FLAG
        frameUA[1] = Ar;      // Address
        frameUA[2] = UA;      // Control UA
        frameUA[3] = Ar ^ UA; // BCC
        frameUA[4] = F;       // FLAG

        res = write(fd, frameUA, 5);
        printf("Sent UA frame\n\n");
    }

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return (-1);
    }

    close(fd);

    return 1;
}

// Sends data in buf with size bufSize
int llwrite(char *buf, int bufSize)
{
    printf("\n- Sending information - llwrite()\n\n");

    int fd, res, newbufSize = bufSize * 2 + 6;
    unsigned char newbuf[newbufSize], frame[1];
    int bcc2 = 0, i;

    newbuf[0] = F;
    newbuf[1] = At;
    newbuf[2] = ns ? I0 : I1;
    newbuf[3] = At ^ newbuf[2];

    // byte stuffing
    for (int i = 0; i < bufSize; i++)
    {
        bcc2 = buf[i] ^ bcc2;

        if ((buf[i] == F) || (buf[i] == ESC))
        {
            newbuf[i] = ESC;
            newbuf[i + 1] = buf[i] ^ 0x20;
            i++;
        }
        else
        {
            newbuf[i] = buf[i];
        }
        bytesWritten++;
    }

    newbuf[i + 5] = bcc2;
    newbuf[i + 6] = F;

    res = write(fd, newbuf, newbufSize);
    printf("Sent information\n\n");

    // Maquina de estados receção RR ou REJ
    printf("State Machine Receiving RR or REJ:\n\n");
    while (STOP == FALSE)
    {                 
        sleep(0.1);           
        res = read(fd, frame, 1); 

        switch (estado)
        {
        case START:
            if (frame[0] == F)
            {
                estado = FLAG_RCV;
                printf("estado %d - flag\n", estado); // debug
            }

            break;

        case FLAG_RCV:
            if (frame[0] == Ar)
            {
                estado = A_RCV;
                printf("estado %d - address\n", estado); // debug
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

        case A_RCV:
            if (frame[0] == RR0 || frame[0] == RR1)
            {
                if (frame[0] == RR0)
                {
                    ns = 0;
                }
                else
                {
                    ns = 1;
                }

                estado = C_RCV;
                printf("estado %d - control\n", estado); // debug
            }
            else if (frame[0] == REJ0 || frame[0] == REJ1)
            {
                STOP = TRUE;
                int llwrite(char *buf, int bufSize);
                estado = START;
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
            if (frame[0] == (At ^ UA))
            {
                estado = BCC_RCV;
                printf("estado %d - BCC\n", estado); // debug
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
                STOP = TRUE;
                printf("estado %d - valid UA\n\n", estado); // debug
            }
            else
            {
                estado = START;
            }
            break;
        }
    }
    estado = START;
    STOP = FALSE;

    return i++;
}

// Receive data in packet
int llread(char *packet) // nomear estados e acabar maquina de estados
{
    printf("- Receiving information - llread()\n\n");
    int fd, res, bcc2 = 0, bufferSize = 2500, i=0;
    unsigned char frame[1], buffer[bufferSize];

    // State Machine Receiving Information
    printf("State Machine Receiving I:\n\n");
    while (STOP == FALSE)
    {
        sleep(0.1); 
        res = read(fd, frame, 1);

        switch (estado)
        {
        case START:
            if (frame[0] == F)
            {
                estado = FLAG_RCV;
                printf("estado %d - flag\n", estado); // debug
            }
            break;

        case FLAG_RCV:
            if (frame[0] == At)
            {
                estado = A_RCV;
                printf("estado %d - address\n", estado); // debug
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

        case A_RCV:
            if ((frame[0] == I0 && ns == 0) || (frame[0] == I1 && ns == 1))
            {   
                if (frame[0] == I0)
                {
                    nr = 1;
                }
                else
                {
                    nr = 0;
                }
                estado = C_RCV;
                printf("estado %d - control N(%d)\n", estado, ns); // debug
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
            if ((frame[0] == I0 ^ At && ns == 0) || (frame[0] == I1 ^ At && ns == 1))
            {
                estado = BCC_RCV;
                printf("estado %d - BCC1\n", estado); // debug
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
            if (frame[0] != F)
            {
                packet[i] = frame[0];
                printf("estado %d - data\n", estado); // debug
                estado = DATA_RCV;
            }
            else if (frame[0] == F)
            {
                estado = BCC2_RCV; 
            }
            
            else
            {
                estado = FLAG_RCV;
            }
            break;

        case DATA_RCV:
            if (frame[0] == F)
            {
                estado = BCC2_RCV;
                printf("estado %d - end flag\n", estado);
            }
            else
            {
                if (frame[0] == ESC)
                {
                    res = read(fd, frame, 1);

                    if (frame[0] == 0x7c)
                    {
                        packet[i++] = F;
                    }
                    else if (frame[0] == 0x7d)
                    {
                        packet[i++] = ESC;
                    }
                }
                else
                {
                    packet[i++] = frame[0];
                }
                bcc2 = packet[i] ^ bcc2;
            }
            break;

        case BCC2_RCV:
            if (bcc2 == packet[--i])
            { // BCC2 is valid
                printf("Valid frame received\n");

                // Send RR
                unsigned char rrFrame[5];
                rrFrame[0] = F;
                rrFrame[1] = Ar;
                rrFrame[2] = nr ? RR0 : RR1;
                rrFrame[3] = Ar ^ rrFrame[2]; 
                rrFrame[4] = F;

                res = write(fd, rrFrame, 5);
                if (res != 5)
                {
                    perror("write");
                    return -1;
                }
                printf("RR frame sent\n");

                STOP = TRUE;
            }
            else
            {
                printf("BCC2 error\n");
                // Send REJ
                unsigned char rejFrame[5];
                rejFrame[0] = F;
                rejFrame[1] = Ar;
                rejFrame[2] = nr ? REJ0 : REJ1;
                rejFrame[3] = Ar ^ rejFrame[2]; 
                rejFrame[4] = F;

                res = write(fd, rejFrame, 5);
                if (res != 5)
                {
                    perror("write");
                    return -1;
                }
                printf("REJ frame sent\n");

                estado = START;
            }
            break;
        }
    }
    estado = START;
    STOP = FALSE;

    bytesWritten = i - 6;
    return bytesWritten;
}

// Closes previously opened connection; if showStatistics==TRUE, link layer should print statistics in the console on close
int llclose(linkLayer connectionParameters, int showStatistics)
{
    printf("- Closing connection - llclose()\n\n");
    int fd, res, estado = 0;
    struct termios oldtio, newtio;
    unsigned char frame[5], frameAU[5];

    if (connectionParameters.role == transmitter)
    {
        // Create and send DISC frame
        frame[0] = F;
        frame[1] = At;
        frame[2] = DISC;
        frame[3] = At ^ DISC;
        frame[4] = F;

        res = write(fd, frame, 5);
        printf("Sent DISC frame\n\n");

        // State Machine - reciving DISC from reciver
        printf("State Machine - reciving DISC from reciver:\n\n");
        while (STOP == FALSE)
            {     
                 sleep(0.1);                         
                res = read(fd, frame, 1);

                switch (estado)
                {
                case START:
                    if (frame[0] == F)
                    {
                        estado = FLAG_RCV;
                        printf("estado %d - flag received\n", estado); // debug
                    }

                    break;

                case FLAG_RCV:
                    if (frame[0] == Ar)
                    {
                        estado = A_RCV;
                        printf("estado %d - address received\n", estado); // debug
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

                case A_RCV:
                    if (frame[0] == DISC)
                    {
                        estado = C_RCV;
                        printf("estado %d - C_DISC\n", estado); // debug
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
                    if (frame[0] == Ar ^ DISC)
                    {
                        estado = BCC_RCV;
                        printf("estado %d - BCC\n", estado); // debug
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
                        STOP = TRUE;
                        printf("estado %d - valid DISC\n\n", estado); // debug
                    }
                    else
                    {
                        estado = START;
                    }
                    break;
                }
            }
            // DISC valido recebido

            // enviar resposta UA
            unsigned char frameUA[5];
            frameUA[0] = F;       // FLAG
            frameUA[1] = At;      // Address
            frameUA[2] = UA;      // Control UA
            frameUA[3] = At ^ UA; // BCC
            frameUA[4] = F;       // FLAG

            res = write(fd, frameUA, 5);
            printf("Sent UA frame\n");
        
    }
    else if (connectionParameters.role == receiver)
    {
        // State Machine SET
        printf("State Machine - reciving DISC from transmitter:\n\n"); // debug
        while (STOP == FALSE)
        {
            sleep(0.1); 
            res = read(fd, frame, 1);

            switch (estado)
            {
            case START:
                if (frame[0] == F)
                {
                    estado = FLAG_RCV;
                    printf("estado %d - flag\n", estado); // debug
                }

                break;

            case FLAG_RCV:
                if (frame[0] == At)
                {
                    estado = A_RCV;
                    printf("estado %d - address\n", estado); // debug
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

            case A_RCV:
                if (frame[0] == DISC)
                {
                    estado = C_RCV;
                    printf("estado %d - C_DISC\n", estado); // debug
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
                if (frame[0] == At ^ DISC)
                {
                    estado = BCC_RCV;
                    printf("estado %d - BCC\n", estado); // debug
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
                    STOP = TRUE;
                    printf("estado %d - valid DISC\n\n", estado); // debug
                }
                else
                {
                    estado = START;
                }
                break;
            }
        }
        estado = START;
        STOP = FALSE;
        // DISC valido recebido

        frame[0] = F;
        frame[1] = Ar;
        frame[2] = DISC;
        frame[3] = Ar ^ DISC;
        frame[4] = F;

        res = write(fd, frame, 5);

        // receive UA frame
        estado = START;
        STOP = FALSE;

        printf("State Machine - reciving UA from transmitter:\n\n");
        while (STOP == FALSE)
        {
            sleep(0.1); 
            res = read(fd, frame, 1);

            switch (estado)
            {
            case START:
                if (frame[0] == F)
                {
                    estado = FLAG_RCV;
                    printf("State %d - flag\n", estado);
                }
                break;

            case FLAG_RCV:
                if (frame[0] == At)
                {
                    estado = A_RCV;
                    printf("State %d - address\n", estado);
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

            case A_RCV:
                if (frame[0] == UA)
                {
                    estado = C_RCV;
                    printf("State %d - control\n", estado);
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
                if (frame[0] == At ^ UA)
                {
                    estado = BCC_RCV;
                    printf("State %d - BCC\n", estado);
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
                    STOP = TRUE;
                    printf("State %d - valid UA received\n", estado);
                }
                else
                {
                    estado = START;
                }
                break;
            }
        }
    }
    estado = START;
    STOP = FALSE;

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    if (showStatistics)
    {
        printf("Statistics:\n");
        printf("Bytes written: %u\n", bytesWritten);
        printf("Bytes read: %u\n", bytesRead);
    }

    return close(fd);
}
