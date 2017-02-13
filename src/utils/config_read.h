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
  
/*   remove spaces on the left   */  
char * left_space_remove(char * stroutput, const char *strinput);  
  
/*   remove spaces on the right   */  
char *right_space_remove(char *stroutput, const char *strinput);  
  
/*   remove spaces on both sides   */  
char * both_sides_space_remove(char * stroutput, const char * strinput);  
  
/*   config read function   */  
int conf_read(char *filepath, char *AppName, char *KeyName, char *KeyVal );

#endif
