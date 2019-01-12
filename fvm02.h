// fvm01.h // 2017/05/09 derived from fvm00.h samsuanchen@gmail.com
           // 2019/01/08 updated for wbc_lcdtest.ino samsuanchen@gmail.com

#ifndef _FVM_H_
#define _FVM_H_

#include <arduino.h>

// Serial IO ////////////////////////////////////////////////////////////////////////////////////////////
#define PRINTF	  Serial.printf    // formated print
#define PRINT     Serial.print     // print given object
#define AVAILABLE Serial.available // check if available to read
#define READ	  Serial.read      // read ascii code of available char
#define WRITE	  Serial.write     // write char of given ascii code
#define ABORT(ERR,ID,CODE,FORMAT,...) ERR=ID;Serial.printf("\nError %03d ",ID);Serial.printf(FORMAT,__VA_ARGS__);CODE()
// running state
#define READING 0 // reading to terminal input buffer char by char.
#define PARSING 1 // parsing and evaluating from script token by token.
#define CMPLING 2 // compiling to word-list of forth colon definition word by word.
// word flag
#define IMMED       0x8000 // immediate word
#define COMPO       0x4000 // compile-only word
#define IMMED_COMPO 0xc000 // immediate compile-only word
#define HIDEN       0x2000 // hidden word (searched but not seen)
#define COMPO_HIDEN 0x6000 // hidden compile-only word
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TIB_SIZE  2048            // default size of terminal input buffer to recieve characters
#define TMP_SIZE   256            // default size of temporary buffer to parse token or to convert integer to number string
#define SL_LIMIT   254            // string length limit (number of characters as leading byte, 0 as trailing byte)
#define SB_SIZE   4096            // default size of string buffer to keep all unique strings
#define SB_LIMIT  (SB+SB_SIZE-1)  // string buffer limit
#define DS_SIZE     16            // default depth limit of data stack (number of 32-bit cells)
#define DS_LIMIT  (DS+DS_SIZE-1)  // data stack limit
#define CS_SIZE    256            // default number of 32-bit cells as compile space for word list of a forth colon definition
#define CS_LIMIT  (CS+CS_SIZE-1)  // compile space limit
#define RS_SIZE     16            // default depth limit of return stack (number of 32-bit cells)
#define RS_LIMIT  (RS+RS_SIZE-1)  // return stack limit
#define CONSOLE_WIDTH 80          // console output length limit (used by the forth word "words")
/////////////////////////////////////////////////////////////////////////////////////////////////////////
union P {                 // use same 32-bit cell to hold any one of the following:
  int            con    ; // constant
//int            var    ; // variable
//int            val    ; // value
  char         * mne    ; // name of a primitive word
  struct Word ** wpl    ; // word list of forth colon definition
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////
union X {                 // use same 32-bit cell to hold a floating number or an integer number
  float          f      ; // a floating number
  int            i      ; // an integer number
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void (*FuncP)() ; // the forth function pointer type
/////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct Word {     // the forth word type
  struct Word * link    ; // link to previous word
  uint16_t      id      ; // word id
  uint16_t      flag    ; // IMMED 1 immediate, COMPO 2 compileOnly, HIDEN 3 hidden,
  char        * name    ; // the address pointing to name of the forth word
  FuncP         code    ; // pointing to the function code to execute
  P				p       ; // parameter field
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct Voc {      // the forth vocaburary type
  Word     * first      ; // point to the first forth word
  Word     * predefined ; // point to the last predefined foth word
  Word     * context    ; // point to the last forth word in voc
  uint16_t   nWord      ;
  uint16_t   lastId     ;
};
typedef struct Task {     // the forth task
//----------------------------------------------------------------------------------------------------
  char     * tib        ; // terminal input buffer to recieve characters
  int      * DS         ; // data stack
  int      * RS         ; // return stack
  Word    ** CS         ; // temporary word-list (colon definition) to compile
//----------------------------------------------------------------------------------------------------
  char     * iEnd       ; // end of terminal input buffer
  char     * pBgn       ; // forth script (0 ended string) to interpret (parse and eval)
  char     * pEnd       ; // point to forth script remain (0 ended string)
  char     * tokenAt    ; // token found in forth script
  char     * tokenEnd   ; // end of token
  char     * hld        ; // addr to save digit while converting number to digits
//----------------------------------------------------------------------------------------------------
  Word     * context    ; // the last forth word defined in vocabulary.
  Word     * W          ; // running forth word.
  int        base=10    ; // number input/output coversion base
  uint32_t   waitMsUntil; // wait until specified time in ms.
  int      * DP         ; // point to top of data stack
//----------------------------------------------------------------------------------------------------
  Word    ** IP         ; // point to next cell in word-list (colon definition) at run time
  int      * RP         ; // point to top of return stack
  Word     * last       ; // the last forth word defined (may not in vocabulary yet)
  Word    ** CP         ; // point to cell in temporary word-list (colon definition) at compile time
//----------------------------------------------------------------------------------------------------
  int        state      ; // READING, PARSING, CMPLING, CALLING, LOADING
  int        tracing    ; // tracing depth of calling colon type forth word
  int        error      ; // error code 
  int        message=0  ; // no warning message
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////
class FVM;
/////////////////////////////////////////////////////////////////////////////////////////////////////////
class FVM {
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  public:
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
    FVM () {}
    virtual ~FVM () {}
    Voc   * voc;                    // vocabulary of all forth words defined.
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    Task  * T;                      // working task.
    Task  * createTask();           // create task of default sizes
    Task  * createTask(int tib_size, int DS_size, int RS_size, int CS_size);
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    dClear();               // clear data stack
    void    dPush(int);             // push integer on top of data stack
    int	    dPop();                 // pop integer from top of data stack
    int     dPick(int);             // push i-th item of data stack on top (0-th is the top)
    void    dRoll(int);             // roll i-th item of data stack to top (0-th is the top)
    void    dBackRoll(int);         // back roll top of data stack to i-th
    int	    dDepth();               // current depth of data stack
    boolean dHasItems(int);         // check if data stack has given number of items
    boolean dHasSpace(int);         // check if data stack has space for given number of items
    boolean dIsFull();              // check if data stack full
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    rClear();               // clear return stack
    void    rPush(int);             // push integer onto return stack
    int     rPop();                 // pop integer from return stack
    int     rTop(int);              // top i-th integer of return stack
    int     rDepth();               // current depth of return stack
    boolean rHasItems(int);         // check if return stack has given number of items
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    dotS();                 // show data stack
    void    showStacks();           // show return stack and data stack
    void    dot(int);               // print integer.
    void    dotR(int i,int n,char); // print i in n-char wide (fill leading char '0' or ' ').
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    char	toDigit(int);           // convert integer into ascii code as a digit
    char  * toDigits(int, int);     // convert integer into number string of given base.
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    boolean isFloatingNumber(char*);// return 1 if given string is a valid floating number; return 0 otherwise.
    char  * hexPrefix(char*);       // return remain string if prefix 0x or $; return 0 otherwise.
    int     toNumber(char*);        // convert token as a number to integer (or float).
    boolean isNAN();                // return 1 if token converted is not a number; return 0 otherwise.
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    cpInit();               // initializing word list to compile.
    void    compile(Word*);         // compile w into word list.
    Word ** cpClone();              // make a copy of word list as the colon definition of new forth word.
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    Word  * createWord(uint16_t flag, uint16_t id, char*name, FuncP code, int data);// create forth word
	Word  * newPrimitive(char*name, FuncP code, char*codeName);
    Word  * newPrimitive(char*,FuncP);// create forth primitive word of given name and given code.
    Word  * newConstant(char*,int);// create forth constant word of given name and value.
    bool    isWord(Word*);          // check if given forth word is in vocabulary then return 1; otherwise return 0.
    void    showWordType(Word*);    // show type of given foth word
    void    vocInit(Word*);         // link given forth word as the last word in vocabulary
    Word  * vocSearch(char*);       // search the forth word of given name in vocabulary
    void    vocAdd(Word*);          // add given forth word into vocabulary
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    bool    isConstantType(Word*);  // check if given forth word is constant type then return 1; otherwise return 0.
    bool    isValueType(Word*);     // check if given forth word is value type then return 1; otherwise return 0.
    bool    isVariableType(Word*);  // check if given forth word is vareable type then return 1; otherwise return 0.
    bool    isColonType(Word*);     // check if given forth word is colon type then return 1; otherwise return 0.
    bool    isPrimitiveType(Word*); // check if given forth word is primitive type then return 1; otherwise return 0.
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    words(char*);           // show all forth word names including given substring
    void    dump(int*, int);        // dump cells at given address
    void    see(Word *);            // see insight of given forth word
//  void    mountFFS();             // mount flash files system
//  void    fileSystemTest();
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    ms(int);                // wait given number of milli seconds
    void    showTime();             // show current time in format hh:mm:ss.nnn
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    init(long);      		// setup baud rate to communicate via serial port
    void    init(long, Word*);      // setup baud rate to communicate via serial port and word set
    void    update();               // run FVM
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    setCalling();     	    // setup to execute colon definition
    void    callingWrd();           // execute the word list of a colon definition word by word
    void    ipPush();               // push working word and IP to return stack
    void    ipPop();                // pop IP and working word from return stack
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    eval(char*);            // evaluate given script
    void    parsingTkn();           // parse and evaluate token one by one
    char  * uniqueString(char*);    // create a unique string for given token
	char  * parseToken(char);       // parse token by given delimiter (for example, ' ', '"', or ')')
    void    evalToken(char*);	    // evalate token
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    void    initTib();           // begining   of read input line to tib
    void    readTibChar();           // read to tib char by char until '\r'
//----------------------------------------------------------------------------------------------------
//  File    curDir;
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  // all the following are used to compile or decompile forth words
    int     i_doCon, i_doVal, i_doVar, i_doCol, i_ret, i_doNext, i_doLit, i_zbran, i_bran, i_compile;
    Word  * w_doCon, * w_doVal, * w_doVar, * w_doCol,
          * w_ret, * w_doFor, * w_doNext, * w_doLit, * w_doStr, * w_doIf, * w_doElse, * w_doThen,
          * w_doBegin, * w_doAgain, * w_doUntil, * w_doWhile, * w_doRepeat, * w_compile, * redefined=0;
    int     needExtraCell[5]={ i_doLit, i_compile, i_doNext, i_zbran, i_bran };
    char    tmp[TMP_SIZE];          // tmp buffer used in parseToken() and toDigits()
    char  * tmpLimit=tmp+TMP_SIZE-1;//
    int     hint=0; 				// hint message
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  private:
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
    char    SB[SB_SIZE];             // string buffer to hold all unique srings
    char  * sbEnd=SB;                // string buffer end
    int     lineIndex=0;             // index of input line
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    boolean isWhiteSpace(char c);   // check if c is white space
    char  * remain;                 // remain string of number converted token.
};
//////////////////////////////////////////////////////////////////////////
#define LAST 0
#endif _FVM_H_
