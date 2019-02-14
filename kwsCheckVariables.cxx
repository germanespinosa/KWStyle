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
	return !findChar(".&|!{}();,\n\r #=+-*/%\"\\[]\'<>^",c);
}

bool isBreak(char c)
{
	return findChar("{}();,=\"",c);
}

bool isWord (std::string  buffer)
{
	if (buffer.length()==0) return false;
	if (buffer.at(0)>='0' && buffer.at(0)<='9') return false;
	if (buffer.find("class") != std::string::npos) return false;
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
		if (isVariable && variable.find(':')!=std::string::npos) isVariable=false;
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
}