#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> //Including standard C library to better parse text file
//#include <conio.h>
//#include <windows.h>
#include "rs232.h"
#include "serial.h"
#include "RobotInit.h"  //Header for external file for robot initialisation

//#define DEBUG
#define bdrate 115200               /* 115200 baud */

#define MAXWIDTH 100     //Maximum writing width in mm



int NextWord[64];          //Array of ascii values comprising the next word to be translated into commands
int WordLength;         //Length of text in file to appropriately allocate TextInput array size
int WordPosition = 0;   //Current location of the start/end of a word
unsigned int TrailingSpaces;        //How many spaces are after the current word      




void SendCommands (char *buffer );
int TextParse(const int *TextArray, int TextLength,int *WordPosition, int *NextWord, int *WordLength,unsigned int *TrailingSpaces);
int GenerateGCode(const int *NextWord, int WordLength,unsigned int TrailingSpaces);

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

// Send the data to the robot - note in 'PC' mode you need to hit space twice
// as the dummy 'WaitForReply' has a getch() within the function.
void SendCommands (char *buffer )
{
   
    //printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    WaitForReply();
    Sleep(100); // Can omit this when using the writing robot but has minimal effect
}


int TextParse(const int *TextArray, int TextLength,int *WordPosition, int *NextWord, int *WordLength, unsigned int *TrailingSpaces)       //Function to seperate TextArray into words that can be processed into GCode commands
{
    if (*WordPosition >= TextLength)      //Check if end of text has been reached  
    {
        *WordLength = 0;
        *TrailingSpaces = 0;       
        return 0;  //Return no word / Failure
    }

    int count = 0;  //Counter to track how many ascii values have been counted, therefore the word length
    *TrailingSpaces = 0;    //Reseting trailing spaces for each word

    if (TextArray[*WordPosition] == 13)     //If CR is in word
    { 
    NextWord[count++] = 13; //Store CR
    (*WordPosition)++;
    if (*WordPosition < TextLength && TextArray[*WordPosition] == 10) //If LF follows
    { 
        NextWord[count++] = 10; //Store LF
        (*WordPosition)++;
    }
    *WordLength = count;    
    return 1; //Return new word / Success
    }
    else if (TextArray[*WordPosition] == 10) //If just LF is in word
    { 
    NextWord[count++] = 10; //Store LF
    (*WordPosition)++;
    *WordLength = count;
    return 1;   //Return new word / Success
    }

    while (*WordPosition < TextLength &&       //If ascii value is part of a word process it  
           TextArray[*WordPosition] != 32 &&   //Space
           TextArray[*WordPosition] != 13 &&   //CR
           TextArray[*WordPosition] != 10)     //LF
    {   
        NextWord[count++] = TextArray[*WordPosition];       //Saving TextArray value at that point to the NextWord array
        (*WordPosition)++;
    }
    
    while (*WordPosition < TextLength && TextArray[*WordPosition] == 32)    //Count the spaces left after a word
    {
        (*TrailingSpaces)++;
        (*WordPosition)++;
    }


    if (*WordPosition < TextLength &&     //Skip past special characters after they have been identified earlier  
        (TextArray[*WordPosition] == 32 ||
         TextArray[*WordPosition] == 13 ||
         TextArray[*WordPosition] == 10)) 
    {
        (*WordPosition)++;
    }

    *WordLength = count;        //Set WordLength to the amount of ascii values processed in current word
    return 1;  //Return new word / Success
}

int GenerateGCode(const int *NextWord, int WordLength, unsigned int TrailingSpaces)
{
    char buffer[128];

  
    if (NextWord[0] == 13 || NextWord[0] == 10)        //Processing new line / return carriage ascii values
    {
        
        XOffset = 0;                //Move X offset back to the origin (return carriage)
        YOffset -= LineSpacing;    //Move Y down a line
        return 1; //Return Success
    }

    
    int WordWidth = 0;
    for (int i = 0; i < WordLength; i++)        //Iterate across word
    {
        int ascii = NextWord[i];
        if (ascii < 0 || ascii >= MAXCHARS) continue;       //Error handling invalid inputs
        struct FontData letter = FontSet[ascii];            //Pulls structure data for the current letter and gives it a local label "letter"
        if (letter.num_movements > 0)
        {
            WordWidth += letter.movements[letter.num_movements - 1].x;       //Width based on position of the last movement
        }
    }

    
    if (XOffset + WordWidth > MAXWIDTH)     //If word exceeds width limit
    {
        
        XOffset = 0;        //Send carriage back                
        YOffset -= LineSpacing;  // negative Y direction per spec
    }

    
    for (int i = 0; i < WordLength; i++)        //Generate G-code for each letter in the word
    {
        int ascii = NextWord[i];
        if (ascii < 0 || ascii >= MAXCHARS) continue;       //Error handling invalid inputs

        struct FontData letter = FontSet[ascii];    //Pulls structure data for the current letter and gives it a local label "letter"

        for (unsigned int j = 0; j < letter.num_movements; j++)     //Iterate through each movement for the ascii value
        {
            struct Instructions move = letter.movements[j];
            if ((XOffset + move.x)>= MAXWIDTH)
            {
                XOffset = 0;        //Send carriage back                
                YOffset -= LineSpacing;  // negative Y direction per spec
            }

            int TargetX = XOffset + move.x;     //Set X coordinate based on font instruction and continuing offset
            int TargetY = YOffset + move.y;     //Set Y coordinate based on font instruction and continuing offset

            if (move.pen == 1)      //If pen down is specified in font
            {
                sprintf(buffer, "S1000 G1 X%d Y%d\n", TargetX, TargetY);        //Set spindle on and movement to steady while moving to position
            } else {
                sprintf(buffer, "S0 G0 X%d Y%d\n", TargetX, TargetY);           //Set spindle off and movement to rapid while moving to position
            }
            SendCommands(buffer);   //Send Commands
        }

        
        if (letter.num_movements > 0)       //If there are movements
        {
            XOffset += letter.movements[letter.num_movements - 1].x;        //Update XOffset for next letter
        }
    }
    struct FontData Space = FontSet[32];    //Pulling the movement data of space as it has been scaled 
    XOffset += TrailingSpaces*Space.movements[0].x;     //Increasing the offset by the spaces between words
    return 1;       //Return Success
}


