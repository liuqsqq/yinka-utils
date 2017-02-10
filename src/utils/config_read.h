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

#ifndef CONFIG_READ_H
#define CONFIG_READ_H


#define KEYVALLEN 256  
  
/*   É¾³ý×ó±ßµÄ¿Õ¸ñ   */  
char * l_trim(char * szOutput, const char *szInput);  
  
/*   É¾³ýÓÒ±ßµÄ¿Õ¸ñ   */  
char *r_trim(char *szOutput, const char *szInput);  
  
/*   É¾³ýÁ½±ßµÄ¿Õ¸ñ   */  
char * a_trim(char * szOutput, const char * szInput);  
  
  
int conf_read(char *filepath, char *AppName, char *KeyName, char *KeyVal );

#endif
