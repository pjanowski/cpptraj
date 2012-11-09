#include <locale>  // isdigit, 
#include <stdexcept> // ArgList[]
#include <cstring> //strtok, strchr
#include "ArgList.h"
#include "CpptrajStdio.h"
#include "StringRoutines.h"

ArgList::ArgList(const char* input) {
  if (input != 0) SetList( std::string(input), " ");
}

ArgList::ArgList(std::string const& input) {
  SetList( input, " " );
}

ArgList::ArgList(std::string const& input, const char* sep) {
  SetList( input, sep );
}

// COPY CONSTRUCTOR
ArgList::ArgList(const ArgList &rhs) :
  arglist_(rhs.arglist_),
  argline_(rhs.argline_),
  marked_(rhs.marked_)
{}

// ArgList::operator=()
ArgList &ArgList::operator=(const ArgList &rhs) {
  if (&rhs==this) return *this;
  // Allocate and copy. Assignment ops should automatically deallocate.
  arglist_ = rhs.arglist_;
  argline_ = rhs.argline_;
  marked_ = rhs.marked_;
  return *this;
}

// ArgList::operator[]
std::string const& ArgList::operator[](int idx) {
  if (idx < 0 || idx >= (int)arglist_.size())
    throw std::out_of_range("ArgList[]");
  return arglist_[idx];
}

// ArgList::SetList()
/** Separate input by the characters in separator and store as separate args.
  * This overwrites any existing args and completely resets the list.
  * \param inputString NULL terminated string to be converted to arguments
  * \param separator string containing characters used to separate arguments
  * \return 0 if arglist successfully set up, 1 if not.
  */
int ArgList::SetList(std::string const& inputString, const char *separator) {
  if (inputString.empty() || separator==0) return 1;
  // Copy inputString to temp since it is destroyed by tokenize.
  size_t inputStringSize = inputString.size();
  if (inputStringSize < 1) return 1;
  char* tempString = new char[ inputStringSize+1 ];
  inputString.copy( tempString, inputStringSize, 0 );
  tempString[ inputStringSize ] = '\0'; // copy() does not append NULL
  // Remove newline char from tempString if present
  if ( tempString[ inputStringSize - 1 ] == '\n' )
    tempString[ inputStringSize - 1 ] = '\0';
  // Free existing arglist
  arglist_.clear();
  marked_.clear();
  // Store inputString
  argline_.assign(inputString);
  // Begin tokenization
  char quotechar;
  char *pch = strtok(tempString, separator);
  if (pch!=NULL) {
    while (pch!=NULL) {
      //if (debug>1) mprintf("getArgList:  Arg %i, Token [%s], ",nargs,pch);
      // If the argument is not quoted add it to the list
      if      (pch[0]=='"' ) quotechar='"';
      else if (pch[0]=='\'') quotechar='\'';
      else quotechar=' ';
      if (quotechar==' ') {
        arglist_.push_back( std::string(pch) );
      } else {
        // If the argument begins with a quote, place this and all subsequent
        // arguments ending with another quote into this argument
        std::string argument(pch);
        unsigned int argsize = argument.size();
        // Check for blank quote token ("")
        if (argsize != 2 || argument[1] != quotechar) {
          // Check if this argument itself ends with a quote
          if (argsize == 1 || argument[argsize-1]!=quotechar) {
            while (pch!=NULL) {
              argument.append(" ");
              pch=strtok(NULL," ");
              // If pch is NULL at this point there was no closing quote.
              if (pch==NULL) {
                mprintf("\tWarning: argument missing closing quote [%c]\n",quotechar);
                break;
              }
              argument.append(pch);
              if (strchr(pch,quotechar)!=NULL) break;
            }
          }
          // Remove quotes from the argument
          for (std::string::iterator character = argument.begin();
                                     character < argument.end();
                                     ++character)
            if (*character == quotechar) character = argument.erase(character);
          arglist_.push_back(argument);
        }
      }
      //if (debug>1) mprintf("Arglist[%i]= [%s]\n",nargs-1,arglist[nargs-1]);
      pch = strtok(NULL,separator);
    } // END while loop
    // Set up marked array
    marked_.resize( arglist_.size(), false );
  }
  // if (debug>0) mprintf("getArgList: Processed %i args\n",nargs);
  delete[] tempString;
  return 0;
}

ArgList ArgList::RemainingArgs() {
  ArgList remain;
  for (unsigned int arg = 0; arg < arglist_.size(); ++arg) {
    if ( !marked_[arg] )
      remain.AddArg( arglist_[arg] );
  }
  return remain;
}

// ArgList::AddArg()
/** \param input NULL terminated string to add to argument list
  */
