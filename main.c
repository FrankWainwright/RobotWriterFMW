#include <stdio.h>
#include <stdlib.h>
//#include <conio.h>
//#include <windows.h>
#include "rs232.h"
#include "serial.h"

#define bdrate 115200               /* 115200 baud */
#define MAXMOVEMENTS 50     // upper bound for movements per character
#define MAXCHARS     128    // ASCII range (0–127)

struct Instructions {
    int x;     // X offset from origin
    int y;     // Y offset from origin
    int pen;   // Pen state: 0 = up, 1 = down
};

struct FontData {
    int ascii_code;                   // ASCII value of the character
    int num_movements;                // Number of movements
    struct Instructions movements[MAXMOVEMENTS];  // Array of movements
};
struct FontData FontSet[MAXCHARS];
int FontRead(const char *fontfilename) {
    FILE *fontfile = fopen(fontfilename, "r");
    if (!fontfile) {
        printf("Error opening file: %s\n", fontfilename);
        return 0;
    }

    int marker, ascii, n;
    int count = 0;

    // Loop until EOF
    while (fscanf(fontfile, "%d %d %d", &marker, &ascii, &n) == 3) {
        if (marker != 999) {
            // ignore lines that do not contain 999 for defining new character
            continue;
        }

        if (count >= MAXCHARS) {
            printf("Warning: exceeded MAXCHARS limit.\n");
            break;
        }

        FontSet[count].ascii_code = ascii;
        FontSet[count].num_movements = n;

        for (int i = 0; i < n && i < MAXMOVEMENTS; i++) {
            fscanf(fontfile, "%d %d %d",
                   &FontSet[count].movements[i].x,
                   &FontSet[count].movements[i].y,
                   &FontSet[count].movements[i].pen);
        }

        count++;
    }

    fclose(fontfile);
    return 1;  // return success
}

int Initialisation() {
    int Success = FontRead("SingleStrokeFont.txt");
    if (Success)
    {
        printf("Font File Processed");
        return 1;
    }
    else
    {
        printf("Failed to read Font File");
        return 0;
    }
    
}
void SendCommands (char *buffer );

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

    Initialisation(); // Initialse program for font data
    printf("Character: H (ASCII %d)\n",FontSet[72].ascii_code );
            printf("Number of movements: %d\n", FontSet[72].num_movements);

            for (int j = 0; j < FontSet[72].num_movements; j++) {
                printf("  Movement %d: x=%d, y=%d, pen=%d\n",
                       j,
                       FontSet[72].movements[j].x,
                       FontSet[72].movements[j].y,
                       FontSet[72].movements[j].pen);
            }

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


    // These are sample commands to draw out some information - these are the ones you will be generating.
    sprintf (buffer, "G0 X-13.41849 Y0.000\n");
    SendCommands(buffer);
    sprintf (buffer, "S1000\n");
    SendCommands(buffer);
    sprintf (buffer, "G1 X-13.41849 Y-4.28041\n");
    SendCommands(buffer);
    sprintf (buffer, "G1 X-13.41849 Y0.0000\n");
    SendCommands(buffer);
    sprintf (buffer, "G1 X-13.41089 Y4.28041\n");
    SendCommands(buffer);
    sprintf (buffer, "S0\n");
    SendCommands(buffer);
    sprintf (buffer, "G0 X-7.17524 Y0\n");
    SendCommands(buffer);
    sprintf (buffer, "S1000\n");
    SendCommands(buffer);
    sprintf (buffer, "G0 X0 Y0\n");
    SendCommands(buffer);

    // Before we exit the program we need to close the COM port
    CloseRS232Port();
    printf("Com port now closed\n");

    return (0);
}

// Send the data to the robot - note in 'PC' mode you need to hit space twice
// as the dummy 'WaitForReply' has a getch() within the function.
void SendCommands (char *buffer )
{
    // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    WaitForReply();
    Sleep(100); // Can omit this when using the writing robot but has minimal effect
    // getch(); // Omit this once basic testing with emulator has taken place
}
