#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> //Including standard C library to better parse text file
//#include <conio.h>
//#include <windows.h>
#include "rs232.h"
#include "serial.h"
#include "RobotInit.h"  //Header for external file for robot initialisation
#include "RobotCommand.h"   //Header for external file for robot commands (functions used inside loop)

//#define DEBUG
#define bdrate 115200               /* 115200 baud */
      


int main()
{

    //char mode[]= {'8','N','1',0};
    char buffer[100];

    // If we cannot open the port then give up immediately
    if ( CanRS232PortBeOpened() == -1 )
    {
        printf ("\nUnable to open the COM port (specified in serial.h) ");
        exit (0);
    }

    // Time to wake up the robot
    printf ("\nAbout to wake up the robot\n");

    // We do this by sending a new-line
    sprintf (buffer, "\n");
     // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    Sleep(100);

    Initialisation("SingleStrokeFont.txt", "Test.txt");         //Initialse program for font data
    // This is a special case - we wait  until we see a dollar ($)
    WaitForDollar();

    printf ("\nThe robot is now ready to draw\n");

        //These commands get the robot into 'ready to draw mode' and need to be sent before any writing commands
    sprintf (buffer, "G1 X0 Y0 F1000\n");
    SendCommands(buffer);
    sprintf (buffer, "M3\n");
    SendCommands(buffer);
    sprintf (buffer, "S0\n");
    SendCommands(buffer);

    #ifdef DEBUG        //Debug value tests
        printf("Character: H (ASCII %u)\n",FontSet[72].ascii_code );
        printf("Number of movements: %u\n", FontSet[72].num_movements);

            for (unsigned int j = 0; j < FontSet[72].num_movements; j++) 
            {
                printf("  Movement %u: x=%u, y=%u, pen=%u\n",
                       j,
                       FontSet[72].movements[j].x,
                       FontSet[72].movements[j].y,
                       FontSet[72].movements[j].pen);
            }
        printf("Read %d ASCII values:\n", TextLength);
        for (int i = 0; i < TextLength; i++)
        {
            printf("%d ", TextInput[i]);
        }
    #endif
    
    while (TextParse(TextInput, TextLength, &WordPosition, NextWord, &WordLength,&TrailingSpaces)) 
    {
    
        if (WordLength > 0)     //If TextParse returns 1 when a word or CR/LF was successfully extracted
        {
        
            if (GenerateGCode(NextWord, WordLength,TrailingSpaces))        //Generate G-code for this word and send commands
            {
            //Show debug info
            #ifdef DEBUG
            printf("Processed word of length %d at position %d\n", WordLength, WordPosition);
            #endif
            }
            else 
            {
            printf("Error generating G-code for word at position %d\n", WordPosition);
            }
        }
    }

// After finishing all words, ensure pen is up and return to origin
sprintf(buffer, "S0 G0 X0 Y0\n");   // Pen up, move to (0,0)
SendCommands(buffer);

    // Before we exit the program we need to close the COM port
    CloseRS232Port();
    printf("Com port now closed\n");
    free(TextInput);        //Free up allocated memory
    TextInput = NULL;       //Remove pointer address
    return (0);
}




