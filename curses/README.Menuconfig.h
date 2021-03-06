"Menuconfig gives the Linux kernel configuration a long needed face\n"
"lift.  Featuring text based color menus and dialogs, it does not\n"
"require X Windows.  With this utility you can easily select a kernel\n"
"option to modify without sifting through 100 other options.\n"
"\n"
"Overview\n"
"--------\n"
"Some kernel features may be built directly into the kernel.\n"
"Some may be made into loadable runtime modules.  Some features\n"
"may be completely removed altogether.  There are also certain\n"
"kernel parameters which are not really features, but must be \n"
"entered in as decimal or hexadecimal numbers or possibly text.\n"
"\n"
"Menu items beginning with [*], <M> or [ ] represent features \n"
"configured to be built in, modularized or removed respectively.\n"
"Pointed brackets <> represent module capable features.\n"
"                                                             more...\n"
"\n"
"To change any of these features, highlight it with the cursor \n"
"keys and press <Y> to build it in, <M> to make it a module or\n"
"<N> to removed it.  You may also press the <Space Bar> to cycle\n"
"through the available options (ie. Y->N->M->Y). \n"
"\n"
"Items beginning with numbers or other text within parenthesis can \n"
"be changed by highlighting the item and pressing <Enter>.  Then\n"
"enter the new parameter into the dialog box that pops up.\n"
"\n"
"\n"
"Some additional keyboard hints:\n"
"\n"
"Menus\n"
"----------\n"
"o  Use the Up/Down arrow keys (cursor keys) to highlight the item \n"
"   you wish to change or submenu wish to select and press <Enter>.\n"
"   Submenus are designated by \"--->\".\n"
"\n"
"   Shortcut: Press the option's highlighted letter (hotkey).\n"
"             Pressing a hotkey more than once will sequence\n"
"             through all visible items which use that hotkey.\n"
"\n"
"   You may also use the <PAGE UP> and <PAGE DOWN> keys to scroll\n"
"   unseen options into view.\n"
"\n"
"o  To exit a menu use the cursor keys to highlight the <Exit> button\n"
"   and press <ENTER>.  \n"
"\n"
"   Shortcut: Press <ESC><ESC> or <E> or <X> if there is no hotkey\n"
"             using those letters.  You may press a single <ESC>, but\n"
"             there is a delayed response which you may find annoying.\n"
"\n"
"   Also, the <TAB> and cursor keys will cycle between <Select>,\n"
"   <Exit> and <Help>\n"
"\n"
"o  To get help with an item, use the cursor keys to highlight <Help>\n"
"   and Press <ENTER>.\n"
"\n"
"   Shortcut: Press <H> or <?>.\n"
"\n"
"\n"
"Radiolists  (Choice lists)\n"
"-----------\n"
"o  Use the cursor keys to select the option you wish to set and press\n"
"   <S> or the <SPACE BAR>.\n"
"\n"
"   Shortcut: Press the first letter of the option you wish to set then\n"
"             press <S> or <SPACE BAR>.\n"
"\n"
"o  To see available help for the item, use the cursor keys to highlight\n"
"   <Help> and Press <ENTER>.\n"
"\n"
"   Shortcut: Press <H> or <?>.\n"
"\n"
"   Also, the <TAB> and cursor keys will cycle between <Select> and\n"
"   <Help>\n"
"\n"
"\n"
"Data Entry\n"
"-----------\n"
"o  Enter the requested information and press <ENTER>\n"
"   If you are entering hexadecimal values, it is not necessary to\n"
"   add the '0x' prefix to the entry.\n"
"\n"
"o  For help, use the <TAB> or cursor keys to highlight the help option\n"
"   and press <ENTER>.  You can try <TAB><H> as well.\n"
"\n"
"\n"
"Text Box    (Help Window)\n"
"--------\n"
"o  Use the cursor keys to scroll up/down/left/right.  The VI editor\n"
"   keys h,j,k,l function here as do <SPACE BAR> and <B> for those\n"
"   who are familiar with less and lynx.\n"
"\n"
"o  Press <E>, <X>, <Enter> or <Esc><Esc> to exit.\n"
"\n"
"\n"
"Final Acceptance\n"
"----------------\n"
"With the exception of the old style sound configuration,\n"
"YOUR CHANGES ARE NOT FINAL.  You will be given a last chance to\n"
"confirm them prior to exiting Menuconfig.\n"
"\n"
"If Menuconfig quits with an error while saving your configuration,\n"
"you may look in the file /usr/src/linux/.menuconfig.log for\n"
"information which may help you determine the cause.\n"
"\n"
"Alternate Configuration Files\n"
"-----------------------------\n"
"Menuconfig supports the use of alternate configuration files for\n"
"those who, for various reasons, find it necessary to switch \n"
"between different kernel configurations.\n"
"\n"
"At the end of the main menu you will find two options.  One is\n"
"for saving the current configuration to a file of your choosing.\n"
"The other option is for loading a previously saved alternate\n"
"configuration.\n"
"\n"
"Even if you don't use alternate configuration files, but you \n"
"find during a Menuconfig session that you have completely messed\n"
"up your settings, you may use the \"Load Alternate...\" option to\n"
"restore your previously saved settings from \".config\" without \n"
"restarting Menuconfig.\n"
"\n"
"Other information\n"
"-----------------\n"
"The windowing utility, lxdialog, will only be rebuilt if your kernel\n"
"source tree is fresh, or changes are patched into it via a kernel\n"
"patch or you do 'make mrproper'.  If changes to lxdialog are patched\n"
"in, most likely the rebuild time will be short.  You may force a\n"
"complete rebuild of lxdialog by changing to it's directory and doing\n"
"'make clean all'\n"
"\n"
"If you use Menuconfig in an XTERM window make sure you have your \n"
"$TERM variable set to point to a xterm definition which supports color.\n"
"Otherwise, Menuconfig will look rather bad.  Menuconfig will not \n"
"display correctly in a RXVT window because rxvt displays only one\n"
"intensity of color, bright.\n"
"\n"
"Menuconfig will display larger menus on screens or xterms which are\n"
"set to display more than the standard 25 row by 80 column geometry.\n"
"In order for this to work, the \"stty size\" command must be able to \n"
"display the screen's current row and column geometry.  I STRONGLY\n"
"RECOMMEND that you make sure you do NOT have the shell variables\n"
"LINES and COLUMNS exported into your environment.  Some distributions\n"
"export those variables via /etc/profile.  Some ncurses programs can\n"
"become confused when those variables (LINES & COLUMNS) don't reflect\n"
"the true screen size.\n"
"\n"
"\n"
"NOTICE:  lxdialog requires the ncurses libraries to compile.  If you\n"
"         don't already have ncurses you really should get it.\n"
"\n"
"         The makefile for lxdialog attempts to find your ncurses\n"
"         header file.  Although it should find the header for older\n"
"         versions of ncurses, it is probably a good idea to get the\n"
"         latest ncurses anyway. \n"
"\n"
"         If you have upgraded your ncurses libraries, MAKE SURE you\n"
"         remove the old ncurses header files.  If you don't you\n"
"         will most certainly get a segmentation fault.\n"
"\n"
"WARNING: It is not recommended that you change any defines in\n"
"         lxdialog's header files.  If you have a grayscale display and\n"
"         are brave, you may tinker with color.h to tune the colors to\n"
"         your preference.\n"
"\n"
"COMPATIBILITY ISSUE:\n"
"         There have been some compatibility problems reported with\n"
"         older versions of bash and sed.  I am trying to work these\n"
"         out but it is preferable that you upgrade those utilities.\n"
"\n"
"\n"
"******** IMPORTANT, OPTIONAL ALTERNATE PERSONALITY AVAILABLE ********\n"
"********                                                     ********\n"
"If you prefer to have all of the kernel options listed in a single\n"
"menu, rather than the default multimenu hierarchy, you may edit the\n"
"Menuconfig script and change the line \"single_menu_mode=\"  to \n"
"\"single_menu_mode=TRUE\".\n"
"\n"
"This mode is not recommended unless you have a fairly fast machine.\n"
"*********************************************************************\n"
"\n"
"\n"
"Propaganda\n"
"----------\n"
"The windowing support utility (lxdialog) is a VERY modified version of\n"
"the dialog utility by Savio Lam <lam836@cs.cuhk.hk>.  Although lxdialog\n"
"is significantly different from dialog, I have left Savio's copyrights\n"
"intact.  Please DO NOT contact Savio with questions about lxdialog.\n"
"He will not be able to assist.\n"
"\n"
"William Roadcap was the original author of Menuconfig.\n"
"Michael Elizabeth Chastain <mec@shout.net> is the current maintainer.\n"
"\n"
"<END OF FILE>\n"
