/* HP25 type calculator 

Debugged together by  C Lombard

February 1991, October 1994

The idea was to replace my old HP25 (whose batteries are now
always flat) with something just as convenient and with the
same keystrokes on the numeric pad. Also, I want to always
see what's in the registers and stack. No mice please, they
make me scream.

Accuracy: >12 digits (sometimes), actually C-double precision
types are used. You can't program the calculator because it
would be silly to do so. Rather use BASIC. 

On my linux setup, with a standard IBM enhanced keyboard, the
keypad works fine with Num Lock on. You might have to fix the 
get_ch function to do some character translation.

If you want to hack this to change the command definitions
or add features, or do anything else: be my guest.
Just don't ask me to explain the logic, I don't know anymore. 
What I do remember is that no commands can start with 'e' or
_EEX character since these characters signal an exponent. 

If you divide by zero or pull the root of a negative number,
your house will cave in - serves you right!

For unix version: link with -lm -lncurses, i.e.
gcc -O rpn.c -o rpn -lm -lncurses    
seems to do the trick.

***  To display this file nicely - set tab every 2 chars. ***
*/

#ifdef __TURBOC__
#include <conio.h>
#endif

#ifdef __unix__
#include <ncurses/curses.h>
#endif

#ifdef __linux__
#ifdef __GNUC__
#define M_PI (3.14159265358979323846264338327950288)
#endif
/*
 #define stpcpy(dest,src) ((strcpy((dest),(src)))+strlen(src))
*/
#endif


#ifdef _MSC_VER
//For Microsoft Visual Studio.NET
#pragma unmanaged
#include<windows.h>
#include <conio.h>
#define stpcpy(dest,src) ((strcpy((dest),(src)))+strlen(src))
#define _USE_MATH_DEFINES  /* must be defined for M_PI */
#endif

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//End of defines for windows 


/* keyboard - function definitions 
NB unix  character translations are done in get_ch() */
#define _PERC   '%'
#define _ENTER  '\r'
#define _CHS    ','
#define _EEX    '^'
#define _min    '-'
#define _plus   '+'
#define _times  '*'
#define _SQRx   '\\'
#define _xSQR   ';'
#define _ytox   ':'
#define _ABS    '|'
#define _div    '/'
#define _FACT   '!'
#define _RUBOUT '\b'

#define SMALL 1.0e-20       /* a very small number, tests for 0.0 */

/* utility keys */
#define _QUIT   'q'     /* quit the program */
#define _HELP   '?'     /* show help screen */

char d_format[10] = "%30.3g";   /*const format variable */
char disp_format[8] = "eng3";   /* format flag */

char help_screen1[] = "\n"
"    FUNCTION           KEY          FUNCTION           KEY\n"
"    enter              'Enter'      +,*,-,/            '+,*,-,/'\n"
"    change sign        ','          square root x      '\\'\n"
"    x^2                ';'          y^x                ':'\n"
"    |x|                '|'          x percent of y     '%'\n"
"    factorial x        '!'          roll down          'rd'\n"
"    1/x                'xi'         last x             'lx'\n"
"    change x and y     'xy'         clear x            'cx'\n"
"    clear stack        'cs'         clear regs         'cr'\n"
"    pi                 'pi'         2*pi               'tp'\n"
"    sin x              'si'         sinh x             'sh'\n"
"    cos x              'co'         cosh x             'ch'\n"
"    tan x              'ta'         tanh x             'th'\n"
"    asin x             'as'         asinh x            'hs'\n"
"    acos x             'ac'         acosh x            'hc'\n"
"    atan x             'at'         atanh x            'ht'\n"
"    degrees mode       'dg'         radians mode       'ra'\n"
"    normal distrib.    'nd'         inv. normal distr. 'id'\n"
"    normal density     'no' \n\n"
"       For more - any key   q - quits";

char help_screen2[] = "\n"
"    FUNCTION           KEY          FUNCTION           KEY\n"
"    exp x              'xp'         ln x               'ln'\n"
"    10^x               'tx'         log x              'lg'\n"
"    frac x             'fr'         int x              'in'\n"
"    H.MMSS -> H        'hd'         H -> H.MMSS        'dh'\n"
"    store reg(n)       'sn'         recall reg(n)      'rn'\n"
"    reg(n) +*-/ x      's+*-/n'\n"
"    polar convert      'pc'         rect. convert      'rc'\n"
"    fit line           'fl'         solve quad. eq.    'so'\n"
"    sum x,xy,y...      'sp'         subtract x,xy,y... 'sm'\n"
"    mean x,y           'mn'         std. dev.sx,sy,sxy 'sd'\n"
"    PV (Compounded)    'pv'         FV (Compounded)    'fv'\n"
"    i (Compound)       'ci'         PMT (Mortgage)     'pt'\n"
"    i (Mortgage)       'mi'\n"
"    FV (Ord. Annuity)  'fa'         PV (Ord.Annuity)   'pa'\n"
"    FV (Annuity due)   'fd'         PV (Annuity due)   'pd'\n"
"    disp. fixed(n)     'fxn'        disp. scien.(n)    'fsn'\n"
"    display eng.(n)    'fgn'        for 12 digits n=<space>\n"
"    QUIT program       'q' \n\n"
"  Scientific numbers may be entered as '1e-3' or '1.2^4'\n\n"
"      To return - any key   q - quits";


/* state flags */
char lift = 0;          /*true if stack must be lifted */
char min_exp = 0;       /*true if '-' in exponent, afterthought: also '+'.  */
double convert = 1;     /*rad-deg (grad is long dead) conversion factor */

