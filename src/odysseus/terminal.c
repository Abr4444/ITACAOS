/************************************************
 * @File: terminal.c
 * @Purpose: Parser of the Odysseus command terminal. Recognizes the
 *           commands (case insensitive), checks their syntax and
 *           extracts their arguments.
 * @Date: 02/10/2026
 ************************************************/

#include "terminal.h"

/************************************************
 * @Name: sameWord
 * @Def: Compares a typed word with an uppercase keyword, ignoring case.
 * @Arg: In: psWord = word typed by the user (may be NULL)
 *       In: psKeyword = keyword in uppercase
 * @Ret: 1 if they match, 0 otherwise.
 ************************************************/
static int sameWord(char *psWord, char *psKeyword) {

    int i = 0;

    if (NULL == psWord) {
        return 0;
    }
    while ('\0' != psWord[i] && toupper((unsigned char) psWord[i]) == psKeyword[i]) {
        i++;
    }
    return '\0' == psWord[i] && '\0' == psKeyword[i];
}

/************************************************
 * @Name: toNumber
 * @Def: Converts a word made only of digits into an int.
 * @Arg: In: psWord = word to convert (may be NULL)
 * @Ret: The value, or -1 if the word is missing, has a non-digit
 *       character or is too long to fit in an int.
 ************************************************/
static int toNumber(char *psWord) {

    int nValue = 0;
    int i = 0;

    if (NULL == psWord) {
        return -1;
    }
    while ('\0' != psWord[i]) {
        if (MAX_NUMBER_DIGITS <= i || 0 == isdigit((unsigned char) psWord[i])) {
            return -1;
        }
        nValue = nValue * 10 + (psWord[i] - '0');
        i++;
    }
    return nValue;
}

/************************************************
 * @Name: splitWords
 * @Def: Splits the line into words separated by spaces or tabs.
 *       The line is modified (strtok).
 * @Arg: In/Out: psLine = line typed by the user
 *       Out: apsWords = first MAX_WORDS words, unused slots set to NULL
 * @Ret: The total number of words, which may exceed MAX_WORDS.
 ************************************************/
static int splitWords(char *psLine, char **apsWords) {

    char *psWord = NULL;
    int nWords = 0;
    int i = 0;

    for (i = 0; i < MAX_WORDS; i++) {
        apsWords[i] = NULL;
    }
    psWord = strtok(psLine, WORD_SEPARATORS);
    while (NULL != psWord) {
        if (MAX_WORDS > nWords) {
            apsWords[nWords] = psWord;
        }
        nWords++;
        psWord = strtok(NULL, WORD_SEPARATORS);
    }
    return nWords;
}

/************************************************
 * @Name: setResult
 * @Def: Stores the command type and its usage text, and turns the
 *       validity check into a parse result.
 * @Arg: Out: pstCommand = command being parsed
 *       In: nType = CMD_ value of the recognized command
 *       In: psUsage = expected syntax of that command
 *       In: nIsValid = 1 if the syntax was correct, 0 otherwise
 * @Ret: PARSE_OK or PARSE_BAD_SYNTAX.
 ************************************************/
static int setResult(Command *pstCommand, int nType, char *psUsage, int nIsValid) {

    pstCommand->nType = nType;
    pstCommand->psUsage = psUsage;
    if (nIsValid) {
        return PARSE_OK;
    }
    return PARSE_BAD_SYNTAX;
}

/************************************************
 * @Name: parseList
 * @Def: Parses LIST VOYAGES and LIST MARKET.
 * @Arg: In: apsWords = words of the line
 *       In: nWords = number of words
 *       Out: pstCommand = parsed command
 * @Ret: PARSE_OK or PARSE_BAD_SYNTAX.
 ************************************************/
static int parseList(char **apsWords, int nWords, Command *pstCommand) {

    if (sameWord(apsWords[1], "VOYAGES")) {
        return setResult(pstCommand, CMD_LIST_VOYAGES, USAGE_LIST_VOYAGES, 2 == nWords);
    }
    if (sameWord(apsWords[1], "MARKET")) {
        return setResult(pstCommand, CMD_LIST_MARKET, USAGE_LIST_MARKET, 2 == nWords);
    }
    return setResult(pstCommand, CMD_NONE, USAGE_LIST, 0);
}

/************************************************
 * @Name: parseTrade
 * @Def: Parses BUY <PRODUCT> <AMOUNT>, SELL <PRODUCT> <AMOUNT> and
 *       BUY MAP 1 (the only valid amount for a map).
 * @Arg: In: apsWords = words of the line
 *       In: nWords = number of words
 *       Out: pstCommand = parsed command
 *       In: nType = CMD_BUY or CMD_SELL
 *       In: psUsage = expected syntax of that command
 * @Ret: PARSE_OK or PARSE_BAD_SYNTAX.
 ************************************************/
