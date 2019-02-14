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

bool same_line_else_if (std::string line, int indent)
{
	return indent_change(line)==0 && line.at(indent)=='}';
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
	return ((line.find ("if") !=std::string::npos && line.find (';') ==std::string::npos)||(line.find("else")!=std::string::npos && indent_change(line)==0));
}
bool switch_exception_begin ( std::string line)
{
	return (line.find ("switch") !=std::string::npos);		
}
bool case_exception ( std::string line)
{
	return (line.find ("case") !=std::string::npos && line.find (':') !=std::string::npos);		
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
  int last_line_if_exception = 0;
  bool last_line_switch_exception = false;
  std::vector<int> switches_indent;
  while(std::getline(ss,l,'\n'))
  {
	line_number++;
	std::string line = clean_line(l) ;
	if (line.empty()) continue;
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
		if (switches_indent.size()) 
		{
			if (switches_indent.back() == actual && indent_change(line) ==-1) 
			{
				switches_indent.pop_back();
				expected-=size;
			}
		}
        if ( (actual!=expected	&& 
		       actual!=expected+indent_change(line) * size   &&
		       abs(actual-expected)  > last_line_if_exception * size) && 
			  ! (same_line_else_if(line,actual) && actual ==expected - size) && 
			  ! (case_exception(line) && actual + size == expected ))
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
		if (switch_exception_begin(line)) 
		{
			switches_indent.push_back(expected);
			expected+=size;
		}
        expected+=indent_change(line) * size;
		last_line_switch_exception=switch_exception_begin(line);
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