#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> //Including standard C library to better parse text file
#include "rs232.h"
#include "serial.h"
#include "RobotInit.h"  //Header for external file for robot initialisation
#include "RobotCommand.h"   //Header for external file for robot commands (functions used inside loop)

int NextWord[64];          //Array of ascii values comprising the next word to be translated into commands
int WordLength;         //Length of word   
unsigned int TrailingSpaces;        //How many spaces are after the current word
FILE *TextFile;

// Send the data to the robot - note in 'PC' mode you need to hit space twice
// as the dummy 'WaitForReply' has a getch() within the function.
void SendCommands (char *buffer )
{
   
    //printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    WaitForReply();
    Sleep(100); // Can omit this when using the writing robot but has minimal effect
}
int TextReadWord(const char *TextFileName,FILE **TextFile,int *NextWord,int *WordLength,unsigned int *TrailingSpaces)       //Function to read text.txt word by word and write the word to the Nextword array
{
    if (*TextFile == NULL) //If file pointer is not yet pointing to a filestream
    {
        *TextFile = fopen(TextFileName, "r");       //Opens file stream and assigns to pointer
        if (!*TextFile)     //If text file could not be opened
        {
            printf("Failed to read file \n");
            exit(1);        //End program
        }
    }

    int ch;     //Temporary character storage
    int count = 0;      //Temporary counter
    *TrailingSpaces = 0;    //Resetting trailing spaces for new word

   
    while ((ch = fgetc(*TextFile)) == 32)       //While the character is space
    {
        (*TrailingSpaces)=0;        //Increment spaces
    }

    if (ch == EOF)      //If character is end of file
    {
        fclose(*TextFile);      //Close file stream
        *TextFile = NULL;       //Unpoint textfile pointer
        *WordLength = 0;        //Set no word length
        *TrailingSpaces = 0;    //Set no spaces
        return 0; //End of file, no more words
    }


    if (ch == 13)       //If character is CR
    {
        NextWord[count++] = 13;     //Add CR value to next word array
        ch = fgetc(*TextFile);      //Look at next character
        if (ch == 10)       //If LF follows from CR
        { 
            NextWord[count++] = 10; //Add LF value to next word array
        } else if (ch != EOF)       //If accidentally reached the end of file with the lookup
        {
            ungetc(ch, *TextFile);      //Remove character increment so next function call finds end of file and handles
        }
        *WordLength = count;        
        return 1;       //Return success / new word 
    }
    else if (ch == 10)      //If character is LF
     { 
        NextWord[count++] = 10;     //Add LF value to next word array
        *WordLength = count;
        return 1;       //Return success / new word 
    }


    while (ch != EOF && ch != 32 && ch != 13 && ch != 10)       //Collect characters until space/CR/LF/EOF
    {
        NextWord[count++] = ch;     //Add ch value to next word array
        ch = fgetc(*TextFile);      //Look at next character
    }

    
    while (ch == 32)        //While characters are space
    {
        (*TrailingSpaces)++;        //Increment trailing spaces
        ch = fgetc(*TextFile);      //Look at next character
    }

    if (ch != EOF && ch != 13 && ch != 10)      //If character is not CR/LF/EOF
    {
        ungetc(ch, *TextFile);      //Unget next character
    } else if (ch == 13 || ch == 10) 
    {
        ungetc(ch, *TextFile);        //Leave CR/LF for next call
    }

    *WordLength = count;
    return 1; //Success / new word
}

int GenerateGCode( int *NextWord, int WordLength, unsigned int TrailingSpaces)      //Function for generating GCode commands for the words passed to it
{
    char buffer[128];
    if (NextWord[0] == 13 || NextWord[0] == 10)        //Processing new line / return carriage ascii values
    {
        
        XOffset = 0;                //Move X offset back to the origin (return carriage)
        YOffset -= LineSpacing;    //Move Y down a line
        sprintf(buffer, "S0 G0 X%f Y%f\n", XOffset, YOffset);
        SendCommands(buffer);
        return 1; //Return Success
    }

    
    double WordWidth = 0;
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
        if (ascii < 0 || ascii >= MAXCHARS)
        {
            continue;      //Error handling invalid inputs
        }
        else
        {
            printf("Could not generate commands /N");
            return 0;
        }
        struct FontData letter = FontSet[ascii];    //Pulls structure data for the current letter and gives it a local label "letter"
        unsigned int Down = 0; 
        for (unsigned int j = 0; j < letter.num_movements; j++)     //Iterate through each movement for the ascii value
        {
            struct Instructions move = letter.movements[j];
            if ((XOffset + move.x)>= MAXWIDTH)
            {
                XOffset = 0;        //Send carriage back                
                YOffset -= LineSpacing;  // negative Y direction per spec
            }
                 
            double TargetX = XOffset + move.x;     //Set X coordinate based on font instruction and continuing offset
            double TargetY = YOffset + move.y;     //Set Y coordinate based on font instruction and continuing offset    
            if (move.pen == 1)      //If pen down is specified in font
            {
                if(!Down)
                {
                    Down = 1;
                    sprintf(buffer, "S1000 G1 X%f Y%f\n", TargetX, TargetY);        //Set spindle on and movement to steady while moving to position
                }
                else
                {
                   sprintf(buffer, "G1 X%f Y%f\n", TargetX, TargetY);        //Set movement to steady while moving to position 
                }
            } 
            else 
            {
                Down = 0;
                sprintf(buffer, "S0 G0 X%f Y%f\n", TargetX, TargetY);           //Set spindle off and movement to rapid while moving to position
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