void ArgList::AddArg(std::string const& input) {
  // Dont store blank tokens
  if (input.empty()) return;
  if (input[0]=='\n') return;
  arglist_.push_back(input);
  argline_.append(input);
  argline_.append(" ");
  marked_.push_back(false);
}

// ArgList::MarkArg()
void ArgList::MarkArg(int arg) {
  if (arg < 0 || arg >= (int) marked_.size()) return;
  marked_[arg]=true;
}

// ArgList::CheckForMoreArgs()
/** Check if all arguments have been processed. If not print a warning along
  * with all unprocessed arguments.
  */
void ArgList::CheckForMoreArgs() {
  std::string notmarked;
  for (unsigned int arg=0; arg < arglist_.size(); arg++) {
    if (!marked_[arg]) 
      notmarked.append(arglist_[arg] + " ");
  }
  if (!notmarked.empty())  
    mprintf("Warning: [%s] Not all arguments handled: [ %s]\n",
            arglist_[0].c_str(), notmarked.c_str());
}

// ArgList::PrintList()
void ArgList::PrintList() {
  for (unsigned int arg = 0; arg < arglist_.size(); arg++) 
    mprintf("  %u: %s\n",arg+1,arglist_[arg].c_str());
}

// ArgList::PrintDebug()
void ArgList::PrintDebug() {
  mprintf("ArgLine: %s\n",argline_.c_str());
  for (unsigned int arg = 0; arg < arglist_.size(); arg++)
    mprintf("\tArg %u: %s (%i)\n",arg+1,arglist_[arg].c_str(),(int)marked_[arg]);
}

// ArgList::RemoveFirstArg()
void ArgList::RemoveFirstArg() {
  if (arglist_.empty()) return;
  arglist_.erase( arglist_.begin() );
  marked_.erase( marked_.begin() );
}

// ArgList::Command()
/* \return pointer to the first argument
 */
const char *ArgList::Command() const {
  if (arglist_.empty()) return NULL;
  return arglist_[0].c_str();
}

// ArgList::CommandIs()
/** \param key Key to check first argument against
  * \return true if first argument matches key
  */
bool ArgList::CommandIs(const char *key) const {
  if (arglist_.empty()) return false;
  if (arglist_[0].compare( key )==0) return true;
  return false;
}

// ArgList::getNextString()
/** \return the next unmarked string.
  */
ArgList::ConstArg ArgList::getNextString() {
  for (unsigned int arg = 0; arg < arglist_.size(); ++arg)
    if (!marked_[arg]) {
      marked_[arg]=true;
      return arglist_[arg].c_str();
    }
  return NULL;
}

// ArgList::GetStringNext()
std::string ArgList::GetStringNext() {
  for (unsigned int arg = 0; arg < arglist_.size(); ++arg)
    if (!marked_[arg]) {
      marked_[arg]=true;
      return arglist_[arg];
    }
  return std::string();
}

// ArgList::getNextMask()
/** Return next unmarked Mask. A mask MUST include one of the following: 
  *   ':' residue
  *   '@' atom
  *   '/' element
  *   '%' type
  *   '*' everything
  * \return the next unmarked atom mask expression
  */
ArgList::ConstArg ArgList::getNextMask() {
  for (unsigned int arg=0; arg < arglist_.size(); ++arg) {
    if (!marked_[arg]) {
      size_t found = arglist_[arg].find_first_of(":@*/%");
      if (found != std::string::npos) {
        marked_[arg]=true;
        return arglist_[arg].c_str();
      }
    }
  }
  return NULL;
}

// ArgList::GetMaskNext()
std::string ArgList::GetMaskNext() {
  for (unsigned int arg = 0; arg < arglist_.size(); ++arg) {
    if (!marked_[arg]) {
      size_t found = arglist_[arg].find_first_of(":@*/%");
      if (found != std::string::npos) {
        marked_[arg] = true;
        return arglist_[arg];
      }
    }
  }
  return std::string();
}

// ArgList::getNextTag()
/** Return the next unmarked tag. A tag is defined as a character string
  * bounded by brackets, e.g. [tag].
  */
std::string ArgList::getNextTag() {
  for (unsigned int arg = 0; arg < arglist_.size(); arg++) {
    if (!marked_[arg]) {
      std::string::reverse_iterator lastchar  = arglist_[arg].rbegin();
      std::string::iterator         firstchar = arglist_[arg].begin();
      if (*firstchar=='[' && *lastchar==']') {
        marked_[arg]==true;
        return arglist_[arg];
      }
    }
  }
  return std::string();
}

// validInteger()
/// Brief check that the passed in string begins with a digit or '-'
inline bool validInteger(std::string const &argument) {
  std::locale loc;
  if (isdigit(argument[0],loc) || argument[0]=='-') return true;
  return false;
}

