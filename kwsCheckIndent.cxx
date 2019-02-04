/*=========================================================================

  Program:   KWStyle - Kitware Style Checker
  Module:    kwsCheckIndent.cxx

  Copyright (c) Kitware, Inc.  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "kwsParser.h"

#include <string.h>
#include <sstream>
namespace kws {

#define ALIGN_LEFT -99999

/** Extract the current line from pos to LF */
std::string Parser::ExtractLine(size_t pos)
{
  size_t p = m_Buffer.find("\n",pos);
  if(p>pos)
    {
    return m_Buffer.substr(pos,p-pos-1);
    }
  return "";
}

/** Return the current ident */
int Parser::GetCurrentIdent(std::string line,char type)
{
  int indent = 0;
  std::string::const_iterator it = line.begin();
  while(it != line.end() && (*it)== type)
    {
    indent++;
    it++;
    }
  return indent;
}

bool check_line(std::string line, int *expected, int *actual,int *current, char indent, int size)
{	
	bool result = true; //passed
	int len = line.length()-1;
	*current = *expected; // we store the expected in current
	*actual=0; 
	std::cout << "\n" << *actual << "-" << len << "\n";
	while(*actual<len)
	{
		if (line.at((*actual)+1)!=indent) break;
		(*actual)++; 
	}
	if (len <= 0) return result; // empty line, nothing to be done

	int pos=*actual+1;
	result = *actual == *expected;
	while(pos<len)
	{ 
		if (line.at(pos)=='{') *expected+=size;
		if (line.at(pos)=='}') *expected-=size;
		pos++;
	}
	return result;
}

std::string get_line ( std::string & buffer, unsigned long *pos)
{
	unsigned oldpos=*pos;
	*pos=buffer.find('\n',*pos+1)+1;
	std::string line = buffer.substr(oldpos,*pos - 2 - oldpos);
	return line;
}

int get_indent(std::string line, char indent)
{
	int pos =0;
	while (pos<line.length())
	{
		if( line.at(pos) != indent )
			return pos;
		pos++;
	}
}

std::string clean_line(std::string line)
{
	int pos = line.length()-1;
	while (pos >=0 && (line.at(pos) == '\r' || line.at(pos) == ' '))	
	{
		pos--;
	}
	std::string rt= line.substr(0,pos+1);
	pos =0;
	std::string result="";
	
	bool active=true;
	while (pos<rt.length())
	{
		if( rt.at(pos) == '\"' ) active = !active;
		if (active && rt.at(pos) != '\"') result +=  rt.at(pos);
		pos++;
	}
	pos=0;
	rt=result;
	result="";
	active=true;
	while (pos<rt.length())
	{
		if( rt.at(pos) == '\'') active = !active;
		if (active && rt.at(pos) != '\'') result +=  rt.at(pos);
		pos++;
	}
	return result;
}

int indent_change (std::string line)
{
	int pos =0;
	int result=0;
	while (pos<line.length())
	{
		if( line.at(pos) == '{' ) result++;
		if( line.at(pos) == '}' ) result--;
		pos++;
	}	
	return result;
}

bool is_directive(std::string line)
{
    size_t pos=0;
    while (pos<line.length())
    {
        if (line.at(pos) == '#') return true;
        if (line.at(pos) != '\t' and line.at(pos) != ' ') return false;
        pos++;
    }
    return false;
}

bool if_exception ( std::string line)
{
	return (line.find ("if") !=std::string::npos && line.find (';') ==std::string::npos);		
}
bool case_exception_begin ( std::string line)
{
	return (line.find ("case") !=std::string::npos && line.find (':') !=std::string::npos);		
}
bool case_exception_end ( std::string line)
{
	return (line.find ("break") !=std::string::npos && line.find (';') !=std::string::npos);		
}

/** Check the indent size */
bool Parser::CheckIndent(IndentType itype,
                         unsigned long size,
                         DirectivePosition directivePosition,
                         bool doNotCheckHeader,
                         bool allowBlockLine,
                         unsigned int maxLength,
                         bool allowCommaIndent)
{
  char indent_char = ' ';
  if(itype == TAB) indent_char='\t';
  
  bool hasError = false;
  int expected=0, current=0;
  std::stringstream ss (m_BufferNoComment);
  std::string l;
  int line_number = 0;
  int last_line_if_exception=0;
  int case_exception=0;
  while(std::getline(ss,l,'\n'))
  {
	line_number++;
	std::string line = clean_line(l) ;
    //std::cout<< is_directive(line) << " - " << line <<"\n";
	int actual = get_indent(line,indent_char);
    if (is_directive(line))
    {
        int dexpected=0;
        switch (directivePosition)
        {
            case INDENTED:
                dexpected=expected;
                break;
            case FREE:
                dexpected=actual;
        }
        if (dexpected!=actual)
        {
          hasError=true;
          Error error;
          error.line = line_number;
          error.line2 = error.line;
          error.number = INDENT;
          error.description = "Directive indent is wrong ";
          char* localval = new char[10];
          sprintf(localval,"%d",actual);
          error.description += localval;
          error.description += " (should be ";
          delete [] localval;
          localval = new char[10];
          sprintf(localval,"%d",dexpected);
          error.description += localval;
          error.description += ")";
          delete [] localval;
          m_ErrorList.push_back(error);
        }
    }
    else
    {
		//std::cout << actual << '\t' << expected << "\t" << last_line_if_exception << "\t" << indent_change(line) << "\t" << abs(actual-expected) << ":" << line <<"\n";
        if ( (actual!=expected	&& 
		       actual!=expected+indent_change(line) * size   &&
		       abs(actual-expected)  > last_line_if_exception * size) )
        {
          hasError=true;
          Error error;
          error.line = line_number;
          error.line2 = error.line;
          error.number = INDENT;
          error.description = "Indent is wrong ";
          char* localval = new char[10];
          sprintf(localval,"%d",actual);
          error.description += localval;
          error.description += " (should be ";
          delete [] localval;
          localval = new char[10];
          sprintf(localval,"%d",expected);
          error.description += localval;
          error.description += ")";
          delete [] localval;
          m_ErrorList.push_back(error);
        }
		if (last_line_if_exception) last_line_if_exception--;
		last_line_if_exception += if_exception(line);
        expected+=indent_change(line) * size;
    }
  }
 return !hasError;
}


/** Check if the current position is a valid switch statement */
bool Parser::CheckValidSwitchStatement(unsigned int posSwitch)
{
    return true;
}

/** Init the indentation */
bool Parser::InitIndentation()
{
  return true;
}

void Parser::AddIndent(const char* name,int current,int after)
{
}

bool Parser::IsInElseForbiddenSection(size_t pos)
{
  return false;
}


} // end namespace kws