/* windows positions */
char stackw_bottom = 23;    /* stack window lines from top */
char stackw_left = 25;      /* stack window characters from right */
char stackw_width = 60;     /* stack window width */
char regw_bottom = 14;      /* reg window lines from top */
char regw_left = 25;        /* reg window characters from right */
char regw_width = 60;       /* reg window width */
char echow_bottom = 24;     /* lines from top */
char echow_left = 65;       /* characters from left */
char echow_width = 15;      /* echo window width */
char flagw_bottom = 1;      /* flags */
char flagw_left = 55;




/* main variables */
double s_x, s_y, s_z, s_t;  /* stack variables */
double lastx;           /* last x variable */
double regs[10];        /* registers */

/* buffer */
char echo_buffer[16];       /* used for input echoing */

/*functions */
void update_stack (void);
void update_regs (void);
void update_flags (void);
void init (void);
void update (char);     /* update display- char = lift flag */
void clear_stack (char);    /* clear  window */
void clear_echo (void);     /* clear echo window */
void echo_line (void);      /* update echo line */
void clrscr_exit (int);     /* clear window and exit */
void show_help (void);      /* show help screen */
void xupdate (void);        /* update x register only */
char get_ch (void);     /* read a single character from keyboard */

double atanh (double);      /* atanh function */
double asinh (double);      /* asinh function */
double acosh (double);      /* acosh function */
double norm_dis (double);   /* normal distribution */
double errf (double);       /* error function */
double inerrf (double);     /* inverse error function */
double find_mortgage_i (void);  /* find mortgage interest by newton iteration */


