// fvm02.cpp
#include <fvm02.h>
#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

extern Word*predefined;
extern Word*firstDefined;
/////////////////////////////////////// forth primitives ////////////////////////////////////////////
// FVM function 01 show the type of given forth word
void FVM::showWordType(Word*w) {
  uint16_t flag=w->flag;
  if(flag&IMMED) PRINT("IMMED ");
  if(flag&COMPO) PRINT("COMPO ");
  if(flag&HIDEN) PRINT("HIDEN ");
  int _code=(int)w->code;
  if(_code==i_doCon) PRINT("constant ");
  else if(_code==i_doVal) PRINT("value ");
  else if(_code==i_doVar) PRINT("variable ");
  else if(_code==i_doCol) PRINT("colon ");
  else PRINT("primitive ");
  PRINT("type word ");
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 02 check if given char (ASCII code) is a white space
boolean FVM::isWhiteSpace(char c){ return c==' '||c=='\t'||c=='\n'||c=='\r'; } // check if c is white space
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 03 check if given token is a floating number
boolean FVM::isFloatingNumber( char *tkn ){ // use /-?(\d+|\d+\.\d*|\.\d+)(e-?\d+)?/ to check tkn
  if( T->base != 10 ) return 0; // base not decimal
  char *p = tkn, c = *p, d = 0;
  if( c == '-' ) c = *(++p);
  while( c>='0' && c <='9' ) d = c, c = *(++p);
  if( c == '.' ){ c = *(++p);
    while( c>='0' && c <='9' ) d = c, c = *(++p);
  }
  if( ! d ) return 0; // no digit in tkn
  if( c ){
  	if( c != 'e' && c != 'E' ) return 0; // illrgal char
  	c = *(++p); d = 0;
  	if( c == '-' ) c = *(++p);
  	while( c>='0' && c <='9' ) d = c, c = *(++p);
  	PRINTF("\nlast digit 0x%x '%c' ", d, d);
  	if( ! d ) return 0; // no digit in Exponential Part
  }
  return 1; }
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 04 converte given token to a number (integer or floating number)
int	FVM::toNumber(char *token){ int sign = 1, n = 0; // assume the number is positive 0
  int b = T->base;
  char *p = remain = token + ( *token < 0x20 ? 1 : 0 ); // adjust token if it is an nString
  remain = hexPrefix(p);
  if( remain ) b = 16; // hexadecimal
  else remain = p; // no hexPrefix
  char c = *remain; // the first char
  if( ! c ) return 0; // null string is not a number
  if( c == '-' ) sign = -1, c = *(++remain); // the number is negative
  if( b == 10 ){ // decimal
    while( c>='0' && c<='9' ) n = n*10+(c-'0'), c = *(++remain); // convert digits to an absolute number
    if( ! c ) n *= sign; // adjust the number if end of token
    else {
      if( isFloatingNumber( p ) ){ String tkn = p; X x; x.f = tkn.toFloat(); n = x.i, remain = ""; } // floating number
    }
    return n; // n is invalid if *remain != 0.
  } else n = strtol(remain, &remain, b); // convert to integer number on base b (*remain is non-digit)
  return n; // n is invalid if *remain != 0.
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 05 check if token converted is not a number (integer or floating number)
boolean FVM::isNAN(){ return *remain != 0; };
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 06 print given integer number and a space
void FVM::dot(int i){ PRINTF("%s ",toDigits(i,T->base)); } // 
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 07 print integer v as n digits with leading c (given ASCII code of 0 or blank)
void FVM::dotR(int v, int n, char c){ // 
  char*s=toDigits(v,T->base); for(int8_t i=strlen(s); i<n; i++) WRITE(c); PRINTF("%s ",s); }
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 08 show data stack
void FVM::dotS(){
  PRINTF("dStk %d [ ",dDepth()); // show depth
  if(dDepth()>0){
    if(dDepth()>5)PRINTF(".. "); // showing at most top 5 items
    for ( int *i=T->DP-4>T->DS?T->DP-4:T->DS; i <= T->DP; i++ ) dot(*i); // show stack
  }
  PRINTF("] base %d ", T->base);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 09 show return stack and data stack
void	FVM::dotId( Word*w ){ // show word's id and name
  PRINTF( "W%03x 0x%x \"%s\" ", w->id, *w->name, w->name+1 );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 09 show return stack and data stack
void	FVM::showStacks(){ // show rack and stack
  PRINT("\n< "), showTime();
  PRINTF("rStk %d [ ",rDepth()); // show depth
  if(rDepth()>0){
    if(rDepth()>5)PRINTF(".. "); // showing at most top 5 items
    for ( int *i=T->RP-4>T->RS?T->RP-4:T->RS; i<=T->RP; i++ ) dot(*i); // show rack
  }
  PRINTF("] ");
  dotS();
  PRINTF("inp%03d >\n", lineIndex++); // show base 
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// FVM function 0a show return stack and data stack
boolean FVM::dIsFull     ()      { return T->DP   >= T->DS+DS_SIZE; } // check if stack full
boolean FVM::dHasSpace (int n) { return T->DP+n <= T->DS+DS_SIZE; } // check if stack has space for n items
boolean FVM::dHasItems (int n) { return T->DP   >= T->DS+n-1    ; } // check if stack has n items
void    FVM::dClear    ()      { T->DP=T->DS-1 ; } // reset stack
void    FVM::dPush     (int n) { *(++(T->DP))=n; } // push integer onto stack
int     FVM::dPop      ()      { return *(T->DP--) ; } // pop integer from stack
int     FVM::dPick     (int i) { return *(T->DP-i) ; } // top i-th integer of stack
void    FVM::dRoll     (int n) { int *p=T->DP-n, X=*p; while(++p <= T->DP) *(p-1)=*p; *(p-1)=X; } // roll top n integers on stack
void    FVM::dBackRoll (int n) { int *p=T->DP, X=*p; while(--p >= T->DP-n) *(p+1)=*p; *(p+1)=X; } // back roll top n integers of stack
int     FVM::dDepth    ()      { return T->DP-T->DS+1; } // depth of stack
/////////////////////////////////////////////////////////////////////////////////////////////////////
boolean FVM::rHasItems (int n) { return T->RP   >= T->RS+n-1    ; } // check if rack has n items
void    FVM::rClear    ()      { T->RP=T->RS-1 ; } // reset rack
void    FVM::rPush     (int n) { *(++(T->RP))=n; } // push integer onto rack
int     FVM::rPop      ()      { return *(T->RP--) ; } // pop integer from rack
int     FVM::rTop      (int i) { return *(T->RP-i); } // top integer of rack
int     FVM::rDepth    ()      { return T->RP-T->RS+1; } // depth of rack
/////////////////////////////////////////////////////////////////////////////////////////////////////
char    FVM::toDigit   (int x) { return x+(x<10?0x30:0x57); } // convert integer x to digit char, for example, 10 to 'a'.
/////////////////////////////////////////////////////////////////////////////////////////////////////
char*   FVM::toDigits (int x, int b) { // convert integer x into digits in given base b
  char*p = T->hld = tmp+TMP_SIZE-1; *p = 0; // setup to convert number
  if(x==0){ *(--p)='0'; T->hld = p; return p; }
  boolean neg = ((b==10) && (x<0)); if( neg ) x = -x;
  uint u = x;
  while(u) *(--p) = toDigit( u % b ), u /= b; // convert current digit to character
  if( neg ) *(--p) = '-';
  T->hld = p; return p;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
char*	  FVM::hexPrefix(char *s) { // 0xff 0XFF $ff $FF $Ff are all acceptable as hexadecimal numbers
  char c;
  if((c=*s++) != '0' && c != '$') return 0;
  if(c=='0' && (c=*s++) != 'x' && c != 'X') return 0;
  return s; // remain string
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
char*   FVM::uniqueString(char *s) { // save s in the string buffer and return nString address p
  char *p=SB+1;                 // let p point to the first string
  uint8_t n=strlen(s+1);
  while (p<sbEnd) {             // if p is not pointing to end of the string buffer
    if(strcmp(p,s+1)==0){
      return p-1;// return p if s is already in the string buffer
    }
    p+=*(p-1)+2;                // let p point to the next string
  }
  if (n > SL_LIMIT) {           // if string length over limit
    ABORT( T->error, 1, initTib, "uniqueString too long, \nThe string \"%s\"\nlength %d > limit %d", s, n, SL_LIMIT);
    return 0;
  }
  if(sbEnd+n >= SB_LIMIT){      // if the string buffer full
    ABORT( T->error, 2, initTib, "uniqueString too long, \nThe string \"%s\"\nlength %d but string buffer size %d but remain only %d", s, SB_SIZE, SB_LIMIT-sbEnd);
    return 0;
  }
  p=sbEnd;                      // p is pointing to the the string copy
  *sbEnd++=n;                   // the gap count for going to next string
  strcpy(sbEnd,s+1);              // append s to the string buffer
  sbEnd+=n+1;                   // advance the end of the string buffer
  return p;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
Word* FVM::createWord(uint16_t flag, uint16_t id, char*name, FuncP code, int data){
  // create a forth word of name n with the function code f to execute
    char*u;
    if( *name >= 0x20 ){
    	*tmp = strlen( name ), strcpy( tmp+1, name );
    	name = tmp;
    }
    Word *w=vocSearch(name);
    if(w) PRINTF("\nwarning !!! 0x%x \"%s\" reDef ",*name,name+1), u=w->name;
    else u=uniqueString(name);
    T->last=w=(Word*)malloc(sizeof(Word));
    w->flag=flag, w->name=u, w->code=code;
    w->p.con=data;
    if(isColonType(w)!=1) vocAdd(w);
    return w;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
Word * FVM::newPrimitive(char*name, FuncP code, char*codeName){
  createWord( 0, 0, name, code, (int) codeName ); }
/////////////////////////////////////////////////////////////////////////////////////////////////////
Word * FVM::newPrimitive(char*name, FuncP code){
  newPrimitive( name, code, *name < 0x20 ? (name+1) : name ); }
///////////////////////////////////////////////////////////////////////////////////////
Word * FVM::newConstant(char*name, int i){
createWord( 0, 0, name, (FuncP)i_doCon, i ); }
///////////////////////////////////////////////////////////////////////////////////////
Task * FVM::createTask( int sizeTib, int sizeDS, int sizeRS, int sizeCS ){
  int bytesForTask = sizeof(Task), bytesForTib = sizeTib, bytesForDS = sizeDS*4, bytesForRS = sizeRS*4, bytesForCS = sizeCS*4;
  int bytesTotal = bytesForTask + bytesForTib + bytesForDS + bytesForRS + bytesForCS;
  T = (Task *) malloc(bytesTotal);
  if(hint) PRINTF("\ncreate task 0x%x alloc %d-byte \n", T, bytesTotal);
  int a = (int)T; a += bytesForTask;
  T->tib = (char *) a; a += bytesForTib;
  T->DS = (int *) a; a += bytesForDS;
  T->RS = (int *) a; a += bytesForRS;
  T->CS = (Word **) a, T->base = 10, T->error = 0, T->tracing = 0;
  return T;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
Task * FVM::createTask(){ return createTask( TIB_SIZE, DS_SIZE, RS_SIZE, CS_SIZE ); }
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool    FVM::isWord (Word*w) { // asure that w is a word
    Word*x=voc->context; while(x){ if((int)x==(int)w) return 1; x=x->link; } return 0; }
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::cpInit () { T->CP=T->CS; } // initialize temporary compile space
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::compile (Word*w) { // compile w into temporary compile space
  if(T->tracing) PRINTF("%x:%08x ", T->CP, w); *((T->CP)++)=w; }
/////////////////////////////////////////////////////////////////////////////////////////////////////
Word**  FVM::cpClone() {              // cloning temporary compile space to a wplist
    int n=(T->CP)-(T->CS), m=n*sizeof(Word*);
    Word** wplist=(Word**)malloc(m);
    if(T->tracing) PRINTF("\ncpClone() malloc(%d) at 0x%x ", m, wplist);
    memcpy(wplist,T->CS,m);
    if(T->tracing) dump((int*)wplist,n);
    return wplist;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void FVM::vocInit (Word*wrd) {
  Word *w=wrd, *first;
  voc = (Voc*)malloc(sizeof(Voc));
  voc->context = T->last = wrd; // initialize vocabulary (dictionary)
  voc->nWord=0;
  while(w) voc->nWord++, first=w=w->link;
  voc->first = first;
  voc->lastId=T->last ? T->last->id : 0;
#ifdef TRACING
  PRINTF("\n%d forth words predefined", voc->nWord);
  if(voc->nWord)
  	  PRINTF(", the first is 0x%x \"%s\", the last is 0x%x \"%s\" ",
		     voc->nWord, *(first->name), first->name+1, *(T->last->name), T->last->name+1);
#endif
  w_doCon=vocSearch("\x05" "(con)"); if(w_doCon) i_doCon=w_doCon->p.con;
  w_doVal=vocSearch("\x05" "(val)"); if(w_doVal) i_doVal=w_doVal->p.con;
  w_doVar=vocSearch("\x05" "(var)"); if(w_doVar) i_doVar=w_doVar->p.con;
  w_doCol=vocSearch("\x03" "(:)"  ); if(w_doCol) i_doCol=w_doCol->p.con;
  w_ret  =vocSearch("\x03" "(;)"  ); if(w_ret  ) i_ret=(int)w_ret->code;
#ifdef TRACING
  PRINTF("\nw_doCon=0x%x, w_doVal=0x%x, w_doVar=0x%x, w_doCol=0x%x, ",w_doCon, w_doVal, w_doVar, w_doCol);
  PRINTF("\ni_doCon=0x%x, i_doVal=0x%x, i_doVar=0x%x, i_doCol=0x%x, ",i_doCon, i_doVal, i_doVar, i_doCol);
  PRINTF("\nw_ret=0x%x, i_ret=0x%x ", w_ret, i_ret);
#endif
  w_doBegin =vocSearch("\x07" "(begin)" );
  w_doAgain =vocSearch("\x07" "(again)" );
  w_doUntil =vocSearch("\x07" "(until)" );
  w_doWhile =vocSearch("\x07" "(while)" );
  w_doRepeat=vocSearch("\x08" "(repeat)");
  w_doThen  =vocSearch("\x06" "(then)"  );
  w_doFor   =vocSearch("\x05" "(for)"   );
  w_doNext  =vocSearch("\x06" "(next)"  ); needExtraCell[0]=i_doNext = w_doNext ? (int)w_doNext ->code : 0;
  w_doIf    =vocSearch("\x04" "(if)"    ); needExtraCell[1]=i_zbran  = w_doIf   ? (int)w_doIf   ->code : 0;
  w_doElse  =vocSearch("\x06" "(else)"  ); needExtraCell[2]=i_bran   = w_doElse ? (int)w_doElse ->code : 0;
  w_doLit   =vocSearch("\x05" "(lit)"   ); needExtraCell[3]=i_doLit  = w_doLit  ? (int)w_doLit  ->code : 0;
  w_compile =vocSearch("\x07" "compile" ); needExtraCell[4]=i_compile= w_compile? (int)w_compile->code : 0;
#ifdef TRACING
  PRINTF("\nw_doNext=0x%x, w_doIf=0x%x, w_doElse=0x%x, w_doLit=0x%x, w_compile=0x%x ", w_doNext, w_doIf, w_doElse, w_doLit, w_compile);
  PRINTF("\ni_doNext=0x%x, i_zbran=0x%x, i_bran=0x%x, i_doLit=0x%x, i_compile=0x%x ", i_doNext, i_zbran, i_bran, i_doLit, i_compile);
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void FVM::vocAdd (Word *w) { // add forth word w into vocabulary
  w->id = ++(voc->lastId); w->link = voc->context; voc->context=w;
  if(hint){
    PRINTF("\n%03d W%03x 0x%x \"%s\" new ",voc->nWord++, w->id, *(w->name), w->name+1 );
    showWordType( w );
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
Word *FVM::vocSearch (char *token) { // check if given token is a name of forth word w then return w
  Word *w=0;
  if(token){ char n=*token;
    if(n){
      if(n>=0x20){
        ABORT( T->error, 3, initTib, "illegal token, vocSearch(\"\\x%x\" \"%s\") ", *token, token+1 ); return 0;
      }
      w = voc->context;
      while ( w && strcmp(w->name,token) ){
        w=w->link;
      }
    }
  }
  return w;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::words(char*sub) { // show all word names having specific substring
  char *s=sub; if(s && *s<0x20) s++;
//PRINT("show words");
//if(sub && *sub)PRINTF(" those names including substring 0x%x \"%s\" ", *sub, sub+1);
//PRINT(":\n");
  char m=0, n; int i=voc->nWord;
  Word* w=voc->context;
  while (w) {
    if( s==0 || *s==0 || strstr(w->name+1,s) ){
      if( !(w->flag&HIDEN) ){
        m+=strlen(w->name)+5;
        if(m>CONSOLE_WIDTH) WRITE('\n'), m=0;
        PRINTF("W%03x %s ",w->id,w->name+1);
      }
    }
    w=w->link, i--; 
  }
  WRITE('\n');
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::dump(int *a,int n) { // dump n cells at adr
    int *lmt=a+n;
    char *ba;
    char *blmt;
    for( ; a<lmt; a+=4) { 
      PRINTF("\n%8.8x : ", (int) a);
      for(int i=0; i< 4; i++){
        if( a+i>=lmt )PRINTF("         ");
        else          PRINTF("%8.8x ", *( a+i));
      }
      PRINTF(": ");
      ba=(char*)a, blmt=(char*)lmt;
      for(int i=0; i<16; i++){
        if(ba+i>=blmt)PRINTF("   ");
        else          PRINTF("%2.2x ", *(ba+i));
      }
      PRINTF(": ");
      for(int i=0; i<16; i++){
        if(ba+i>=blmt)PRINTF(" ");
        else {
          char c=*(ba+i); n=(int)c;
          if( n==0 ) c='.';
          else if( n<0x20 || (n>0x7e&&n<0xa4) || n>0xc6 ) c='_';
          else if(n>=0xa4&&n<=0xc6) { // head-byte of commmon big5
            n=(int)*(ba+i+1);
            if( n<0x40 || (n>0x7e&&n<0xa1) || n>0xfe) c='_'; // next is not tail-byte of commmon big5 
            else PRINTF("%c",c), c=(char)n, i++; // show head-byte of commmon big5 and get tail-byte
          }
          PRINTF("%c",c);
        }
      }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/*
void FVM::mountFFS(){
  if( ! SPIFFS.begin(-1) ){
  	ABORT( T->error, 4, initTib, "SPIFFS Mount Failed ", 0);  // added by derek for internal flash file system 
  	return;
  }
  if(hint) PRINTF( "\nSPIFFS totalBytes %d usedBytes %d\n", SPIFFS.totalBytes(), SPIFFS.usedBytes() );
  char *path = "/";
  File dir = SPIFFS.open( path, FILE_READ );
  if( ! dir ){ PRINTF("\nno SPIFFS directory yet "); return; }
  curDir = dir;
  if(hint) PRINTF( "\nSPIFFS current directory %s ", dir.name() );
//fileSystemTest();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void FVM::fileSystemTest(){
  if( ! curDir ){ PRINTF("\nno SPIFFS directory "); return; }
  PRINTF( "\nshow directory %s ", curDir.name() );
  int nfiles=0, dirsize=0, totalsize=0, filesize;
  char filename[20],dirname[20];
  File curfile;
  while( curfile = curDir.openNextFile(FILE_READ) ) {
    strcpy(filename,curfile.name());
    char *p=strchr(filename+1,'/');
    if(p){
      if(!nfiles){ *p=0; strcpy(dirname,filename); }
      nfiles++, filesize=curfile.size(); dirsize+=filesize, totalsize+=filesize;
    } else {
      if(nfiles){
        PRINTF("\n   <DIR> %s %d files %d (0x%x) bytes ",dirname,nfiles,dirsize,dirsize);
        dirsize=nfiles=0;
      }
    //PRINTF(" 0x%x %s %d %c\n",curfile.size(),filename,p-filename,*p);
    }
  	PRINTF( "\n%8d %s ", curfile.size(), curfile.name() );
  }
  if(nfiles) PRINTF( "\n%8d %s <DIR> %d files ", dirsize, dirname, nfiles );
}
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////
const bool any(int w, int*ws, int8_t n){ while(n--){ if(w==ws[n]) return 1; } return 0; }
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool FVM::isConstantType(Word*w){ return w?w->code==(FuncP)i_doCon:0; }
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool FVM::isValueType(Word*w){ return w?w->code==(FuncP)i_doVal:0; }
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool FVM::isVariableType(Word*w){ return w?w->code==(FuncP)i_doVar:0; }
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool FVM::isColonType(Word*w){ return w?w->code==(FuncP)i_doCol:0; }
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool FVM::isPrimitiveType(Word*w){
  if(isConstantType(w)) return 0;
  if(isValueType(w)) return 0;
  if(isVariableType(w)) return 0;
  if(isColonType(w)) return 0;
  return 1;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void FVM::see(Word *w) { // show the forth word
  if(! isWord(w)){
    ABORT( T->error, 5, initTib, "not forth word to see ", w); return; }
  int _code=(int)w->code; char *name, n; uint16_t flag;
  PRINTF("\n----------------------");
  Word *link = w->link;
  PRINTF("\n%x:%08x link ", &w->link, link);
  if(link) PRINTF("to W%03x ", link->id);
  PRINTF("\n%x:%04x%04x flag ",&w->id, w->flag, w->id);
  showWordType(w);
  PRINTF("W%03x ", w->id);
  name=w->name, n=*name;
  PRINTF("\n%x:%08x name 0x%x \"%s\" " , &w->name, name, *name, name+1);
  PRINTF("\n%x:%08x code ", &w->code ,_code );
  if( _code ){
    if(_code==i_doCon) PRINT("_doCon ");
    else if(_code==i_doVal) PRINT("_doVal ");
    else if(_code==i_doVar) PRINT("_doVar ");
    else if(_code==i_doCol) PRINT("_doCol ");
    else PRINTF("%s ", w->p.mne);
  }
  int parm=w->p.con;
  PRINTF( "\n%x:%08x parm ", &(w->p.con), parm );
  if( parm ){
         if(parm==i_doCon) PRINT("_doCon ");
    else if(parm==i_doVal) PRINT("_doVal ");
    else if(parm==i_doVar) PRINT("_doVar ");
    else if(parm==i_doCol) PRINT("_doCol ");
    else if(_code!=i_doCon && _code!=i_doVal && _code!=i_doVar  && _code!=i_doCol) PRINTF("\"%s\" ", w->p.mne);
  }
  PRINTF("\n----------------------");
  if (_code && _code==i_doCol){
    int x_code;
    Word **ip=w->p.wpl, *x;
    do {
       x=*ip; x_code=(int)x->code;
       PRINTF("\n%x:%08x %s%s ",ip++,x,x->flag==IMMED?"[compile] ":"",x->name+1);
       if( any( x_code, needExtraCell, 5 ) ){
          Word*z=*ip;
          PRINTF("\n%x:%08x ",ip++,z);
          if( x_code==i_compile ) PRINTF("%s ",z->name+1); // word of compile
          else{
            if( 0x74732805==*(int*)(x->name) ){
              char*p=(char*)z; PRINTF("0x%x \"%s\" ", *p, p+1);
            } // string of (str)
            else PRINTF("%d ",z);
          }
       }
    } while ( x != w_ret );
    PRINTF("\n----------------------\n");
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::ms(int n){ T->waitMsUntil=millis()+n-1; }  // set to wait n ms (milli seconds)
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::callingWrd () { // execute word one by one in the wplist of a colon word
  if( millis() <= T->waitMsUntil ) return; // wait up to given time
  if ( ! (T->IP) ){
  //PRINT("\ncallingWrd() end of full calling");
    return; // end of full calling
  }
  T->W=*(T->IP)++; // from IP, get a word to execute
  if(T->W->code == 0) return;
  if ( T->tracing ) {
    int8_t nd=(T->DP)-(T->DS)+1, nr=(T->RP)-(T->RS)+1, n=nr/2, i;
    PRINTF("\nR %d [ ", nr);
    if(nr>=2) PRINTF("%08x ", *(T->RP-1)); else PRINT("........ ");
    if(nr>=1) PRINTF("%08x ", *(T->RP  )); else PRINT("........ ");
    PRINTF("] D %d [ ", nd);
    if(nd>=2) PRINTF("%08x ", *(T->DP-1)); else PRINT("........ ");
    if(nd>=1) PRINTF("%08x ", *(T->DP  )); else PRINT("........ ");
    PRINTF("] %x:%x ",T->IP-1,T->W);
    for(i=0; i<n; i++) PRINTF("| ");
    PRINTF("%s ", T->W->name+1);
  }
  (T->W)->code(); // execute the forth word W
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::ipPush(){ rPush((int)(T->IP)); rPush((int)(T->W)); }
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::ipPop (){ T->W=(Word*)rPop(); T->IP=(Word**)rPop(); }
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::eval (char *str) {
  char *b=str, *p=b, *e;
  while( e = strstr(p,"  ") ){
  	while( b<e ) WRITE(*b++); WRITE('\n'), e++; p=b=e;
  	while( *p && *p==' ' ) p++;
  }
  while( *b ) WRITE(*b++);
  PRINT("\n"); 
  int n=strlen(str); char *pLmt;
  T->pBgn = T->pEnd = str, pLmt = str+n, T->state |= PARSING;
  if ( T->tracing ) PRINTF("\n000 eval %d-char pBgn=0x%x pLmt=0x%x \n", n, T->pBgn, pLmt);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::parsingTkn() { // parse token to evaluate
  if( millis() <= T->waitMsUntil ) return; // wait up to given time
  if( T->state == 0 ) return;
  if( T->IP )       return;
  if( *(T->pEnd)==0 ) { initTib(); return; }  // end of parsing
  char *token = parseToken(' ');              // get a token
  if( ! *token ){                  return; }  // end of script ( need to work for fload )
  if( T->tracing ) PRINTF("\n%03d parsingTkn \"\\x%02x\" \"%s\" ", T->tokenAt-T->pBgn, *token, token+1);
  evalToken( token );                     		  // evaluate token
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::evalToken( char *token ) { // evaluate nString token
//tracing=1;
  T->W = vocSearch( token );                     // search token in vocabulary
  if ( T->W ) {                                  // found word W
    if(T->W->code == 0) return;
  	uint16_t flag = T->W->flag;
    if( ( flag&IMMED ) // (word W not compile only and state not compilling) or
      ||( ((T->state) & CMPLING)==0 ) ) {                    // word W immediate
    if( (flag&COMPO)&&((T->state) & CMPLING)==0 ) { 
      ABORT( T->error, 6, initTib, "cannot execute compile-only type word 0x%x \"%s\" ", *(T->W->name), T->W->name+1); return; }
      if( T->tracing ) PRINT( "execute " );
      T->W->code();							  // execute word W
    }
    else {
      if( T->tracing ) PRINTF( "compile " );
      compile( T->W );                   		  // compile word W
    }
    if( T->tracing ){
      showWordType(T->W);
    }
    return;
  }
  int n = toNumber( token );                  // convert token to number
  if( isNAN() ){
  	ABORT( T->error, 7, initTib, "unDef 0x%x \"%s\" ", *token++, token ); // while(*token) PRINTF("%02x ",*token++);
    return;
  }
  if( ((T->state) & CMPLING)==0 ){                   // state not compilling
  	if( T->tracing ) PRINT( "push " );
  	dPush(n);                                // push number to stack
  } else {
    if( T->tracing ) PRINT("compile ");
    compile(w_doLit), compile((Word*)n);   // compile (lit) and number n
  }
  if( T->tracing ) PRINTF("0x%x %d ", n, n);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
char*   FVM::parseToken (char delimiter){ // parse token by given delimiter (if delimiter=0x20, take white space as delimiter)
  char *p, i, n, c=*(T->pEnd);
  if(c==0) return 0;
  if(delimiter==0x20){
    while ( c && isWhiteSpace(c) ) c = *(++(T->pEnd));   // ignore leading white spaces
    T->tokenAt = T->pEnd;                       // token found at first non white space
    while ( c && !isWhiteSpace(c) ) c = *(++(T->pEnd));  // colect non white spaces
    T->tokenEnd=T->pEnd;
  } else {
    if( isWhiteSpace(c) ) T->pEnd++;   // skip leading white space
    T->tokenAt=T->pEnd;
    do { T->tokenEnd=strchr(T->pEnd, delimiter); T->pEnd=T->tokenEnd+1; c=*(T->pEnd);
    } while( c && !isWhiteSpace(c) );
  }
  n = T->tokenEnd - T->tokenAt;               // compute token length
  if ( n>=TMP_SIZE-1 ) {                      // check if length too long
    ABORT( T->error, 8, initTib, "token length too long, token length %d >= %d\n",n,TMP_SIZE-1);
    return "";
  }
  strncpy(&tmp[1],T->tokenAt,n), tmp[0]=n, tmp[n+1] = 0;         // make a null ended string at tmp
  char*u=uniqueString(tmp);        // make a null ended string at tmp
  return u;                  // return a unique nString in string buffer
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::showTime(){
  int ms=millis(), s=ms/1000, m=s/60, h=m/60;  ms=ms%1000, s=s%60, m=m%60;
  PRINTF("%02d:%02d:%02d.%03d ", h, m, s, ms);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void    FVM::initTib() { // set to input char from serial port
  T->iEnd = T->tib, T->state = T->waitMsUntil = 0, T->IP = 0;
  if(hint){
	  showStacks();     // showing depth, numbers, and number coversion base of stack 
  } else PRINTF(" OK ");
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void FVM::readTibChar () { // input char from serial port
  if ( (T->state)&PARSING ) return;
  if ( !AVAILABLE() ) return;     // serial port not ready
  char c = READ(); // read a char from serial port
//if ( c == '\b' ) {              // back space
//  if ( T->iEnd > T->tib ) PRINT("\b \b"), T->iEnd--; // erase last character
//  return;   
//}
  if ( c == '\r' ) { // carriage return or line feed
    *(T->iEnd) = 0; // append end of input
    eval(T->tib); return;        // interpret tib
  }
  if ( T->iEnd >= T->tib+TIB_SIZE-1 ) { ABORT( T->error, 9, initTib, "tib full, max length %d ",TIB_SIZE-1); return; }
  *(T->iEnd++) = c;          // append c into tib
//WRITE(c); // echo c
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void FVM::update(){ readTibChar(), parsingTkn(), callingWrd(); }
/////////////////////////////////////////////////////////////////////////////////////////////////////
void FVM::init( long baud, Word* word_set ){
  Serial.begin( baud );
  PRINTF("==================================================\n");
  PRINTF("     Wifi Boy ESP32 forth virtual machine 0.2     \n");
  PRINTF("20190112 derek@wifiboy.org & samsuanchen@gmail.com\n");
  PRINTF("==================================================\n");;
  sbEnd=SB;
  T = createTask();
  T->DP = T->DS-1;           // clearing data   stack
  T->RP = T->RS-1;           // clearing return stack
  T->error = 0;
  vocInit( word_set );
  initTib();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void FVM::init( long baud ){
  init( baud, 0 );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