static int parseTrade(char **apsWords, int nWords, Command *pstCommand, int nType, char *psUsage) {

    pstCommand->psArgument = apsWords[1];
    pstCommand->nNumber = toNumber(apsWords[2]);

    if (CMD_BUY == nType && sameWord(apsWords[1], "MAP")) {
        return setResult(pstCommand, CMD_BUY_MAP, USAGE_BUY_MAP, 3 == nWords && 1 == pstCommand->nNumber);
    }
    return setResult(pstCommand, nType, psUsage, 3 == nWords && 0 < pstCommand->nNumber);
}

/************************************************
 * @Name: parseSimpleCommand
 * @Def: Parses the commands without user-given arguments:
 *       CONNECT ITHACA, LIST ..., MAP, STATUS, DELIVER and CLAIM.
 * @Arg: In: apsWords = words of the line
 *       In: nWords = number of words
 *       Out: pstCommand = parsed command
 * @Ret: PARSE_OK, PARSE_BAD_SYNTAX, or PARSE_UNKNOWN if the first
 *       word is none of these commands.
 ************************************************/
static int parseSimpleCommand(char **apsWords, int nWords, Command *pstCommand) {

    if (sameWord(apsWords[0], "CONNECT")) {
        return setResult(pstCommand, CMD_CONNECT_ITHACA, USAGE_CONNECT,
                         2 == nWords && sameWord(apsWords[1], "ITHACA"));
    }
    if (sameWord(apsWords[0], "LIST")) {
        return parseList(apsWords, nWords, pstCommand);
    }
    if (sameWord(apsWords[0], "MAP")) {
        return setResult(pstCommand, CMD_MAP, USAGE_MAP, 1 == nWords);
    }
    if (sameWord(apsWords[0], "STATUS")) {
        return setResult(pstCommand, CMD_STATUS, USAGE_STATUS, 1 == nWords);
    }
    if (sameWord(apsWords[0], "DELIVER")) {
        return setResult(pstCommand, CMD_DELIVER, USAGE_DELIVER, 1 == nWords);
    }
    if (sameWord(apsWords[0], "CLAIM")) {
        return setResult(pstCommand, CMD_CLAIM, USAGE_CLAIM, 1 == nWords);
    }
    return PARSE_UNKNOWN;
}

/************************************************
 * @Name: parseArgumentCommand
 * @Def: Parses the commands with user-given arguments:
 *       ACCEPT, SAIL, BUY and SELL.
 * @Arg: In: apsWords = words of the line
 *       In: nWords = number of words
 *       Out: pstCommand = parsed command
 * @Ret: PARSE_OK, PARSE_BAD_SYNTAX, or PARSE_UNKNOWN if the first
 *       word is none of these commands.
 ************************************************/
static int parseArgumentCommand(char **apsWords, int nWords, Command *pstCommand) {

    if (sameWord(apsWords[0], "ACCEPT")) {
        pstCommand->nNumber = toNumber(apsWords[1]);
        return setResult(pstCommand, CMD_ACCEPT, USAGE_ACCEPT, 2 == nWords && 0 < pstCommand->nNumber);
    }
    if (sameWord(apsWords[0], "SAIL")) {
        pstCommand->psArgument = apsWords[1];
        return setResult(pstCommand, CMD_SAIL, USAGE_SAIL, 2 == nWords);
    }
    if (sameWord(apsWords[0], "BUY")) {
        return parseTrade(apsWords, nWords, pstCommand, CMD_BUY, USAGE_BUY);
    }
    if (sameWord(apsWords[0], "SELL")) {
        return parseTrade(apsWords, nWords, pstCommand, CMD_SELL, USAGE_SELL);
    }
    return PARSE_UNKNOWN;
}

/************************************************
 * @Name: parseCommand
 * @Def: Recognizes a terminal line and validates its syntax.
 *       The line is modified and the command's psArgument points
 *       inside it, so it must not be freed while the command is used.
 * @Arg: In/Out: psLine = line typed by the user
 *       Out: pstCommand = parsed command
 * @Ret: PARSE_OK, PARSE_BAD_SYNTAX (see pstCommand->psUsage),
 *       PARSE_UNKNOWN, or PARSE_EMPTY for a blank line.
 ************************************************/
int parseCommand(char *psLine, Command *pstCommand) {

    char *apsWords[MAX_WORDS];
    int nWords = 0;
    int nResult = PARSE_UNKNOWN;

    pstCommand->nType = CMD_NONE;
    pstCommand->psArgument = NULL;
    pstCommand->nNumber = -1;
    pstCommand->psUsage = NULL;

    nWords = splitWords(psLine, apsWords);
    if (0 == nWords) {
        return PARSE_EMPTY;
    }

    nResult = parseSimpleCommand(apsWords, nWords, pstCommand);
    if (PARSE_UNKNOWN == nResult) {
        nResult = parseArgumentCommand(apsWords, nWords, pstCommand);
    }
    return nResult;
}