int main ()
{
	/*variables   */
	char input,   count = 0;
	char second   = 0;
	double temp;

	/*initialize */
	init ();

	/*process input   characters */
	while ((input =   get_ch ()) != _QUIT)
	{
		if (input == _RUBOUT) /* backspace character */
		{
			count -= (count   > 0);
			echo_buffer[count] = '\0';
			clear_echo ();
		}
		else
		{
			echo_buffer[count] = input;
			echo_buffer[count +   1] = '\0';
			if (count <   echow_width - 1)
				count++;
		}

		echo_line ();     /* update echo line and refresh screen */

		/*check   for numeric input */
		if ((isdigit (input) ||   (input == '.') || (input == 'e')
			|| (input == _EEX)
			|| ((min_exp) && ((input ==  '-') || (input == '+')))
			|| (input == _RUBOUT)) && !(second))
			/* if you've ever seen random logic---this is it! */
		{           /* digit characters */
			min_exp   = 0;
			if (count == 1)
			{
				clear_stack   (stackw_bottom);    /* clear x window */
				if (lift)
				{
					s_t   = s_z;
					s_z   = s_y;
					s_y   = s_x;
					update (0);
				}
			}
			if (input == _EEX)
				echo_buffer[count - 1] = 'e';
			s_x   = atof (echo_buffer);
			if (echo_buffer[count -   1] == 'e')
			{
				min_exp   = 1;
				/*trap possible   following minus or plus */
				/* trap   initial exponent */
				if (count == 1)
				{
					echo_buffer[0] = '1';
					echo_buffer[1] = 'e';
					echo_buffer[2] = '\0';
					count =   2;
				}
			}
			if (echo_buffer[count -   1] == '-' || echo_buffer[count - 1] == '+')
			{
				min_exp   = 0;    /*reset -/+ flag */
			}
			xupdate   ();     /* update x register display only */
		}
		else
			/* not a digit */
		{
			if (count >   1 && !second)   /*must be beginning of operator, me thinks */
			{
				/* restart input buffer   */
				clear_echo ();
				count =   1;
				echo_buffer[0] = input;
				echo_buffer[1] = '\0';

				echo_line (); /* update echo line */

			}
			if (count == 1)   /* one character available */
				/* first check for single character operators */
			{
				switch (input)    /* do one key codes */
				{
				case _ENTER:
					{
						s_t = s_z;
						s_z = s_y;
						s_y = s_x;
						update (0);
						break;
					}
				case _plus:
					{
						lastx = s_x;
						s_x += s_y;
						s_y = s_z;
						s_z = s_t;
						update (1);
						break;
					}
				case _min:
					{
						lastx = s_x;
						s_x = s_y - s_x;
						s_y = s_z;
						s_z = s_t;
						update (1);
						break;
					}
				case _times:
					{
						lastx = s_x;
						s_x *= s_y;
						s_y = s_z;
						s_z = s_t;
						update (1);
						break;
					}
				case _div:
					{
						lastx = s_x;
						s_x = s_y / s_x;
						s_y = s_z;
						s_z = s_t;
						update (1);
						break;
					}
				case _PERC:
					{
						lastx = s_x;
						s_x = s_y * (s_x / 100.0);
						update (1);
						break;
					}
				case _CHS:
					{
						lastx = s_x;
						s_x = -s_x;
						update (0);
						break;
					}
				case _SQRx:
					{
						lastx = s_x;
						s_x = sqrt (s_x);
						update (1);
						break;
					}
				case _xSQR:
					{
						lastx = s_x;
						s_x *= s_x;
						update (1);
						break;
					}
				case _ABS:
					{
						lastx = s_x;
						s_x = fabs (s_x);
						update (0);
						break;
					}
				case _ytox:
					{
						lastx = s_x;
						s_x = pow (s_y, s_x);
						s_y = s_z;
						s_z = s_t;
						update (1);
						break;
					}
				case _HELP:
					{
						show_help ();
						break;
					}
				case _FACT:
					{
						lastx = s_x;
						temp = s_x - 1.0;
						if (s_x == 0.0)
							s_x   = 1.0;
						else
							while (temp   > 0.0)
							{
								s_x   *= (temp--);
							}
							update (0);
							break;
					}
					/* register   functions */
				case 'r':
					{     /* recall register */
						input = get_ch ();  /* check if register function */
						if (isdigit (input))    /* must be recall function */
						{
							s_t = s_z;
							s_z = s_y;
							s_y = s_x;
							s_x = regs[input - '0'];
							update (1);
						}
						else
						{
							ungetch (input);    /* replace next character in buffer */
							second = 1;
						}
						break;
					}
				case 's':
					{     /* store in register */
						input = get_ch ();  /* check if register function */
						if (isdigit (input))    /* must be store function */
						{
							regs[input - '0'] = s_x;
							update_regs ();
						}
						else
						{
							if (input == '+')
							{
								input = get_ch ();
								if (isdigit (input))
								{
									regs[input - '0'] += s_x;
									update_regs ();
								}
							}
							else
							{
								if (input == '-')
								{
									input = get_ch ();
									if (isdigit (input))
									{
										regs[input - '0'] -= s_x;
										update_regs ();
									}
								}
								else
								{
									if (input == '*')
									{
										input = get_ch ();
										if (isdigit (input))
										{
											regs[input - '0'] *= s_x;
											update_regs ();
										}
									}
									else
									{
										if (input == '/')
										{
											input = get_ch ();
											if (isdigit (input))
											{
												regs[input - '0'] /= s_x;
												update_regs ();
											}
										}
										else
										{     /* must be beginning of 2 character command */
											ungetch (input);
											second = 1;
										}
									}
								}
							}
						}
						break;
					}
				default:
					{
						second = 1;
						break;
					}     /* this is not a 1 character command */
				}
			}           /* must be a two character operator */
			if (!second)
				count = 0;      /* it was a one character command */
		}           /* so go get more cahracters */
		/*process two character   codes */
		if (count == 2 && second &&   input != _RUBOUT)
		{
			switch (echo_buffer[0])   /* check first character */
			{
			case 'r':       /* check double codes beginning with r */
				{
					switch (echo_buffer[1])     /* check second character */
					{
					case 'd':
						{
							temp = s_x;
							s_x   = s_y;
							s_y   = s_z;
							s_z   = s_t;
							s_t   = temp;
							update (1);
							break;
						}
					case 'a':
						{
							convert   = 1.0;
							update (0);
							break;
						}
					case 'c':
						{
							lastx =   s_x;
							s_x   = cos (s_y * convert) * lastx;
							s_y   = sin (s_y * convert) * lastx;
							update (1);
							break;
						}
					}
					break;
				}
			case 'x':       /*check double codes beginning with x */
				{
					switch (echo_buffer[1])
					{
					case 'y':
						{
							temp = s_x;
							s_x   = s_y;
							s_y   = temp;
							update (1);
							break;
						}
					case 'i':
						{
							lastx =   s_x;
							s_x   = 1.0 / s_x;
							update (1);
							break;
						}
					case 'p':
						{
							lastx =   s_x;
							s_x   = exp (s_x);
							update (1);
							break;
						}

					}
					break;
				}
			case 'c':       /*check double codes beginning with c */
				{
					switch (echo_buffer[1])
					{
					case 'x':
						{
							lastx =   s_x;
							s_x   = 0.0;
							update (0);
							break;
						}
					case 's':
						{
							lastx =   s_x;
							s_x   = s_y = s_z = s_t = 0.0;
							update (0);
							break;
						}
					case 'o':
						{
							lastx =   s_x;
							s_x   = cos (s_x * convert);
							update (1);
							break;
						}
					case 'r':
						{
							char i;
							for   (i = 0; i < 10; regs[i++] = 0.0);
							update_regs   ();
							break;
						}
					case 'h':
						{
							lastx =   s_x;
							s_x   = cosh (s_x * convert);
							update (1);
							break;
						}
					case 'i':
						{
							regs[0]   = 100.0*(pow ((regs[2]  / regs[1]), 1.0 / regs[4]) - 1.0);
							update_regs   ();
							break;
						}
					}
					break;
				}
			case 'l':       /*check double codes beginning with l */
				{
					switch (echo_buffer[1])
					{
					case 'n':
						{
							lastx =   s_x;
							s_x   = log (s_x);
							update (1);
							break;
						}
					case 'g':
						{
							lastx =   s_x;
							s_x   = log10 (s_x);
							update (1);
							break;
						}
					case 'x':
						{
							temp = s_x;
							s_t   = s_z;
							s_z   = s_y;
							s_y   = s_x;
							s_x   = lastx;
							lastx =   temp;
							update (0);
							break;
						}
					}
					break;
				}
			case 'p':       /*check double codes beginning with p */
				{
					switch (echo_buffer[1])
					{
					case 'a':  /* Present value of ordinary annuity (payment at end of period)*/
						{
							regs[1]   = regs[3]*( 1.0-pow( (1.0+(regs[0]/100.0)),-(regs[4]) ) )/(regs[0]/100.0);
							update_regs   ();
							break;
						}

					case 'd':  /* Present value of annuity due (payment at start of period)*/
						{
							regs[1]   = regs[3]*(1.0-pow( (1.0+(regs[0]/100.0)),-(regs[4]) ) )/(regs[0]/100.0)*(1.0+(regs[0]/100.0));
							update_regs   ();
							break;
						}
					case 'i':
						{
							lastx =   s_x;
							s_t   = s_z;
							s_z   = s_y;
							s_y   = s_x;
							s_x   = M_PI;
							update (1);
							break;
						}
					case 'c':
						{
							lastx =   s_x;
							s_x   = sqrt (s_x * s_x + s_y * s_y);
							s_y   = atan2 (s_y, lastx) / convert;
							update (1);
							break;
						}
					case 'v':
						{
							regs[1]   = regs[2] * pow ((1.0 + (regs[0]/100.0)), -regs[4]);
							update_regs   ();
							break;
						}
					case 't':
						{
							regs[3]   = regs[1] * (regs[0]/100.0)/(1.0-pow( (1.0+(regs[0]/100.0)),-regs[4]) );
							update_regs   ();
							break;
						}
					}
					break;
				}
			case 'f':       /* check double codes beginning with f */
				{
					switch (echo_buffer[1])
					{
					case 'v':
						{
							regs[2]   = regs[1] * pow ((1.0 + (regs[0]/100.0)), regs[4]);
							update_regs   ();
							break;
						}
					case 'r':
						{
							lastx =   s_x;
							s_x   = s_x - (double) ((long) s_x);
							update (1);
							break;
						}
					case 'g': /*g type display format */
						{
							char *format = (char *)   d_format;
							format = stpcpy   (format, "%30.\0");
							if (isdigit   (input = get_ch ()))
							{
								*(format) =   input;
								*(format + 1) =   'g';
								*(format + 2) =   '\0';
								strcpy (disp_format, "eng");
								*(disp_format +   3) = input;
								*(disp_format +   4) = '\0';
							}
							else
							{
								strcpy (format,   "12g\0");
								strcpy (disp_format, "eng12");
							}

							update (1);
							update_regs   ();
							break;
						}
					case 'x': /*fix type display format */
						{
							char *format = (char *)   d_format;
							format = stpcpy   (format, "%30.\0");
							if (isdigit   (input = get_ch ()))
							{
								*(format) =   input;
								*(format + 1) =   'f';
								*(format + 2) =   '\0';
								strcpy (disp_format, "fix");
								*(disp_format +   3) = input;
								*(disp_format +   4) = '\0';
							}
							else
							{
								strcpy (format,   "12f\0");
								strcpy (disp_format, "fix12");
							}

							update (1);
							update_regs   ();
							break;
						}
					case 's': /*scientific type display format */
						{
							char *format = (char *)   d_format;
							format = stpcpy   (format, "%30.\0");
							if (isdigit   (input = get_ch ()))
							{
								*(format) =   input;
								*(format + 1) =   'e';
								*(format + 2) =   '\0';
								strcpy (disp_format, "sci");
								*(disp_format +   3) = input;
								*(disp_format +   4) = '\0';
							}
							else
							{
								strcpy (format,   "12e\0");
								strcpy (disp_format, "sci12");
							}

							update (1);
							update_regs   ();
							break;
						}
					case 'l': /* fit straight line through data */
						{
							lastx =   s_x;
							temp = (regs[7]   - regs[9] * regs[6] / regs[4]);
							s_x   = temp / (regs[8] - regs[9] * regs[9] / regs[4]);
							s_y   = (regs[6] - s_x * regs[9]) / regs[4];
							s_z   = s_x * temp / (regs[5] - regs[6] * regs[6] / regs[4]);

							update (1);
							break;
						}

					case 'a': /* FV ordinary annuity (payment at end of period)*/
						{
							regs[2]   = (regs[3]*( (pow((1.0+(regs[0]/100.0)),(regs[4])) - 1.0)/(regs[0]/100.0)));
							update_regs   ();
							break;    
						}


					case 'd': /* FV  annuity due (payment at start of period)*/
						{
							regs[2]   = (regs[3]*( (pow((1.0+(regs[0]/100.0)),(regs[4])) - 1.0)/(regs[0]/100.0)))*(1.0+(regs[0]/100.0));
							update_regs   ();
							break;    
						}
					}
					break;
				}
			case 's':       /* check double codes beginning with s */
				{
					switch (echo_buffer[1])
					{
					case 'i':
						{
							lastx =   s_x;
							s_x   = sin (s_x * convert);
							update (1);
							break;
						}
					case 'h':
						{
							lastx =   s_x;
							s_x   = sinh (s_x * convert);
							update (1);
							break;
						}
					case 'p': /* summation in regs */
						{
							regs[9]   += s_x;
							regs[8]   += s_x * s_x;
							regs[7]   += s_x * s_y;
							regs[6]   += s_y;
							regs[5]   += s_y * s_y;
							regs[4]   += 1.0;
							lastx =   s_x;
							s_x   = regs[4];
							update (0);
							update_regs   ();

							break;
						}
					case 'm': /* anti-summation in regs */
						{
							regs[9]   -= s_x;
							regs[8]   -= s_x * s_x;
							regs[7]   -= s_x * s_y;
							regs[6]   -= s_y;
							regs[5]   -= s_y * s_y;
							regs[4]   -= 1.0;
							lastx =   s_x;
							s_x   = regs[4];
							update (0);
							update_regs   ();

							break;
						}
					case 'd': /* standard deviation */
						{
							lastx =   s_x;
							s_x   = sqrt ((regs[8] - regs[9] * regs[9] / regs[4]) / (regs[4] - 1));
							s_y   = sqrt ((regs[5] - regs[6] * regs[6] / regs[4]) / (regs[4] - 1));
							s_z   = (regs[7] - regs[9] * regs[6] / regs[4]) / (regs[4] - 1.0);
							update (1);

							break;
						}
					case 'o': /* solve quadratic equation */
						{
							double d, t,  r, x;
							d =   s_y * s_y - 4.0 * s_x * s_z;    /*discriminant */
							t =   2.0 * s_x;
							if (fabs (t) > SMALL)
							{   /* is a quadratic equation allright */
								r =   -s_y / t;
								if (d >= 0.0)
								{
									x =   sqrt (d) / t;
									s_x   = r + x;
									s_y   = r - x;
									s_z   = d;
								}
								else
								{
									x =   sqrt (-d) / t;
									s_x   = r;
									s_y   = x;
									s_z   = d;
								}
							}
							else
							{   /* naughty you, a linear equation, flag by y=z=0 */
								s_x   = -s_y / s_z;
								s_y   = 0;
								s_z   = 0;
							}

							update (1);
							break;
						}
					}
					break;
				}
			case 'd':       /* check double codes beginning with d */
				{
					switch (echo_buffer[1])
					{
					case 'g':
						{
							convert   = M_PI / 180.0;
							update (0);
							break;
						}
					case 'h':
						{       /*convert to H.MS */
							int   hours, mins, secs;
							lastx =   s_x;
							secs = (int)ceil  (s_x * 3600.0);
							hours =   secs / 3600;
							secs = (int)fmod  (secs, 3600.0);
							mins = secs   / 60;
							secs = (int)fmod  (secs, 60.0);
							s_x   = hours + mins / 100.0 + secs / 10000.0;
							update (1);
							break;
						}
					}
					break;
				}
			case 'a':       /* check double codes beginning with a */
				{
					switch (echo_buffer[1])
					{
					case 'c':
						{
							lastx =   s_x;
							s_x   = acos (s_x) / convert;
							update (1);
							break;
						}
					case 's':
						{
							lastx =   s_x;
							s_x   = asin (s_x) / convert;
							update (1);
							break;
						}
					case 't':
						{
							lastx =   s_x;
							s_x   = atan (s_x) / convert;
							update (1);
							break;
						}
					}
					break;
				}
			case 'n':       /* check double codes beginning with n */
				{
					switch (echo_buffer[1])
					{
					case 'o':
						{
							lastx =   s_x;
							s_x   = norm_dis (s_x);
							update (1);
							break;
						}
					case 'd':
						{
							lastx =   s_x;
							s_x   = errf (s_x);
							update (1);
							break;
						}
					}
					break;
				}
			case 'i':       /* check double codes beginning with i */
				{
					switch (echo_buffer[1])
					{
					case 'd':
						{
							lastx =   s_x;
							s_x   = inerrf (s_x);
							update (1);
							break;
						}
					case 'n':
						{
							lastx =   s_x;
							s_x   = (double) ((long) s_x);
							;
							update (1);
							break;
						}
					}
					break;
				}
			case 'h':       /* check double codes beginning with h */
				{
					switch (echo_buffer[1])
					{
					case 't':
						{
							lastx =   s_x;
							s_x   = atanh (s_x) / convert;
							update (1);
							break;
						}
					case 's':
						{
							lastx =   s_x;
							s_x   = asinh (s_x) / convert;
							update (1);
							break;
						}
					case 'c':
						{
							lastx =   s_x;
							s_x   = acosh (s_x) / convert;
							update (1);
							break;
						}
					case 'd':
						{       /* convert to decimal hours */
							double mins, secs;
							lastx =   s_x;
							temp = (s_x   - floor (s_x)) * 100.0;
							mins = floor (temp);
							temp = (temp - mins) * 100.0;
							secs = ceil   (temp);
							s_x   = (double) ((long) s_x) + mins / 60.0 +
								secs / 3600.0;
							update (1);
							break;
						}
					}
				}
			case 't':       /* check double codes beginning with t */
				{
					switch (echo_buffer[1])
					{
					case 'a':
						{
							lastx =   s_x;
							s_x   = tan (s_x * convert);
							update (1);
							break;
						}
					case 'p':
						{
							lastx =   s_x;
							s_t   = s_z;
							s_z   = s_y;
							s_y   = s_x;
							s_x   = 2.0 * M_PI;
							update (1);
							break;
						}
					case 'x':
						{
							lastx =   s_x;
							s_x   = pow (10.0, s_x);
							update (1);
							break;
						}
					case 'h':
						{
							lastx =   s_x;
							s_x   = tanh (s_x * convert);
							update (1);
							break;
						}
					}
					break;
				}
			case 'm':       /* check double codes beginning with m */
				{
					switch (echo_buffer[1])
					{
					case 'i':
						{
							regs[0]   = 100.0*find_mortgage_i ();
							update_regs   ();
							break;
						}
					case 'n':
						{
							lastx =   s_x;
							s_x   = regs[9] / regs[4];
							s_y   = regs[6] / regs[4];
							update (1);
							break;
						}
					}
					break;
				}
			}
			second = 0;       /* reset second character flag */
			count =   0;      /* reset input character counter */
		}           /* end of two character commands */
	}
	clrscr_exit   (0);        /* quit program */
}


