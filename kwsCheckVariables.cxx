/*=========================================================================

  Program:   KWStyle - Kitware Style Checker
  Module:    kwsCheckVariables.cxx

  Copyright (c) Kitware, Inc.  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "kwsParser.h"
#include <iostream>

namespace kws {

/** Check if the variables implementation of the class are correct */
typedef std::pair<std::string,long int> VarDetections;

std::vector<VarDetections> FindVariables(std::string & buffer);


bool Parser::CheckVariables(const char* regEx)
{
    
  m_TestsDone[VARS] = true;
  m_TestsDescription[VARS] = "ivars implementation should match regular expression: ";
  m_TestsDescription[VARS] += regEx;

  // First we need to find the parameters
  bool hasError = false;

  kwssys::RegularExpression regex(regEx);

  std::vector<VarDetections> ivars = FindVariables(m_BufferNoComment);

  std::vector<VarDetections>::const_iterator it = ivars.begin();
  while(it != ivars.end())
  {
    std::string var = (*it).first;
    long int pos =(*it).second;
	//std::cout << "v:" << var << "- pos:" << pos << " - line: " << this->GetLineNumber(pos,true) << "\n";	
	it++;
	
	
      if(!regex.find(var))
        {
        Error error;
        error.line = this->GetLineNumber(pos,true);
        error.line2 = error.line;
        error.number = VARS;
        error.description = "variable (" + var + ") doesn't match regular expression";
        m_ErrorList.push_back(error);
        hasError = true;
        }
	
  }
  
  
  
  return hasError;

  // Do the checking
  while(it != ivars.end())
    {
    std::string v = (*it).first;
    long int p =(*it).second;
    size_t posVar = m_BufferNoComment.find(v);
    while(posVar != std::string::npos)
      {
      // Extract the complete insert of the variable
      if(!this->IsBetweenQuote(posVar)
        &&(
        m_BufferNoComment[posVar-1]=='.'
        || m_BufferNoComment[posVar-1]=='>'
        || m_BufferNoComment[posVar-1]=='\n'
        || m_BufferNoComment[posVar-1]==' '
        || m_BufferNoComment[posVar-1]=='('
        || m_BufferNoComment[posVar-1]=='['
        )
        &&(
        m_BufferNoComment[posVar+v.size()]=='.'
        || m_BufferNoComment[posVar+v.size()]=='>'
        || m_BufferNoComment[posVar+v.size()]=='\n'
        || m_BufferNoComment[posVar+v.size()]==' '
        || m_BufferNoComment[posVar+v.size()]=='('
        || m_BufferNoComment[posVar+v.size()]=='['
        || m_BufferNoComment[posVar+v.size()]==')'
        || m_BufferNoComment[posVar+v.size()]==']'
        )
        )
        {

      size_t i = posVar-1;
      while(i>0)
        {
        if(m_BufferNoComment[i]==' '
         || m_BufferNoComment[i]=='('
         || m_BufferNoComment[i]=='['
         || m_BufferNoComment[i]=='\n'
         || m_BufferNoComment[i]=='!'
         || m_BufferNoComment[i]=='{'
         || m_BufferNoComment[i]==';'
         || m_BufferNoComment[i]=='<'
         || m_BufferNoComment[i]=='*'
         )
          {
          break;
          }
        i--;
        }

      std::string var = m_BufferNoComment.substr(i+1,posVar-i+v.size()-1);
      
      bool showError = true;

      // Check if this a macro
      if(this->GetLineNumber(posVar,true) == p+1)
        {
        showError = false;
        }
      else
        {
        std::string line = this->GetLine(this->GetLineNumber(posVar,true)-1);
        if(line.find("Macro") != std::string::npos)
          {
          showError = false;
          }
        }

      // Check the regex
      if(showError && !regex.find(var))
        {
        Error error;
        error.line = this->GetLineNumber(posVar,true);
        error.line2 = error.line;
        error.number = VARS;
        error.description = "variable (" + var + ") doesn't match regular expression";
        m_ErrorList.push_back(error);
        hasError = true;
        }
        }
      posVar = m_BufferNoComment.find(v,posVar+1);
      }
    it++;
    }
  return !hasError;
}

bool findChar (const char* haystack ,char needle)
{
	while (*haystack && *haystack !=needle) 
	{
		haystack++;
	}
	return *haystack;
}

bool isLetter(char c)
{
	return !findChar("&|!:{}();,\n\r #=+-*/%\"\\[]\'<>^",c);
}

bool isBreak(char c)
{
	return findChar("{}();,=\"",c);
}

