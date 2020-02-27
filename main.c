#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "functions.h"
#include "macros.h"

/* 
 TODO:

  add hashed password to outfile
  add debug statements in functions.c
  cleaup functions.c comments/code
  why isnt debug() working in functions.c?
  

  preserve outfile on failure?
  same in/out file?
  relative pathnames / absolute ones?


*/

/* Debug value 0x01 (1d): print a message on immediate entry and right before
  exit to every function in your code that you write, including main(), but
  not library calls you call.  Print the name of the function and whether
  you're entering or exiting it.

- Debug value 0x02 (2d): print right before and right after calling any
  library call (e.g., from libc, libssl, etc.).  Print the function name and
  whether you're before or after calling it.

- Debug value 0x04 (4d): print right before and right after calling any
  system call (e.g., open, read, write, close).  Print the syscall name and
  whether you're before or after calling it.

- Debug value 0x10 (16d): print also arguments to any function for debug
  values 0x1 (upon entry), 0x2 (right before), and 0x4 (right before).

- Debug value 0x20 (32d): print also return values (and errors if any) for
  any function for debug values 0x1 (right before return), 0x2 (right
  after), and 0x4 (right after). 
  */
int main(int argc, char** argv)
{  
  int bufsize = getpagesize(); // probably 4K
  long dbg_input = 0;
  int len = bufsize;
  void *buf = NULL, *preservebuf = NULL;
  char *infile = NULL, *outfile = NULL, *password = NULL, *pass2 = NULL;
  int fd1 = -1, fd2 = -1, fd3 = -1, fd4 = -1, retval = 0;
  int infile_stdin = -1, outfile_stdin = -1;
  int passfile = -1, safeflag = -1;
  int crypt = -1;
	unsigned char *key = NULL;

  int opt;
  extern char *optarg;
  extern char *optarg;
  extern int optind, opterr, optopt;
  
  while((opt = getopt(argc, argv, ":p:hvD:eds")) != -1)  
  {  
    switch(opt)  
    {  
      case 's':
          safeflag = 1;
          debug("safe flag provided. prompt for pass twice\n");
          break;
      case 'p':  
          password = optarg;
          break;  
      case 'h':  
          fprintf(stderr, "error: a simple usage line on stderr and exit with a non-zero status code.\n"); 
          retval = 14;
          goto out; 
          break;  
      case 'v':  
          fprintf(stderr, "Version: %s\n", VERSION);  
          break; 
      case 'e':  
          if(crypt == -1)
            crypt = 1;
          else{
            fprintf(stderr, "Cannot decrpt if encrypt option is provided\n");
            retval = 13;
            goto out;
          }
          
          break;   
      case 'D':
      {  
          char* test;
          dbg_input = strtol(optarg, &test, 0);          
          break;
      }
      case 'd':  
          if(crypt == -1)
            crypt = 2;
          else{
            fprintf(stderr, "Cannot encrypt if decrypt option is provided\n");
            retval = 12;
            goto out;
          }

          break;    
      case '?':  
          fprintf(stderr, "unknown option: %c\n", optopt); 
          retval = 11;
          goto out; 
    }  
  }  
  DBG_ENTEXIT(dbg_input,"ENTEXIT: Entering main() in main.c\n");
  if(optind < argc)
  {
    for(int j = optind; j < argc; j++)
    {
      if(strcmp("-", argv[j]) == 0)
      {
        if(optind == j)
          infile_stdin = 0;
        else
          outfile_stdin = 0;
      }
      else 
      {
        if(optind == j)
          infile = argv[j];
        else /* if (optind+1 == j) */
          outfile = argv[j];
      }
    }
  }
  else
  {
    fprintf(stderr, "No input file provided\n");
    retval = 10;
    goto out;
  }

  debug("Infile: %s Outfile: %s\n", infile ? infile : "STDIN", outfile ? outfile : "STDOUT");

  if(crypt == 1)
    debug("Encrypt with specified user pw\n");
  else if(crypt == 2)
    debug("Decrypt with specified user pw\n");
  else
    {
      retval = 99;
      dbg("Either encryption or decryption must be selected.\n");
      goto out;
    }


  if(password)
  {
    debug("reading from passfile\n");
    passfile = 1;
  }
   else{
    DBG_LIB(dbg_input,"LIB: Before getpass() \n");
    password = getpass("Enter password:"); 
    DBG_LIB(dbg_input,"LIB: After getpass() \n");
    debug("password: %s, strlen: %ld\n", password, strlen(password));
    if(safeflag == 1 && EC == 1)
      {
        int length;
        char *pwcopy = NULL;
        DBG_LIB(dbg_input,"LIB:Before strlen() \n");
        if((dbg_input & 0x10) && (dbg_input & 0x02))
          fprintf(stderr, "ARGS: strlen-password:%s\n", password);
        length = strlen(password) + 1;
        DBG_LIB(dbg_input,"LIB:After strlen()\n");
        DBG_LIB(dbg_input,"LIB:Before malloc \n");
        pwcopy = (char *)malloc(sizeof(char) * length);
        DBG_LIB(dbg_input,"LIB:After malloc \n");
        DBG_LIB(dbg_input,"LIB:Before strcpy \n");
        if((dbg_input & 0x10) && (dbg_input & 0x02))
          fprintf(stderr, "ARGS: strcpy-pwcopy:%s, password: %s\n", pwcopy, password);
        strcpy(pwcopy, password);
        DBG_LIB(dbg_input,"LIB:After strcpy\n");
        if((dbg_input & 0x20) && (dbg_input & 0x02))
          fprintf(stderr, "RET: strcpy-pwcopy:%s, password: %s\n", pwcopy, password);
        DBG_LIB(dbg_input,"LIB:Before getpass (confirmation)\n");
        pass2 = getpass("Confirm password:");
        DBG_LIB(dbg_input,"LIB:After getpass (confirmation)  \n");
        if((dbg_input & 0x10) && (dbg_input & 0x02))
          fprintf(stderr, "RET: getpass: %s\n", pass2);
        debug("pass2: %s, strlen: %ld\n", pass2, strlen(pass2));
        DBG_LIB(dbg_input,"LIB:Before strcmp (two passwords) \n");
        DBG_LIB(dbg_input,"LIB:Before free (pwcopy)  \n");
        if((dbg_input & 0x10) && (dbg_input & 0x02))
          fprintf(stderr, "ARGS: strcmp-pwcopy: %s, pass2:%s\n", pwcopy, pass2);
        if(strcmp(pwcopy, pass2) == 0)
        {
          debug("passwords match\n");
          if(pwcopy != NULL)
            free(pwcopy);
        }
        else
        {
          if(pwcopy != NULL)
            free(pwcopy);
          fprintf(stderr, "Passwords do not match. Exiting.\n");
          retval = 30;
          goto out;
          
        }
        DBG_LIB(dbg_input,"LIB: After free (pwcopy) \n");
        DBG_LIB(dbg_input,"LIB: After strcmp (two passwords) \n");
      }
    
      
  } 

  // alloc a buffer
  DBG_LIB(dbg_input,"LIB: Before Malloc \n");
  buf = malloc(bufsize);
  if (buf == NULL) {
    perror("malloc");
    retval = 1; // indicates to caller that malloc failed
    goto out;
  }
  DBG_LIB(dbg_input,"LIB: After Malloc \n");
  if((dbg_input & 0x20) && (dbg_input & 0x02))
    fprintf(stderr, "RET: malloc-malloc != NULL:%d\n", buf != NULL);
  DBG_LIB(dbg_input,"LIB: Before Malloc  \n");
  preservebuf = malloc(bufsize);
  if (preservebuf == NULL) {
    perror("malloc");
    retval = 5;
    goto out;
  }
  DBG_LIB(dbg_input,"LIB: After Malloc \n");
  if((dbg_input & 0x20) && (dbg_input & 0x02))
    fprintf(stderr, "RET: malloc-malloc != NULL:%d\n", preservebuf != NULL);
  
  /* ################## opening files ########################### */

  // open infile
  if(infile_stdin == -1)//not reading from stdin
  {
    DBG_SYSCALL(dbg_input, "SYSCALL: Before open() (infile)\n");
    if((dbg_input & 0x10) && (dbg_input & 0x02))
      fprintf(stderr, "ARGS: infile: %s\n", infile);
    fd1 = open(infile, O_RDONLY);
    DBG_SYSCALL(dbg_input, "SYSCALL: After open() (infile)\n");
    if((dbg_input & 0x20) && (dbg_input & 0x02))
      fprintf(stderr, "RET: infile: %s\n", infile);

    if (fd1 < 0) {
      perror(infile);
      retval = 2; // indicates to caller that open infile failed
      goto out;
    }
  }

  // open outfile
  if(outfile_stdin == -1) //not reading from stdin
 { 
    DBG_SYSCALL(dbg_input, "SYSCALL: Before open() (outfile)\n");
    if((dbg_input & 0x10) && (dbg_input & 0x02))
      fprintf(stderr, "ARGS: outfile: %s\n", outfile);
    fd2 = open(outfile, O_RDWR|O_CREAT, 00700);
    DBG_SYSCALL(dbg_input, "SYSCALL: After open() (outfile)\n");
    if((dbg_input & 0x20) && (dbg_input & 0x02))
      fprintf(stderr, "RET: outfile: %s\n", outfile);
    if (fd2 < 0) {
      perror(outfile);
      retval = 3; // indicates to caller that open outfile failed
      goto out;
    }
  }

  //create/open tempfile
  DBG_SYSCALL(dbg_input, "SYSCALL: Before open() (tempfile)\n");
  if((dbg_input & 0x10) && (dbg_input & 0x02))
    fprintf(stderr, "ARGS: preserve-outfile: %s\n", "preserve-outfile");
  fd3 = open("preserved-outfile", O_CREAT|O_WRONLY, 00700 );
  if((dbg_input & 0x20) && (dbg_input & 0x02))
    fprintf(stderr, "RET: preserve-outfile: %d\n", fd3);

  DBG_SYSCALL(dbg_input, "SYSCALL: After open() (tempfile)\n");
  if (fd3 < 0) {
    perror("tempfile");
    retval = 4; //indicates to caller that temp file failed
    goto out;
  }

  if(passfile == 1)
  {
    debug("opening passfile\n");
    //open passfile
    DBG_SYSCALL(dbg_input, "SYSCALL: Before open() (passfile)\n");
    if((dbg_input & 0x10) && (dbg_input & 0x02))
      fprintf(stderr, "ARGS: passfile: %s\n", password);
    fd4 = open(password, O_RDONLY);
    if((dbg_input & 0x20) && (dbg_input & 0x02))
      fprintf(stderr, "RET: passfile: %s\n", password);
  
    DBG_SYSCALL(dbg_input, "SYSCALL: After open() (tempfile)\n");
    if (fd4 < 0) {
      perror("password file");
      retval = 20; //indicates to caller that pass file failed
      goto out;
    }
  }

  int blocksz;
  /* ################## passfile read ########################### */
  if(passfile == 1) 
  { 
    DBG_SYSCALL(dbg_input, "SYSCALL: Before read() (passfile)\n");
    if((dbg_input & 0x10) && (dbg_input & 0x04))
      fprintf(stderr, "ARGS: read-fd:%d, len: %d\n", fd4, len);
    while((blocksz = read(fd4, buf, len)) > 0)
    {
      if (blocksz < 0) { // failed
        debug("password read failed blocksz: %d\n", blocksz);
        retval = 21;
        goto out;
      }     
    } 
    DBG_SYSCALL(dbg_input, "SYSCALL: After read() (passfile)\n");
    if((dbg_input & 0x20) && (dbg_input & 0x04))
      fprintf(stderr, "RET: read-blocksz: %d \n", blocksz);
    char *c = strchr(buf, '\n');
    if (c){
      debug("newline character. removing\n");
      *c = 0;
    }
    //debug("passfile password: %s\n", (char*)(buf));
    DBG_LIB(dbg_input,"LIB: Before strcpy (password, buf)\n");
    if((dbg_input & 0x10) && (dbg_input & 0x02) && password != NULL)
      fprintf(stderr, "ARGS: strcpy-password: %s \n", password);
    strcpy(password, buf);
    DBG_LIB(dbg_input,"LIB: After strcpy (password,buf)\n");
    if((dbg_input & 0x20) && (dbg_input & 0x02) && password != NULL)
      fprintf(stderr, "RET: strcpy-password: %s \n", password);
  }

  //generate hashed key
  unsigned char* pkey = (unsigned char*)password;
  DBG_LIB(dbg_input,"LIB: Before SHA256 (password)\n");
  if((dbg_input & 0x10) && (dbg_input & 0x02))
    fprintf(stderr, "ARGS: SHA256- password: %s \n", password);
	key = SHA256(pkey, strlen(password), 0);
  DBG_LIB(dbg_input,"LIB: After SHA256 (password)\n");
  if((dbg_input & 0x20) && (dbg_input & 0x02) && password != NULL)
    fprintf(stderr, "RET: SHA256- pkey:%04x \n", key[0]);

  unsigned char *iv = (unsigned char *)"0";
  unsigned char ciphertext[4096];

  DBG_LIB(dbg_input,"LIB: Before free \n");
  free(buf);
  DBG_LIB(dbg_input,"LIB: After free \n");
  DBG_LIB(dbg_input,"LIB: Before malloc \n");
  buf = malloc(bufsize);
  DBG_LIB(dbg_input,"LIB: After malloc \n");
  if (buf == NULL) {
    perror("malloc");
    retval = 22; // indicates to caller that malloc failed
    goto out;
  }
  /* ################## outfile to preserve-out read/write loop ########################### */
  if(outfile_stdin == 0)
    fd2 = STDOUT_FILENO;

  DBG_ENTEXIT(dbg_input, "ENTEXIT: Before wrapper function readWriteFile\n");
  //DBG_ARGS(dbg_input, "Args for readWriteFile- len:%d dbg_input:%x\n", len, dbg_input);
  if(readWriteFile(fd2, preservebuf, len, fd3, dbg_input, NULL, NULL, NULL, crypt) !=0 )
  {
    retval = 25;
    goto out;
  }
  DBG_ENTEXIT(dbg_input, "ENTEXIT: After wrapper function readWriteFile\n");

    
  DBG_SYSCALL(dbg_input, "SYSCALL: Before lseek() (reset outfile fd)\n");
  lseek(fd2, 0, SEEK_SET);
  DBG_SYSCALL(dbg_input, "SYSCALL: After lseek()\n");

  /* ################## infile/outfile read/write loop ########################### */
  if(infile_stdin == 0)
    fd1 = STDIN_FILENO;
   if(outfile_stdin == 0)
    fd2 = STDIN_FILENO;

  DBG_ENTEXIT(dbg_input, "ENTEXIT: Before wrapper function readWriteFile\n"); 

   /*  DBG_ARGS(dbg_input, "Args for readWriteFile- len:%d dbg_input:%x key:%02x
    ciphertext:%s crypt:%d \n", len, dbg_input, key, ); */
  if(readWriteFile(fd1, buf, len, fd2, dbg_input, key, iv, ciphertext, crypt) !=0 )
  {
    retval = 24;
    goto out;
  }
  DBG_ENTEXIT(dbg_input, "ENTEXIT: After wrapper function readWriteFile\n");  

  //successful exit
  retval = 0;

out:
  if(retval == 0)
    debug("Successful.\n");
  else
    debug("retval: %d\n", retval);
  if(retval == 0)
    remove("preserved-outfile");
  else
  {
    if(outfile)
    {
      remove(outfile);
      DBG_SYSCALL(dbg_input, "SYSCALL: Before rename() (temp file -> [preserved]outfile)\n");
      rename("preserved-outfile", outfile);
      DBG_SYSCALL(dbg_input, "SYSCALL: After rename() (temp file -> [preserved]outfile)\n");
    }
  }

  DBG_SYSCALL(dbg_input, "SYSCALL: Before close() fd1\n");
  if (fd1 >= 0)
    close(fd1);
  DBG_SYSCALL(dbg_input, "SYSCALL: After close() fd1\n");
  DBG_SYSCALL(dbg_input, "SYSCALL: Before close() fd2\n");
  if (fd2 >= 0)
    close(fd2);
  DBG_SYSCALL(dbg_input, "SYSCALL: After close() fd2\n");
  DBG_SYSCALL(dbg_input, "SYSCALL: Before close() fd3\n");
  if (fd3 >= 0)
    close(fd3);
  DBG_SYSCALL(dbg_input, "SYSCALL: After close() fd3\n");
  DBG_SYSCALL(dbg_input, "SYSCALL: Before close() fd4\n");
  if (fd4 >= 0)
    close(fd4);
  DBG_SYSCALL(dbg_input, "SYSCALL: After close() fd4\n");

  if (buf != NULL)
    free(buf);
  if(preservebuf != NULL)
    free(preservebuf);

  DBG_ENTEXIT(dbg_input, "ENTEXIT: Exiting main\n");  
  exit(retval);

  
}