/*************** Code for MS Visual Studio compiler *****************/
#ifdef _MSC_VER

/* Handle to console */
HANDLE hOuput;
/* current text attribute */
struct _CONSOLE_SCREEN_BUFFER_INFO wAttr ;


void clrscr() /*Clears the screen*/
{
	system("cls");
}

void gotoxy(int xpos, int ypos)
{
	COORD scrn;    
	scrn.X = xpos+1; scrn.Y = ypos-1;
	SetConsoleCursorPosition(hOuput,scrn);
}

void lowvideo()
{
	SetConsoleTextAttribute(hOuput,wAttr.wAttributes );
}

void highvideo()
{
	SetConsoleTextAttribute(hOuput,wAttr.wAttributes ^ FOREGROUND_INTENSITY);
}


char get_ch ()          /* get a character from the keyboard */
{
	return getch ();
}

void clrscr_exit    (int code)      /*clear screen and exit */
{
	clrscr ();
	exit (code);
}

void echo_line ()
{
	gotoxy (echow_left,   echow_bottom);
	puts (echo_buffer);
}

void init ()                /* initializes the screen */
{
	/* Set handle to console */
	hOuput = GetStdHandle(STD_OUTPUT_HANDLE); 
	SetConsoleTitle("RPN");
	GetConsoleScreenBufferInfo(hOuput, &wAttr);
	clrscr ();
	/* initialize stack window */
	gotoxy (1, 1);
	cputs ("RPN Calculator\n\rPress '?' for help");
	gotoxy (stackw_left - 2, stackw_bottom);
	highvideo ();
	putch ('x');
	lowvideo ();
	gotoxy (stackw_left - 2, stackw_bottom - 2);
	putch ('y');
	gotoxy (stackw_left - 2, stackw_bottom - 3);
	putch ('z');
	gotoxy (stackw_left - 2, stackw_bottom - 4);
	putch ('t');
	gotoxy (stackw_left - 3, stackw_bottom - 5);
	cputs ("lx");
	gotoxy (stackw_left + 9, stackw_bottom - 6);
	cputs ("-STACK-");
	gotoxy (stackw_left - 3, stackw_bottom - 5);
	cputs ("lx");
	/* initialize register window */
	gotoxy (regw_left - 7, regw_bottom);
	cputs ("i%  R0");
	gotoxy (regw_left - 7, regw_bottom - 1);
	cputs ("PV  R1");
	gotoxy (regw_left - 7, regw_bottom - 2);
	cputs ("FV  R2");
	gotoxy (regw_left - 7, regw_bottom - 3);
	cputs ("PMT R3");
	gotoxy (regw_left - 7, regw_bottom - 4);
	cputs ("n   R4");
	gotoxy (regw_left - 7, regw_bottom - 5);
	cputs ("�y� R5");
	gotoxy (regw_left - 7, regw_bottom - 6);
	cputs ("�y  R6");
	gotoxy (regw_left - 7, regw_bottom - 7);
	cputs ("�xy R7");
	gotoxy (regw_left - 7, regw_bottom - 8);
	cputs ("�x� R8");
	gotoxy (regw_left - 7, regw_bottom - 9);
	cputs ("�x  R9");
	gotoxy (regw_left + 7, regw_bottom - 10);
	cputs ("-REGISTERS-");
	update_stack ();
	update_regs ();
	update_flags ();
}

