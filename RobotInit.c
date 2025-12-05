#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> //Including standard C library to better parse text file
#include "RobotInit.h"

struct Instructions     //Structure of pen movement instructions
{   
    double x;     //X position from origin
    double y;     //Y position from origin
    unsigned int pen;   //Pen state: 0 = up, 1 = down
};

struct FontData     //Structure of the font header with nested structure of instructions      
 {
    unsigned int ascii_code;                        //ASCII value of the character
    unsigned int num_movements;                     //Number of movements
    struct Instructions movements[MAXMOVEMENTS];    //Array of movements
};
struct FontData FontSet[MAXCHARS];      //Populating the structure with spaces for every ascii character
double XOffset = 0;        //Offset applied from origin in X direction
double YOffset = 0;        //Offset applied from origin in Y direction
unsigned int LineSpacing = 5;   //Spacing between successive lines in mm

int FontRead(const char *fontfilename, unsigned int FontHeight) //Function to load fontfile, and populate structure 
{
    FILE *fontfile = fopen(fontfilename, "r"); //Opens specified file
    if (!fontfile)
     {
        return 0; //Return Failure
    }

    int marker, ascii, n; //Initialising values required to read file
    int count = 0;

    while (fscanf(fontfile, "%d %d %d", &marker, &ascii, &n) == 3) //Continues to read through file until its end or until its formating is malformed
    {
        if (marker != 999) {
            // ignore lines that do not contain 999 for defining new character
            continue;
        }

        if (count >= MAXCHARS) //Checking if more definitions than specified ascii set (standard 128)
         {
            printf("Warning: exceeded MAXCHARS limit.\n");
            break;
        }

        FontSet[ascii].ascii_code = ascii;  //Setting ascii identifier in header of structure from header of font character definition
        FontSet[ascii].num_movements = n;   //Setting number of movements in header of structure from header of font character definition

        for (int i = 0; i < n && i < MAXMOVEMENTS; i++) //Iterate through movements until specified movements in font header are finished or maximum limit is reached
        {
            int BeforeScaleY = 0; //Initilaise a temporary variable so Y commands can be modified by the user specified font height
            int BeforeScaleX = 0; //Initilaise a temporary variable so X commands can be modified by the user specified font height
            fscanf(fontfile, "%d %d %u",
                   &BeforeScaleX,
                   &BeforeScaleY,
                   &FontSet[ascii].movements[i].pen); //Scan file for the three variables specified in the formatting of the file and assign them to the structure at that step
                   FontSet[ascii].movements[i].x = (((double)FontHeight/ 18.0) * BeforeScaleX);  //Modifies X values by the specified height ensuring non integer values are rounded up
                   FontSet[ascii].movements[i].y = (((double)FontHeight/ 18.0) * BeforeScaleY);  //Modifies Y values by the specified height ensuring non integer values are rounded up
        }

        count++;
    }

    fclose(fontfile);
    return 1;  // return success
}

int Initialisation(const char *FontFileName) //Function to handle all font related operations at the start of the program and trap errors
{
    unsigned int FontHeight = 0;        //User specified value for the height they want letters to be written at
    char input[32];     //Buffer for user input, ensuring it doesnt break input loop
   for (;;)     //Run forever until correct input is given
   {
        printf("Please input a height (in mm) between 4 and 10 for the text to be drawn at: ");
        if (fgets(input, sizeof(input), stdin))     //Process user input
         {
            if (sscanf(input, "%u", &FontHeight) == 1)  //Test if the value contains an unsigned integer
            {
                if (FontHeight >= 4 && FontHeight <= 10)    //Test if the integer value is within specified limits
                {
                    break;  //End infinite loop 
                }
            }
        }
        printf("Invalid input. Please enter a whole number between 4 and 10.\n");
    }
    YOffset -= FontHeight;       //Ensure text starts below Y 0
    LineSpacing += FontHeight;      //Set Linespacing to be the distance between lines and the height of a line

    if (FontRead(FontFileName,FontHeight))      //Read font instructions and populate structure with it
    {
        printf("Font File Processed \n");
        
    }
    else
    {
        printf("Failed to read Font File \n");
        exit(1);
    }
    return 1;   //Return Success
}
