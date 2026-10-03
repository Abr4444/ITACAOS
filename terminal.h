#ifndef __TERMINAL_H__
#define __TERMINAL_H__

/************************************************
 *
 * @File: terminal.h
 * @Purpose: Parser of the Odysseus command terminal. Recognizes the
 *           commands (case insensitive), checks their syntax and
 *           extracts their arguments.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 02/10/2026
 *
 ************************************************/

// System includes
#include <ctype.h>

// Project includes
#include "io.h"

// One more than the longest command, so extra words are detected
#define MAX_WORDS 4
#define WORD_SEPARATORS " \t"

// 9 digits always fit in an int
#define MAX_NUMBER_DIGITS 9

// Results of parseCommand
#define PARSE_OK 0
#define PARSE_UNKNOWN -1
#define PARSE_BAD_SYNTAX -2
#define PARSE_EMPTY -3

// Command types
#define CMD_NONE 0
#define CMD_CONNECT_ITHACA 1
#define CMD_LIST_VOYAGES 2
#define CMD_ACCEPT 3
#define CMD_SAIL 4
#define CMD_MAP 5
#define CMD_LIST_MARKET 6
#define CMD_BUY 7
#define CMD_BUY_MAP 8
#define CMD_SELL 9
#define CMD_STATUS 10
#define CMD_DELIVER 11
#define CMD_CLAIM 12

// Expected syntax, shown when a known command is misused
#define SYNTAX_ERROR "Incorrect syntax. Usage: "
#define USAGE_CONNECT SYNTAX_ERROR "CONNECT ITHACA\n"
#define USAGE_LIST SYNTAX_ERROR "LIST VOYAGES | LIST MARKET\n"
#define USAGE_LIST_VOYAGES SYNTAX_ERROR "LIST VOYAGES\n"
#define USAGE_LIST_MARKET SYNTAX_ERROR "LIST MARKET\n"
#define USAGE_ACCEPT SYNTAX_ERROR "ACCEPT <ID> (positive number)\n"
#define USAGE_SAIL SYNTAX_ERROR "SAIL <ISLAND>\n"
#define USAGE_MAP SYNTAX_ERROR "MAP\n"
#define USAGE_BUY SYNTAX_ERROR "BUY <PRODUCT> <AMOUNT> (positive number)\n"
#define USAGE_BUY_MAP SYNTAX_ERROR "BUY MAP 1\n"
#define USAGE_SELL SYNTAX_ERROR "SELL <PRODUCT> <AMOUNT> (positive number)\n"
#define USAGE_STATUS SYNTAX_ERROR "STATUS\n"
#define USAGE_DELIVER SYNTAX_ERROR "DELIVER\n"
#define USAGE_CLAIM SYNTAX_ERROR "CLAIM\n"

typedef struct {
    int nType;
    char *psArgument;
    int nNumber;
    char *psUsage;
} Command;

int parseCommand(char *psLine, Command *pstCommand);

#endif