void show_help ()           /* show help screen */
{
	clrscr ();
	gotoxy (1, 1);
	puts (help_screen1);
	if (get_ch () == _QUIT)
	{
		clrscr_exit   (0);
	}
	clrscr ();
	puts (help_screen2);
	if (get_ch () == _QUIT)
	{
		clrscr_exit   (0);
	}

	init ();
}

void xupdate    ()
{
	clear_stack   (stackw_bottom);
	gotoxy (stackw_left, stackw_bottom);
	highvideo ();
	cprintf   (d_format, s_x);
	lowvideo ();
}

void update_regs    ()          /* updates regs window on screen */
{
	int   i;
	for   (i = 0; i < 10; i++)
	{
		clear_stack   (regw_bottom - i);
	}
	for   (i = 0; i < 10; i++)
	{
		gotoxy (regw_left, regw_bottom - i);
		cprintf   (d_format, regs[i]);
	}
}


void update_stack ()            /* updates stack window on screen */
{
	int   i;
	for   (i = 0; i < 6; i++)
	{
		clear_stack   (stackw_bottom - i);
	}
	gotoxy (stackw_left, stackw_bottom);
	highvideo ();
	cprintf   (d_format, s_x);
	lowvideo ();
	gotoxy (stackw_left, stackw_bottom - 2);
	cprintf   (d_format, s_y);
	gotoxy (stackw_left, stackw_bottom - 3);
	cprintf   (d_format, s_z);
	gotoxy (stackw_left, stackw_bottom - 4);
	cprintf   (d_format, s_t);
	gotoxy (stackw_left, stackw_bottom - 5);
	cprintf   (d_format, lastx);
}

