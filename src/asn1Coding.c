/*
 *      Copyright (C) 2006, 2007 Free Software Foundation
 *      Copyright (C) 2002 Fabio Fiorina
 *
 * This file is part of LIBTASN1.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


/*****************************************************/
/* File: asn1Coding.c                                */
/* Description: program to generate a DER coding     */
/*              of an ASN1 definition.               */   
/*****************************************************/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <libtasn1.h>

#include <progname.h>
#include <version-etc.h>

static const char help_man[] =
  "Usage: asn1Coding [OPTION] DEFINITIONS ASSIGNMENTS\n"
  "asn1Coding generates a DER encoding of ASN.1 DEFINITIONS file\n"
  "and ASSIGNMENTS file with value assignments.\n"
  "\n"
  "Mandatory arguments to long options are mandatory for short options too.\n"
  "  -c, --check           checks the syntax only\n"
  "  -o, --output FILE     output file\n"
  "  -h, --help            display this help and exit\n"
  "  -v, --version         output version information and exit.\n"
  "\n"
  "Report bugs to <" PACKAGE_BUGREPORT ">.";

#define ASSIGNMENT_SUCCESS 1
#define ASSIGNMENT_ERROR   2
#define ASSIGNMENT_EOF     3

int readAssignment(FILE *file,char *varName, char *value){

  int ret;

  ret=fscanf(file,"%s",varName);
  if(ret==EOF) return ASSIGNMENT_EOF;
  if(!strcmp(varName,"''")) varName[0]=0;

  ret=fscanf(file,"%s",value);
  if(ret==EOF) return ASSIGNMENT_ERROR;

  return ASSIGNMENT_SUCCESS;
}



void createFileName(char *inputFileName, char **outputFileName)
{
 char *char_p,*slash_p,*dot_p;

 /* searching the last '/' and '.' in inputFileAssignmentName */
 char_p=inputFileName;
 slash_p=inputFileName;
 while((char_p=strchr(char_p,'/'))){
   char_p++;
     slash_p=char_p;
 }
 
 char_p=slash_p;
 dot_p=inputFileName+strlen(inputFileName);
 
 while((char_p=strchr(char_p,'.'))){
   dot_p=char_p;
   char_p++;
 }
 
 /* outputFileName= inputFileName + .out */
 *outputFileName=(char *)malloc(dot_p-inputFileName+1+
			       strlen(".out"));
 memcpy(*outputFileName,inputFileName,
          dot_p-inputFileName);
 (*outputFileName)[dot_p-inputFileName]=0;
 strcat(*outputFileName,".out");
 return;
}


