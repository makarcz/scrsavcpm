/*
 *----------------------------------------------------------------------------
 * Project: Screen Saver
 * Puspose: Print asterisks at random locations until a key is pressed.
 * Author:  Copyright (C) Marek Karcz 2019. All rights reserved.
 * License: Free for personal use.
 * File:    scrsav.c
 * OS:      Commodore 128 CP/M
 * Compilr: Aztec C.
 *
 * TO DO:
 *
 * REVISION HISTORY:
 *
 * 1/3/2020 - The coordinates of time display change every 100 seconds to
 *            prevent CRT burnout.
 *            Asterisk '*' replaced with a dot '.' symbol.
 * 4/3/2020 - Screen is cleared more often. Added argument 'blank'.
 * 5/8/2020 - Added screen blanking at exit.
 *----------------------------------------------------------------------------
 */

#include "libc.h"

#define ESC 27
#define NL  10
#define CR  13
#define BS  8
#define DEL 20
#define PWL 7
#define SCB 0xfe00
#define HR  (SCB+0x5a)
#define MIN (SCB+0x5b)
#define SCS (SCB+0x5c)
#define DT  (SCB+0x58)
#define DOT '.'
long next = 1;

/*
 * Aztec C 1.05 doesn't have random generator in the library of functions.
 * Here is something I found on the internet.
 * Will return value 0 - 32767.
 */
unsigned rand()
{
   next = next * 1103515245 + 12345;
   return (unsigned)(next / 65536) % 32768;
}

/*
 * Initialize random number generator with a new seed.
 */
srand(seed)
   unsigned seed;
{
   next = seed;
}

/*
 * A simple delay function.
 */
StopFor(dly)
   int dly;
{
   int n;

   n = dly;
   while(n > 0) {n--;}
}

/*
 * Clear the screen (ADM31).
 */
BlankScr()
{
   putchar(ESC);
   putchar('*');
}

/*
 * Cursor positioning (absolute cursor addressing) for ADM31.
 */
GotoXY(col, row)
   char col, row;
{
   putchar(ESC);
   putchar('=');
   putchar(' '+row);
   putchar(' '+col);  
}

/*
 * Non-blocking character input.
 * Return 0 if no character is waiting.
 * return a character without echoing if one is waiting.
 */
int KeyPress()
{
   return (bdos (0x06, 0xFF)); 
}

/*
 * Get password from keyboard. Mask entered characters as '*'.
 * BS/DEL previously entered character: CTRL-H
 * Finish entering: RETURN or ESC
 * Password is not case sensitive. (all characters converted to lowercase)
 * str - string buffer
 * len - maximum length of the passphrase
 */
char *GetPwd(str, len)
   char *str;
   int  len;
{
   int i = 0;
   char c;

   while (KeyPress()); /* clear keyboard buffer */
   while (1) {
   
      c = KeyPress();
      if (c == 0) continue;
      if ((c == BS || c == DEL) && i > 0) {

         putchar(BS);
         putchar(' ');
         putchar(BS);
         i--;
         continue;
      }      
      if (c == NL || c == CR || c == ESC) {

         str[i] = 0;
         break;
      }
      if (c >= 32 && c < 127) {

         if (i < len-1) {

            putchar('*');
            str[i] = tolower(c);
            i++;
         }
      }
   }
   str[len-1] = 0;

   return (str);
}

/*
 * Get date / time (for rand seed initialization).
 * Returns # of seconds (packed BCD), tptr - address of time stamp.
 * The format of time stamp:
 *    DW day ;Day 1 is Jan 1-st 1978
 *    DB hr  ;Packed BCD
 *    DB min ;Packed BCD
 */
int GetDtTm(tptr)
   int *tptr;
{
   return (bdos (0x69, tptr)); 
}

/*
 * Get time string in format hh:mm:ss
 */