void update_flags ()            /* updates flag window on screen */
{
	gotoxy (flagw_left,   flagw_bottom);
	if (lift)
	{
		cputs ("lift");
	}
	else
	{
		cputs ("  ");
	}
	gotoxy (flagw_left + 5,   flagw_bottom);
	if (convert   == 1.0)
	{
		cputs ("rad");
	}
	else
	{
		cputs ("deg");
	}
	gotoxy (flagw_left + 9,   flagw_bottom);
	cputs (disp_format);
	putch (' ');
}

void clear_stack    (char xyzt)     /* clear  stack window */
{
	int   count = 0;
	while (count < stackw_width)
	{
		gotoxy (stackw_left   + count++, xyzt);
		putch (' ');
	}
}

void clear_echo ()            /* clear  echo window */
{
	int   count = 0;
	while (count < 80 -   echow_left) /*clear to end of line */
	{
		gotoxy (echow_left + count++, echow_bottom);
		putch (' ');
	}
}

#endif  /* _MSC_VER */



/*************** Code for Borland Turbo C compiler *****************/
#ifdef __TURBOC__

char get_ch ()            /* get a character from the keyboard */
{
	return getch ();
}

void clrscr_exit  (int code)      /*clear screen and exit */
{
	clrscr ();
	exit (code);
}

void echo_line ()
{
	gotoxy (echow_left, echow_bottom);
	puts (echo_buffer);
}


void init ()              /* initializes the screen */
{
	/* TurboC screen movement */
	clrscr ();
	/* initialize stack window */
	gotoxy (1, 1);
	cputs ("RPN Calculator\n\rPress '?' for help");
	gotoxy (stackw_left - 2, stackw_bottom);
	highvideo ();
	putch ('x');
	lowvideo ();
	gotoxy (stackw_left - 2, stackw_bottom - 2);
	putch ('y');
	gotoxy (stackw_left - 2, stackw_bottom - 3);
	putch ('z');
	gotoxy (stackw_left - 2, stackw_bottom - 4);
	putch ('t');
	gotoxy (stackw_left - 3, stackw_bottom - 5);
	cputs ("lx");
	gotoxy (stackw_left + 9, stackw_bottom - 6);
	cputs ("-STACK-");
	gotoxy (stackw_left - 3, stackw_bottom - 5);
	cputs ("lx");
	/* initialize register window */
	gotoxy (regw_left - 7, regw_bottom);
	cputs ("i   R0");
	gotoxy (regw_left - 7, regw_bottom - 1);
	cputs ("PV  R1");
	gotoxy (regw_left - 7, regw_bottom - 2);
	cputs ("FV  R2");
	gotoxy (regw_left - 7, regw_bottom - 3);
	cputs ("PMT R3");
	gotoxy (regw_left - 7, regw_bottom - 4);
	cputs ("n   R4");
	gotoxy (regw_left - 7, regw_bottom - 5);
	cputs ("�y� R5");
	gotoxy (regw_left - 7, regw_bottom - 6);
	cputs ("�y  R6");
	gotoxy (regw_left - 7, regw_bottom - 7);
	cputs ("�xy R7");
	gotoxy (regw_left - 7, regw_bottom - 8);
	cputs ("�x� R8");
	gotoxy (regw_left - 7, regw_bottom - 9);
	cputs ("�x  R9");
	gotoxy (regw_left + 7, regw_bottom - 10);
	cputs ("-REGISTERS-");
	update_stack ();
	update_regs ();
	update_flags ();
}

