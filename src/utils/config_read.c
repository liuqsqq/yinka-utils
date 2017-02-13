/*
 *   Copyright (C) 2017  Steven Lee <geekerlw@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <assert.h>  
#include <ctype.h>  
#include <errno.h> 

#include "config_read.h"  

char *left_space_remove(char * stroutput, const char *strinput)  
{  
    assert(strinput != NULL);  
    assert(stroutput != NULL);  
    assert(stroutput != strinput);  
    for(NULL; *strinput != '\0' && isspace(*strinput); ++strinput) {  
        ;  
    }  
    return strcpy(stroutput, strinput);  
}  
 
char *right_space_remove(char *stroutput, const char *strinput)  
{  
    char *p = NULL;  
    assert(strinput != NULL);  
    assert(stroutput != NULL);  
    assert(stroutput != strinput);  
    strcpy(stroutput, strinput);  
    for(p = stroutput + strlen(stroutput) - 1; p >= stroutput && isspace(*p); --p) {  
        ;  
    }  
    *(++p) = '\0';  
    return stroutput;  
}  

char * all_sides_space_remove(char * stroutput, const char * strinput)  
{  
    char *p = NULL;  
    assert(strinput != NULL);  
    assert(stroutput != NULL);  
    left_space_remove(stroutput, strinput);  
    for(p = stroutput + strlen(stroutput) - 1;p >= stroutput && isspace(*p); --p) {  
        ;  
    }  
    *(++p) = '\0';  
    return stroutput;  
}  
  
int conf_read(char *filepath, char *AppName, char *KeyName, char *KeyVal )  
{  
    char appname[32],keyname[32];  
    char *buf,*c;  
    char buf_i[KEYVALLEN], buf_o[KEYVALLEN];  
    FILE *fp;  
    int found=0; /* 1 AppName 2 KeyName */

    if( (fp=fopen( filepath,"r" )) == NULL) {  
        printf( "openfile [%s] error [%s]\n", filepath, strerror(errno));  
        return(-1);  
    }
	
    fseek(fp, 0, SEEK_SET );  
    memset(appname, 0, sizeof(appname));  
    sprintf(appname,"[%s]", AppName);  
  
    while( !feof(fp) && fgets(buf_i, KEYVALLEN, fp) != NULL) {  
        left_space_remove(buf_o, buf_i);  
        if(strlen(buf_o) <= 0)  
            continue;  
        buf = NULL;  
        buf = buf_o;  
  
        if(found == 0) {  
            if(buf[0] != '[') {  
                continue;  
            } 
			else if ( strncmp(buf,appname,strlen(appname))==0 ) {  
                found = 1;  
                continue;  
            }  
  
        } 
		else if(found == 1) {  
            if(buf[0] == '#') {  
                continue;  
            } 
			else if (buf[0] == '[') {  
                break;  
            } 
			else {  
                if((c = (char*)strchr(buf, '=')) == NULL)  
                    continue;  
        		memset(keyname, 0, sizeof(keyname));  
  
            	sscanf( buf, "%[^=|^ |^\t]", keyname);  
            	if(strcmp(keyname, KeyName) == 0) {  
            		sscanf( ++c, "%[^\n]", KeyVal);  
                	char *KeyVal_o = (char *)malloc(strlen(KeyVal) + 1);  
               		if(KeyVal_o != NULL){  
                		memset(KeyVal_o, 0, sizeof(KeyVal_o));  
                 	   all_sides_space_remove(KeyVal_o, KeyVal);  
                  	  if(KeyVal_o && strlen(KeyVal_o) > 0)  
                    		strcpy(KeyVal, KeyVal_o);  

					free(KeyVal_o);  
                	KeyVal_o = NULL;  
              	  	}  
				
            	found = 2;  
            	break;  
         		} 
				else {  
        		continue;
				}  
            }  
        }  
    }
	
    fclose( fp );  
    if(found == 2)  
        return(0);  
    else  
        return(-1);  
}    