// validDouble()
/// Brief check that the passed in string begins with a digit, '-', or '.'
inline bool validDouble(std::string const &argument) {
  std::locale loc;
  if (isdigit(argument[0],loc) || argument[0]=='-' || argument[0]=='.' ) return true;
  return false;
}

// ArgList::getNextInteger()
/** \param def Value to return if no integer args found
  * \return Next unmarked integer argument or def
  */
int ArgList::getNextInteger(int def) {
  for (unsigned int arg=0; arg < arglist_.size(); arg++)
    if (!marked_[arg]) {
      // Check that first char is indeed an integer or '-', if not then continue
      if (validInteger(arglist_[arg])) {
        int ival = convertToInteger(arglist_[arg]);
        marked_[arg]=true;
        return ival;
      }
    }
  return def;
}

// ArgList::getNextDouble()
/** \param def Value to return if no double args found
  * \return Next unmarked double argument or def
  */
double ArgList::getNextDouble(double def) {
  for (unsigned int arg=0; arg < arglist_.size(); arg++)
    if (!marked_[arg]) {
      // Check that first char is indeed a digit, '.', or '-', if not then continue
      if (validDouble(arglist_[arg])) {
        double dval = convertToDouble(arglist_[arg]);
        marked_[arg]=true;
        return dval;
      }
    }
  return def;
}

// ArgList::getKeyString()
/** Search the argument list for key, return the argument following key
  * as a string if found, otherwise return 0.
  * \param key string to search for
  */
ArgList::ConstArg ArgList::getKeyString(const char *key) {
  unsigned int nargs = arglist_.size() - 1;
  for (unsigned int arg=0; arg < nargs; ++arg)
    if (!marked_[arg]) {
      if (arglist_[arg].compare(key)==0) { 
        marked_[arg++]=true;
        marked_[arg]=true;
        return arglist_[arg].c_str();
      }
    }
  return NULL;
}

// ArgList::GetStringKey()
std::string ArgList::GetStringKey(const char *key) {
  std::string empty;
  unsigned int nargs = arglist_.size() - 1;
  for (unsigned int arg=0; arg < nargs; arg++)
    if (!marked_[arg]) {
      if (arglist_[arg].compare(key)==0) {
        marked_[arg]=true;
        arg++;
        marked_[arg]=true;
        return arglist_[arg];
      }
    }
  return empty;
}

// ArgList::getKeyInt()
/** Search the argument list for key, return the argument following key
  * as an integer if found, otherwise return def.
  * \param key string to search for
  * \param def Value to return if key not found.
  */
int ArgList::getKeyInt(const char *key, int def) {
  unsigned int nargs = arglist_.size() - 1;
  for (unsigned int arg=0; arg < nargs; arg++)
    if (!marked_[arg]) {
      if (arglist_[arg].compare(key)==0) {
        if (validInteger(arglist_[arg+1])) {
          marked_[arg]=true;
          arg++;
          int ival = convertToInteger(arglist_[arg]);
          marked_[arg]=true;
          return ival;
        }
      }
    }
  return def;
}

// ArgList::getKeyDouble()
/** Search the argument list for key, return the argument following key
  * as a double if found, otherwise return def.
  * \param key string to search for
  * \param def Value to return if key not found.
  */
double ArgList::getKeyDouble(const char *key, double def) {
  unsigned int nargs = arglist_.size() - 1;
  for (unsigned int arg=0; arg < nargs; arg++)
    if (!marked_[arg]) {
      if (arglist_[arg].compare(key)==0) {
        if (validDouble(arglist_[arg+1])) {
          marked_[arg]=true;
          arg++;
          double dval = convertToDouble(arglist_[arg]);
          marked_[arg]=true;
          return dval;
        }
      }
    }
  return def;
}

// ArgList::hasKey()
/** Search the argument list for key, mark and return true if found.
  * \param key string to search for
  * \return true if key is found, false if not.
  */
bool ArgList::hasKey(const char *key) {
  for (unsigned int arg = 0; arg < arglist_.size(); arg++) 
    if (!marked_[arg]) {
      if (arglist_[arg].compare(key)==0) {
        marked_[arg]=true;
        return true;
      }
    }
  return false;
}

// ArgList::Contains()
/** \param key string to search for
  * \return true if key is found, false if not.
  */
// NOTE: Should this be ignoring previously marked strings?
bool ArgList::Contains(const char *key) {
  for (unsigned int arg = 0; arg < arglist_.size(); arg++) 
    if (!marked_[arg]) {
      if (arglist_[arg].compare(key)==0) {
        return true;
      }
    }
  return false;
}