void show_help ()         /* show help screen */
{
	clrscr ();
	gotoxy (1, 1);
	puts (help_screen1);
	if (get_ch () == _QUIT)
	{
		clrscr_exit   (0);
	}
	clrscr ();
	puts (help_screen2);
	if (get_ch () == _QUIT)
	{
		clrscr_exit   (0);
	}

	init ();
}

void xupdate  ()
{
	clear_stack   (stackw_bottom);
	gotoxy (stackw_left, stackw_bottom);
	highvideo ();
	cprintf (d_format, s_x);
	lowvideo ();
}

void update_regs  ()          /* updates regs window on screen */
{
	int   i;
	for   (i = 0; i < 10; i++)
	{
		clear_stack   (regw_bottom - i);
	}
	for   (i = 0; i < 10; i++)
	{
		gotoxy (regw_left, regw_bottom - i);
		cprintf (d_format, regs[i]);
	}
}

void update_stack ()          /* updates stack window on screen */
{
	int   i;
	for   (i = 0; i < 6; i++)
	{
		clear_stack   (stackw_bottom - i);
	}
	gotoxy (stackw_left, stackw_bottom);
	highvideo ();
	cprintf (d_format, s_x);
	lowvideo ();
	gotoxy (stackw_left, stackw_bottom - 2);
	cprintf (d_format, s_y);
	gotoxy (stackw_left, stackw_bottom - 3);
	cprintf (d_format, s_z);
	gotoxy (stackw_left, stackw_bottom - 4);
	cprintf (d_format, s_t);
	gotoxy (stackw_left, stackw_bottom - 5);
	cprintf (d_format, lastx);
}

void update_flags ()          /* updates flag window on screen */
{
	gotoxy (flagw_left, flagw_bottom);
	if (lift)
	{
		cputs ("lift");
	}
	else
	{
		cputs ("    ");
	}
	gotoxy (flagw_left + 5, flagw_bottom);
	if (convert == 1.0)
	{
		cputs ("rad");
	}
	else
	{
		cputs ("deg");
	}
	gotoxy (flagw_left + 9, flagw_bottom);
	cputs (disp_format);
	putch (' ');
}

void clear_stack  (char xyzt)     /* clear  stack window */
{
	int   count = 0;
	while (count < stackw_width)
	{
		gotoxy (stackw_left + count++, xyzt);
		putch (' ');
	}
}

void clear_echo ()            /* clear  echo window */
{
	int   count = 0;
	while (count < 80 -   echow_left) /*clear to end of line */
	{
		gotoxy (echow_left + count++, echow_bottom);
		putch (' ');
	}
}

#endif /* __TURBOC__ */



/*************** Code for Linux GNU c compiler *****************/
#ifdef __unix__

static WINDOW *screen_OK = NULL;

char get_ch ()            /* get a character from the keyboard */
{
	char in;
	in = getch ();
	switch (in)
	{
	case '\n':
		return '\r';      /* unix return */
	case 'J':
		return '.';       /* my keypad seems to need this */
	case '\a':
		return '\b';      /* unix rubout */
	default:
		return in;
	}
}

void clrscr_exit  (int code)      /*clear screen and exit */
{
	erase ();
	refresh ();
	endwin ();
	exit (code);
}

void echo_line ()
{
	move (echow_bottom, echow_left);
	clrtoeol ();
	mvaddstr (echow_bottom, echow_left, echo_buffer);
	refresh ();
}

void init ()              /* initializes the screen */
{
	if (screen_OK == NULL)
	{
		screen_OK = initscr ();   /* methinks this mallocs so I'll do it only once */
		keypad (stdscr, TRUE);
		leaveok (stdscr, TRUE);
		cbreak ();
		noecho ();
	}
	else
		erase ();
	/* initialize stack window */
	mvaddstr (1, 0, "RPN Calculator\n\rPress '?' for help");
	attron (A_STANDOUT);
	mvaddch (stackw_left - 2, stackw_bottom, 'x');
	attroff (A_STANDOUT);
	mvaddch (stackw_bottom - 2, stackw_left - 2, 'y');
	mvaddch (stackw_bottom - 3, stackw_left - 2, 'z');
	mvaddch (stackw_bottom - 4, stackw_left - 2, 't');
	mvaddstr (stackw_bottom - 5, stackw_left - 3, "lx");
	mvaddstr (stackw_bottom - 6, stackw_left + 9, "-STACK-");
	mvaddstr (stackw_bottom - 5, stackw_left - 3, "lx");
	/* initialize register window */
	mvaddstr (regw_bottom, regw_left - 9, "i%    R0");
	mvaddstr (regw_bottom - 1, regw_left - 9, "PV    R1");
	mvaddstr (regw_bottom - 2, regw_left - 9, "FV    R2");
	mvaddstr (regw_bottom - 3, regw_left - 9, "PMT   R3");
	mvaddstr (regw_bottom - 4, regw_left - 9, "n     R4");
	mvaddstr (regw_bottom - 5, regw_left - 9, "S:y^2 R5");
	mvaddstr (regw_bottom - 6, regw_left - 9, "S:y   R6");
	mvaddstr (regw_bottom - 7, regw_left - 9, "S:xy  R7");
	mvaddstr (regw_bottom - 8, regw_left - 9, "S:x^2 R8");
	mvaddstr (regw_bottom - 9, regw_left - 9, "S:x   R9");
	mvaddstr (regw_bottom - 10, regw_left + 7, "-REGISTERS-");
	update_stack ();
	update_regs ();
	update_flags ();
	refresh ();
}