bool isWord (std::string  buffer)
{
	if (buffer.length()==0) return false;
	if (buffer.at(0)>='0' && buffer.at(0)<='9') return false;
	if (buffer.find("struct") != std::string::npos) return false;
	if (buffer.find("enum") != std::string::npos) return false;
	if (buffer.find("const") != std::string::npos) return false;
	if (buffer.find("include") != std::string::npos) return false;
	if (buffer.find("define") != std::string::npos) return false;
	if (buffer.find("NULL") != std::string::npos) return false;
	if (buffer.find("true") != std::string::npos) return false;
	if (buffer.find("false") != std::string::npos) return false;
	if (buffer.find("return") != std::string::npos) return false;
	return true;
}

#define  FWD if (end<buffer.length()-1) end++; else return result

std::vector<VarDetections>  FindVariables(std::string & buffer)
{
	size_t start=0, end=0;
	bool lastIsWord=false;
	bool isVariable=false;
    std::vector<VarDetections> result;
	std::string variable;
	while (start < buffer.length())
	{
		isVariable=false;
		while (isLetter(buffer.at(end))) 
		{
			FWD;
		}
		variable = buffer.substr(start,end-start);
		if (isWord(variable))
		{
			if (!lastIsWord)
			{
				lastIsWord=true;
			}else
			{
				isVariable=true;
				
				lastIsWord =  false;
			}
		}
		else lastIsWord =  false;

		while (!isLetter(buffer.at(end))) 
		{
			if (buffer.at(end)=='(')  isVariable=false;
			if (isBreak(buffer.at(end)))  lastIsWord=false;

			if (buffer.at(end)=='\"') 
			{
				FWD;
				while (buffer.at(end)!='"') FWD;
			}
			if (buffer.at(end)=='\'') 
			{
				FWD;
				while (buffer.at(end)!='\'')  FWD;
			}
			if (buffer.at(end)=='<') 
			{
				FWD;
				while (buffer.at(end)!='>') FWD;
			}
			FWD;
		}
		if (isVariable)  
		{
			VarDetections var;
			var.first = variable;
			var.second =  start; 
			result.push_back(var);
		}
		start=end;
	}
	return result;
}

/** Find the first ivar in the source code */
std::string Parser::FindVariable(std::string & buffer, size_t start, size_t end,size_t & pos)
{
  size_t lastLineStart = 0;
  size_t posSemicolon = buffer.find(";",start);
  while(posSemicolon != std::string::npos && posSemicolon<end)
    {
    // We try to find the word before that
    //std::cout<<"line: "<<buffer.substr(lastLineStart,posSemicolon+lastLineStart)<<" : fin\n";
    size_t i = posSemicolon-1;
    bool inWord = true;
    bool first = false;
    std::string ivar = "";
    while(i!=std::string::npos && inWord)
      {
      if(buffer[i] != ' ')
        {
        if((buffer[i] == '}')
          || (buffer[i] == ')')
          || (buffer[i] == ']')
          || (buffer[i] == '\n')
          )
          {
          inWord = false;
          }
        else
          {
          std::string store = ivar;
          ivar = buffer[i];
          ivar += store;
          inWord = true;
          first = true;
          }
        }
      else // we have a space
        {
        if(first)
          {
          inWord = false;
          }
        }
      i--;
      }
    pos = posSemicolon;
    // We extract the complete definition.
    // This means that we look for a '{' or '}' or '{' or ':'
    // but not '::'
    while(i != std::string::npos)
      {
      if(buffer[i] == ';')
        {
        break;
        }
      else if(buffer[i] == ':')
        {
        if((buffer[i-1] != ':') && (buffer[i+1] != ':'))
          {
          break;
          }
        }
      i--;
      }

    std::string subphrase = "";

    if(i != std::string::npos)
      {
      subphrase = buffer.substr(i+1,posSemicolon-i-1);
      }

    if( (subphrase.find("=") == std::string::npos)
      && (subphrase.find("(") == std::string::npos)
      && (subphrase.find("typedef") == std::string::npos)
      && (subphrase.find("}") == std::string::npos)
      && (subphrase.find("friend") == std::string::npos)
      && (subphrase.find("class") == std::string::npos)
      && (subphrase.find("return") == std::string::npos)
      && (subphrase.find("\"") == std::string::npos)
      && (subphrase.find("<<") == std::string::npos)
      )
      {
      // Check that we are not inside a function(){}
      if(!this->IsInFunction(posSemicolon,buffer.c_str())
        && !this->IsInStruct(posSemicolon,buffer.c_str())
        )
        {
        return ivar;
        }
      }
	std::cout<<"line: "<<buffer.substr(lastLineStart+1,posSemicolon-lastLineStart)<<" : fin\n";
	lastLineStart=posSemicolon;
    posSemicolon = buffer.find(";",posSemicolon+1);
    }

  pos = std::string::npos;
  return "";
}

} // end namespace kws