/********************************************************/
/* Function : main                                      */
/* Description:                                         */
/********************************************************/
int
main(int argc,char *argv[])
{
  static struct option long_options[] =
  {
    {"help",    no_argument,       0, 'h'},
    {"version", no_argument,       0, 'v'},
    {"check",   no_argument,       0, 'c'},
    {"output",  required_argument, 0, 'o'},
    {0, 0, 0, 0}
  };
  int option_index=0;
 int option_result;
 char *outputFileName=NULL;
 char *inputFileAsnName=NULL;
 char *inputFileAssignmentName=NULL;
 int checkSyntaxOnly=0;
 ASN1_TYPE definitions=ASN1_TYPE_EMPTY;
 ASN1_TYPE structure=ASN1_TYPE_EMPTY;
 char errorDescription[MAX_ERROR_DESCRIPTION_SIZE];
 int asn1_result=ASN1_SUCCESS;
 FILE *outputFile;
 FILE *inputFile;
 char varName[1024];
 char value[1024];
 unsigned char *der = NULL;
 int  der_len;
 int  k;

 set_program_name (argv[0]);

 opterr=0; /* disable error messages from getopt */

 while(1){

   option_result=getopt_long(argc,argv,"hvco:",long_options,&option_index);

   if(option_result == -1) break;

   switch(option_result){
   case 'h':  /* HELP */
     printf("%s\n",help_man);

     if(outputFileName) free(outputFileName);
     exit(0);
     break;
   case 'v':  /* VERSION */
     version_etc (stdout, program_name, PACKAGE, VERSION,
		  "Fabio Fiorina", NULL);
     if(outputFileName) free(outputFileName);
     exit(0);
     break;
   case 'c':  /* CHECK SYNTAX */
     checkSyntaxOnly = 1;
     break;
   case 'o':  /* OUTPUT */
     outputFileName=(char *)malloc(strlen(optarg)+1);
     strcpy(outputFileName,optarg);
     break;
   case '?':  /* UNKNOW OPTION */
     fprintf(stderr,"asn1Coding: option '%s' not recognized or without argument.\n\n",argv[optind-1]);
     printf("%s\n",help_man);

     if(outputFileName) free(outputFileName);
     exit(1);
     break;
   default:
     fprintf(stderr,"asn1Coding: ?? getopt returned character code Ox%x ??\n",option_result);
   }
 }

 if(optind == argc){
   fprintf(stderr,"asn1Coding: input file with ASN1 definitions missing.\n");
   fprintf(stderr,"            input file with assignments missing.\n\n");
   printf("%s\n",help_man);

   if(outputFileName) free(outputFileName);
   exit(1);
 }

 if(optind == argc-1){
   fprintf(stderr,"asn1Coding: input file with assignments missing.\n\n");
   printf("%s\n",help_man);

   if(outputFileName) free(outputFileName);
   exit(1);
 }

 inputFileAsnName=(char *)malloc(strlen(argv[optind])+1);
 strcpy(inputFileAsnName,argv[optind]);

 inputFileAssignmentName=(char *)malloc(strlen(argv[optind+1])+1);
 strcpy(inputFileAssignmentName,argv[optind+1]);

 asn1_result=asn1_parser2tree(inputFileAsnName,&definitions,errorDescription);

 switch(asn1_result){
 case ASN1_SUCCESS:
   printf("Parse: done.\n");
   break;
 case ASN1_FILE_NOT_FOUND:
   printf("asn1Coding: FILE %s NOT FOUND\n",inputFileAsnName);
   break;
 case ASN1_SYNTAX_ERROR: 
 case ASN1_IDENTIFIER_NOT_FOUND: 
 case ASN1_NAME_TOO_LONG:
   printf("asn1Coding: %s\n",errorDescription);
   break;
 default:
   printf("libtasn1 ERROR: %s\n",libtasn1_strerror(asn1_result));
 }

 if(asn1_result != ASN1_SUCCESS){
   free(inputFileAsnName);
   free(inputFileAssignmentName);
   exit(1);
 }


 inputFile=fopen(inputFileAssignmentName,"r");
 
 if(inputFile==NULL){
   printf("asn1Coding: file '%s' not found\n",inputFileAssignmentName);
   free(inputFileAsnName);
   free(inputFileAssignmentName);
   exit(1);
 }


 printf("\n");

 while(readAssignment(inputFile,varName,value) == ASSIGNMENT_SUCCESS){
   printf("var=%s, value=%s\n",varName,value);
   if(structure==ASN1_TYPE_EMPTY){
     asn1_result=asn1_create_element(definitions,value,&structure);
   }
   else
     asn1_result=asn1_write_value(structure,varName,value,0); 

   if(asn1_result != ASN1_SUCCESS){
     printf("libtasn1 ERROR: %s\n",libtasn1_strerror(asn1_result));
     
     asn1_delete_structure(&definitions);
     asn1_delete_structure(&structure);
     
     free(inputFileAsnName);
     free(inputFileAssignmentName);
     
     fclose(inputFile);
     exit(1);
   }
 }
 fclose(inputFile);

 printf("\n");
 asn1_print_structure(stdout,structure,"",ASN1_PRINT_NAME_TYPE_VALUE);

 der_len=0;
 asn1_result=asn1_der_coding(structure,"",der,&der_len,
                             errorDescription);
 if (asn1_result==ASN1_MEM_ERROR)
   {
     der = malloc (der_len);
     asn1_result=asn1_der_coding(structure,"",der,&der_len,
				 errorDescription);
   }
 printf("\nCoding: %s\n\n",libtasn1_strerror(asn1_result));
 if(asn1_result!=ASN1_SUCCESS){
   printf("asn1Coding: %s\n",errorDescription);

   if (der)
     free (der);

   asn1_delete_structure(&definitions);
   asn1_delete_structure(&structure);

   free(inputFileAsnName);
   free(inputFileAssignmentName);

   exit(1);
 }

 /* Print the 'Certificate1' DER encoding */ 
 printf("-----------------\nNumber of bytes=%i\n",der_len);
 for(k=0;k<der_len;k++) printf("%02x ",der[k]);  
 printf("\n-----------------\n");

 asn1_delete_structure(&definitions);
 asn1_delete_structure(&structure);
 

 if(outputFileName==NULL)
   createFileName(inputFileAssignmentName,&outputFileName);

 printf("\nOutputFile=%s\n",outputFileName);

 outputFile=fopen(outputFileName,"w");
 
 if(outputFile==NULL){
   printf("asn1Coding: output file '%s' not available\n",outputFileName);
   if (der)
     free (der);
   free(inputFileAsnName);
   free(inputFileAssignmentName);
   free(outputFileName);
   exit(1);
 }

 for(k=0;k<der_len;k++) 
   fprintf(outputFile,"%c",der[k]);  
 fclose(outputFile);
 printf("\nWriting: done.\n");

 if (der)
   free (der);

 free(inputFileAsnName);
 free(inputFileAssignmentName);
 free(outputFileName);

 exit(0);
}