void show_help ()         /* show help screen */
{
	clear ();
	mvaddstr (1, 0, help_screen1);
	refresh ();
	if (get_ch () == _QUIT)
	{
		clrscr_exit   (0);
	}
	clear ();
	mvaddstr (1, 0, help_screen2);
	refresh ();
	if (get_ch () == _QUIT)
	{
		clrscr_exit   (0);
	}
	init ();
}

void xupdate  ()
{
	clear_stack   (stackw_bottom);
	attron (A_STANDOUT);
	mvprintw (stackw_bottom, stackw_left, d_format, s_x);
	attroff (A_STANDOUT);
}

void update_regs  ()          /* updates regs window on screen */
{
	int   i;
	for   (i = 0; i < 10; i++)
	{
		clear_stack   (regw_bottom - i);
	}
	for   (i = 0; i < 10; i++)
	{
		mvprintw (regw_bottom - i, regw_left, d_format, regs[i]);
	}
}


void update_stack ()          /* updates stack window on screen */
{
	int   i;
	for   (i = 0; i < 6; i++)
	{
		clear_stack   (stackw_bottom - i);
	}
	attron (A_STANDOUT);
	mvprintw (stackw_bottom, stackw_left, d_format, s_x);
	attroff (A_STANDOUT);
	mvprintw (stackw_bottom - 2, stackw_left, d_format, s_y);
	mvprintw (stackw_bottom - 3, stackw_left, d_format, s_z);
	mvprintw (stackw_bottom - 4, stackw_left, d_format, s_t);
	mvprintw (stackw_bottom - 5, stackw_left, d_format, lastx);
}

void update_flags ()          /* updates flag window on screen */
{
	if (lift)
		mvaddstr (flagw_bottom, flagw_left, "lift");
	else
		mvaddstr (flagw_bottom, flagw_left, "     ");
	if (convert == 1.0)
		mvaddstr (flagw_bottom, flagw_left + 5, "rad");
	else
		mvaddstr (flagw_bottom, flagw_left + 5, "deg");
	mvprintw (flagw_bottom, flagw_left + 9, disp_format);
	addch (' ');
}

void clear_stack  (char xyzt)     /* clear  stack window */
{
	int   count = 0;
	while (count < stackw_width)
	{
		mvaddch (xyzt, stackw_left + count++, ' ');
	}
}

void clear_echo ()            /* clear  echo window */
{
	int   count = 0;
	while (count < 80 -   echow_left) /*clear to end of line */
	{
		mvaddch (echow_bottom, echow_left + count++, ' ');
	}
}

#endif /* __unix__ */


void update (char liftup)     /* updates display */
{
	/*update stack on screen */
	lift = liftup;
	update_stack ();
	update_flags ();
}


double atanh  (double x)      /* atanh function */
{
	return (0.5   * log ((1.0 + x) / (1.0 - x)));
}

double asinh  (double x)      /* asinh function */
{
	return (log   (x + sqrt (x * x + 1.0)));
}


double acosh  (double x)      /* acosh function */
{
	return (log   (x + sqrt (x * x - 1.0)));
}


double find_mortgage_i    ()      /* calculate interest rate by iteration */
{
        double i0, i1, i0p1, n, pofn, rat;
	n =   regs[4];
	i1 = 1.0 / rat - rat / (n * n);
	do
	{
		i0 = i1;
		i0p1 = i0 + 1.0;
		pofn = pow (i0p1,-n);
		i1 = i0 - ((1.0 - pofn) / i0 - rat) / ((pofn * (i0 * i0p1 / (1.0 - n) + 1.0) - 1.0) / (i0 * i0));
	}
	while ( fabs(i0 - i1) > 1.e-9);
	return i1;
}

double errf (double x)            /* error function */
{
	const double r = 0.2316419,   b1 = 0.31938153, b2 = -.356563782, b3 = 1.781477937,
		b4 = -1.82155978, b5 = 1.330274429;
	double t, neg, cx;
	if (x <   0.0)
	{
		cx = -x;
		neg   = 1.0;
	}
	else
	{
		cx = x;
		neg   = 0.0;
	}
	t =   1.0 / (1.0 + r * cx);
	cx = norm_dis (x) *   ((((b5 * t + b4) * t + b3) * t + b2) * t + b1) * t;
	return (neg) ? cx :   1.0 - cx;
}

double inerrf (double x)      /* inverse error function ) */
{
	const double
		c0 = 2.515517, c1 = 0.802853, c2 = 0.010328, d1 = 1.432788, d2 = 0.189269,
		d3 = 0.001308;
	double t, omx;
	double lth;
	if (x <   0.5)
	{
		omx   = x;
		lth   = -1.0;
	}
	else
	{
		omx   = 1.0 - x;
		lth   = 1.0;
	}
	t =   sqrt (log (1.0 / (omx * omx)));
	return (t -   ((c2 * t + c1) * t + c0) / (((d3 * t + d2) * t + d1) * t + 1.0)) * lth;
}

double norm_dis (double x)        /* normal distribution */
{
	return 1.0 / sqrt (2.0 * M_PI) * exp (-x * x * 0.5);
}