char *GetTimeStr()
{
   long dtm;
   char buf[10], hr[3], min[3];
   int sec;
   static char ret[10];

   clear(buf, 10, 0);
   clear(ret, 10, 0);
   hr[2] = 0;
   min[2] = 0;
   sec = GetDtTm(&dtm);
   sprintf(buf, "%08x", dtm >> 16);
   strncpy(hr, buf+6, 2);
   strncpy(min, buf+4, 2);
   sprintf(ret, "%s:%s:%02x", hr, min, sec);      
   
   return (ret);
}

/*
 * Convert string str to lower case.
 */
char *StrToLower(str)
   char *str;
{
   char *ps;

   ps = str;

   while (*ps != 0) {
      
      *ps = tolower(*ps);
      ps++;
   }

   return str;
}

/*
 * Prompt user for passphrase, compare against pwd, return value > 0 if
 * password entered by user matches, return 0 otherwise.
 */
int CheckPwd(pwd)
   char *pwd;
{
   char buf[PWL];
   int ret = 0;

   clear(buf, PWL, 0);
   GotoXY(0, 23);
   printf("Password:");
   GetPwd(buf, PWL);
   StrToLower(buf);
   StrToLower(pwd);  
   if (0 == strcmp(buf, pwd)) {
      ret = 1;
   } else {

      GotoXY(0, 23);     
      printf("Invalid password.");
      StopFor(30000);
   }

   return (ret);
}



/* ------------------------------ MAIN LOOP -------------------------------- */

main (argc, argv)
   int argc;
   char *argv[];
{
   unsigned row, col, trow;
   int ct, sec, blank = 0;
   long dtm = 0;
   char c = DOT;
   char pwd[PWL];

   printf("Screen Saver (C) Marek Karcz 2019. All rights reserved.\n");
   
   if (argc > 1) { /* scan command line for arguments / options */

      for (ct = 1; ct < argc; ct++) {
      
        StrToLower(argv[ct]);
        printf("Argument: %s\n", argv[ct]);
        if (0 == strcmp(argv[ct], "lock")) {

           while(KeyPress()); /* clear waiting characters */
           clear(pwd, 7, 0);
           printf("Enter the passphrase that will be used to unlock ");
           printf("the screen.\n");
           printf("(CTRL-H to BS/DEL, RETURN or ESC to end, 1-6 characters)\n");
           printf("Password:");
           GetPwd(pwd, PWL);
           putchar(NL);

        } else if (0 == strcmp(argv[ct], "blank")) {

           blank = 1;
        }
      } /* for (ct = 1; ... */
   } /* if (argc > 1) */
   
   printf("\nProgram runs until a key is pressed.\n");
   printf("If 'lock' is provided as argument, program will ask user to ");
   printf("establish\na 1-6 characters long passphrase to be used to unlock ");
   printf("the screen.\n");
   printf("If 'blank' is provided as argument, the screen will be blanked\n");
   printf("for the duration of the run instead of displaying time and random");
   printf("\ndots / stars.\n");
   StopFor(32000);
   while(KeyPress()); /* clear waiting characters */
   sec = GetDtTm(dtm);
   srand(dtm);   
   ct = row = col = trow = 0;

   do {

      BlankScr();

      while (!KeyPress()) {

         if (0 < blank) continue; /* skip dots and time if blank mode */
         /* display or clear a dot at random location */
         row = rand() / 1366 - 1;
         col = rand() / 410 - 1;
         GotoXY(col, row);
         putchar(c);
         c = ((c == DOT) ? ' ' : DOT);
         ct++;
         if (0 == (ct % 100)) { /* move time disp. to prevent burn-in */
            GotoXY(0, trow);
            printf("         ");
            trow++;
            if (23 < trow) trow = 0;
         }
         if (ct > 500) { /* clear screen, reset counter and RND gen. */

            ct = 0;
            srand(dtm);
            BlankScr();
         }
         /* display time in 1-st column and currently designated row */
         sec = GetDtTm(&dtm);
         GotoXY(0, trow);
         printf("%s", GetTimeStr());
         while (sec == GetDtTm(&dtm)) { /* stop for 1 second */
         
            StopFor(1000);
         }
      } /* while (!KeyPress()) */

   } while (strlen(pwd) > 0 && CheckPwd(pwd) <= 0);

   BlankScr();

   return;
}