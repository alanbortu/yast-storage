// Maintainer: fehr@suse.de
/* 
  Textdomain    "storage"
*/


#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/time.h>

#include <blocxx/AppenderLogger.hpp>
#include <blocxx/FileAppender.hpp>
#include <blocxx/Logger.hpp>
#include <blocxx/LogMessage.hpp>

#include "y2storage/AsciiFile.h"
#include "y2storage/StorageTmpl.h"
#include "y2storage/AppUtil.h"
#include "y2storage/SystemCmd.h"

using namespace std;

namespace storage
{

string dupDash(const string& s)
    {
    string ret(s);
    string::size_type pos = ret.find("-");
    while(pos!=string::npos)
	{
	ret.insert(pos,1,'-');
	pos = ret.find("-",pos+2);
	}
    return(ret);
    }

bool
searchFile(AsciiFile& File_Cr, string Pat_Cv, string& Line_Cr)
{
  int LineNr_ii = 0;
  return searchFile(File_Cr, Pat_Cv, Line_Cr, LineNr_ii);
}

bool
searchFile(AsciiFile& File_Cr, string Pat_Cv, string& Line_Cr, int& LineNr_ir)
{
  int End_ii;
  bool Found_bi = false;
  bool BeginOfLine_bi;
  string Tmp_Ci;
  int LineNr_ii;
  string Search_Ci(Pat_Cv);

  BeginOfLine_bi = Search_Ci.length() > 0 && Search_Ci[0] == '^';
  if (BeginOfLine_bi)
    Search_Ci.erase(0, 1);
  End_ii = File_Cr.numLines();
  LineNr_ii = LineNr_ir;
  while (!Found_bi && LineNr_ii < End_ii)
    {
      string::size_type Idx_ii;

      Tmp_Ci = File_Cr[LineNr_ii++];
      Idx_ii = Tmp_Ci.find(Search_Ci);
      if (Idx_ii != string::npos)
	{
	  if (BeginOfLine_bi)
	    Found_bi = Idx_ii == 0;
	  else
	    Found_bi = true;
	}
    }
  if (Found_bi)
    {
      Line_Cr = Tmp_Ci;
      LineNr_ir = LineNr_ii - 1;
    }
  return Found_bi;
}

void timeMark(const char*const Text_pcv, bool PrintDiff_bi)
{
  static unsigned long Start_ls;
  unsigned long Diff_li;
  struct timeb Time_ri;

  if (PrintDiff_bi)
    {
      ftime(&Time_ri);
      Diff_li = Time_ri.time % 1000000 * 1000 + Time_ri.millitm - Start_ls;
    }
  else
    {
      ftime(&Time_ri);
      Start_ls = Time_ri.time % 1000000 * 1000 + Time_ri.millitm;
    }
}

void createPath(string Path_Cv)
{
  string Path_Ci = Path_Cv;
  string Tmp_Ci;

  string::size_type Pos_ii = 0;
  while ((Pos_ii = Path_Ci.find('/', Pos_ii + 1)) != string::npos)
    {
      Tmp_Ci = Path_Ci.substr(0, Pos_ii);
      mkdir(Tmp_Ci.c_str(), 0777);
    }
  mkdir(Path_Ci.c_str(), 0777);
}

bool
checkDir(string Path_Cv)
{
  struct stat Stat_ri;

  return (stat(Path_Cv.c_str(), &Stat_ri) >= 0 &&
	  S_ISDIR(Stat_ri.st_mode));
}

bool
checkSymlink(string Path_Cv)
{
  struct stat Stat_ri;

  return (lstat(Path_Cv.c_str(), &Stat_ri) >= 0 &&
	  S_ISLNK(Stat_ri.st_mode));
}

bool
checkBlockDevice(string Path_Cv)
{
  struct stat Stat_ri;

  return (stat(Path_Cv.c_str(), &Stat_ri) >= 0 &&
	  S_ISBLK(Stat_ri.st_mode));
}

bool
checkNormalFile(string Path_Cv)
{
  struct stat Stat_ri;
  
  return (stat(Path_Cv.c_str(), &Stat_ri) >= 0 &&
	  S_ISREG(Stat_ri.st_mode));
}

string extractNthWord(int Num_iv, string Line_Cv, bool GetRest_bi)
  {
  string::size_type pos;
  int I_ii=0;
  string Ret_Ci = Line_Cv;

  if( Ret_Ci.find_first_of(app_ws)==0 )
    {
    pos = Ret_Ci.find_first_not_of(app_ws);
    if( pos != string::npos )
        {
        Ret_Ci.erase(0, pos );
        }
    else
        {
        Ret_Ci.erase();
        }
    }
  while( I_ii<Num_iv && Ret_Ci.length()>0 )
    {
    pos = Ret_Ci.find_first_of(app_ws);
    if( pos != string::npos )
        {
        Ret_Ci.erase(0, pos );
        }
    else
        {
        Ret_Ci.erase();
        }
    if( Ret_Ci.find_first_of(app_ws)==0 )
        {
        pos = Ret_Ci.find_first_not_of(app_ws);
        if( pos != string::npos )
            {
            Ret_Ci.erase(0, pos );
            }
        else
            {
            Ret_Ci.erase();
            }
        }
    I_ii++;
    }
  if (!GetRest_bi && (pos=Ret_Ci.find_first_of(app_ws))!=string::npos )
      Ret_Ci.erase(pos);
  return Ret_Ci;
  }

list<string> splitString( const string& s, const string& delChars, 
		          bool multipleDelim, bool skipEmpty,
			  const string& quotes )
    {
    string::size_type pos;
    string::size_type cur = 0;
    string::size_type nfind = 0;
    list<string> ret;

    while( cur<s.size() && (pos=s.find_first_of(delChars,nfind))!=string::npos )
	{
	if( pos==cur )
	    {
	    if( !skipEmpty )
		ret.push_back( "" );
	    }
	else
	    ret.push_back( s.substr( cur, pos-cur ));
	if( multipleDelim )
	    {
	    cur = s.find_first_not_of(delChars,pos);
	    }
	else
	    cur = pos+1;
	nfind = cur;
	if( !quotes.empty() )
	    {
	    string::size_type qpos=s.find_first_of(quotes,cur);
	    string::size_type lpos=s.find_first_of(delChars,cur);
	    if( qpos!=string::npos && qpos<lpos && 
	        (qpos=s.find_first_of(quotes,qpos+1))!=string::npos )
		{
		nfind = qpos;
		}
	    }
	}
    if( cur<s.size() )
	ret.push_back( s.substr( cur ));
    if( !skipEmpty && !s.empty() && s.find_last_of(delChars)==s.size()-1 )
	ret.push_back( "" );
    //y2mil( "ret:" << ret );
    return( ret );
    }

string mergeString( const list<string>& l, const string& del )
    {
    string ret;
    for( list<string>::const_iterator i=l.begin(); i!=l.end(); ++i )
	{
	if( i!=l.begin() )
	    ret += del;
	ret += *i;
	}
    return( ret );
    }

map<string,string> 
makeMap( const list<string>& l, const string& delim, const string& removeSur )
    {
    map<string,string> ret;
    for( list<string>::const_iterator i=l.begin(); i!=l.end(); ++i )
	{
	string k, v;
	string::size_type pos;
	if( (pos=i->find_first_of( delim ))!=string::npos )
	    {
	    k = i->substr( 0, pos );
	    string::size_type pos2 = i->find_first_not_of( delim, pos+1 );
	    if( pos2 != string::npos )
		v = i->substr( pos2 );
	    }
	if( !removeSur.empty() )
	    {
	    if( (pos=k.find_first_of(removeSur)) != string::npos )
		k.erase( 0, k.find_first_not_of(removeSur) );
	    if( !k.empty() && (pos=k.find_last_of(removeSur))==k.size()-1 )
		k.erase( k.find_last_not_of(removeSur)+1 );
	    if( (pos=v.find_first_of(removeSur)) != string::npos )
		v.erase( 0, v.find_first_not_of(removeSur) );
	    if( !v.empty() && (pos=v.find_last_of(removeSur))==v.size()-1 )
		v.erase( v.find_last_not_of(removeSur)+1 );
	    }
	if( !k.empty() && !v.empty() )
	    ret[k] = v;
	}
    return( ret );
    }


void putNthWord(int Num_iv, string Word_Cv, string& Line_Cr)
{
  string Last_Ci = extractNthWord(Num_iv, Line_Cr, true);
  int Len_ii = Last_Ci.find_first_of(app_ws);
  Line_Cr.replace(Line_Cr.length() - Last_Ci.length(), Len_ii, Word_Cv);
}


void removeLastIf (string& Text_Cr, char Char_cv)
{
  if (Text_Cr.length() > 0 && Text_Cr[Text_Cr.length() - 1] == Char_cv)
    Text_Cr.erase(Text_Cr.length() - 1);
}

string normalizeDevice( const string& dev )
    {
    string ret( dev );
    normalizeDevice( ret );
    return( ret );
    }

bool isNfsDev( const string& dev )
    {
    return( !dev.empty() && dev[0]!='/' &&
            dev.find( ':' )!=string::npos );
    }

void normalizeDevice( string& dev )
    {
    if( dev.find( "/dev/" )!=0 && !isNfsDev(dev) )
	dev = "/dev/" + dev;
    }

string undevDevice( const string& dev )
    {
    string ret( dev );
    undevDevice( ret );
    return( ret );
    }

void undevDevice( string& dev )
    {
    if( dev.find( "/dev/" )==0 )
	dev.erase( 0, 5 );
    }

static blocxx::String component = "libstorage";

void
log_msg( unsigned level, const char* file, unsigned line, const char* func,
         const char* add_str, const char* format, ... ) 
    {
    using namespace blocxx;

#if BLOCXX_LIBRARY_VERSION >= 5
    ELogLevel   curLevel = LogAppender::getCurrentLogAppender()->getLogLevel();
#else
    ELogLevel   curLevel = Logger::getCurrentLogger()->getLogLevel();
#endif
    String      category;
    switch( level )
	{
	case 0:
	    if( curLevel >= ::blocxx::E_DEBUG_LEVEL)
	    	category = Logger::STR_DEBUG_CATEGORY;
	    break;
	case 1:
	    if( curLevel >= ::blocxx::E_INFO_LEVEL)
	    	category = Logger::STR_INFO_CATEGORY;
	    break;
	case 2:
#if BLOCXX_LIBRARY_VERSION >= 5
	    if( curLevel >= ::blocxx::E_WARNING_LEVEL)
		category = Logger::STR_WARNING_CATEGORY;
#else
	    if( curLevel >= ::blocxx::E_INFO_LEVEL)
		{
		category = Logger::STR_INFO_CATEGORY;
		if( add_str == NULL )
			add_str = "[WARNING]";
		}
#endif
	    break;
	case 3:
	    if( curLevel >= ::blocxx::E_ERROR_LEVEL)
		category = Logger::STR_ERROR_CATEGORY;
	    break;
	default:
	    if( curLevel >= ::blocxx::E_FATAL_ERROR_LEVEL)
		category = Logger::STR_FATAL_CATEGORY;
	    break;
	}

    if( !category.empty())
	{
	char b[4096+1] = {'\0'};
	unsigned ret = 0;
	if( add_str != NULL && *add_str != '\0')
	    ret = snprintf( b, sizeof(b), "%s ", add_str );
	if( ret<sizeof(b) )
	    {
	    va_list p;
	    va_start( p, format );
	    vsnprintf( b+ret, sizeof(b)-ret, format, p );
	    }
	b[sizeof(b)-1] = 0;

#if BLOCXX_LIBRARY_VERSION >= 5
	LogAppender::getCurrentLogAppender()->logMessage( LogMessage(component, category, String(b), file, line , func) );
#else
	Logger::getCurrentLogger()->logMessage( component, category, String(b), file, line , func );
#endif
	}
    }

int createLogger( const string& lcomponent, const string& name,
                  const string& logpath, const string& logfile )
    {
    using namespace blocxx;

#if BLOCXX_LIBRARY_VERSION <  5
    // Add PID to our global component name in blocxx-1.x
    // that does not support the %P log format specifier.
    component = "libstorage(" + decString(getpid()) + ")";
#endif

    if( logpath != "NULL" && logfile != "NULL" )
	{
	String nm = name.c_str();
	LoggerConfigMap configItems;
	LogAppenderRef logApp;
	if( logpath != "STDERR" && logfile != "STDERR" && 
	    logpath != "SYSLOG" && logfile != "SYSLOG" )
	    {
	    String StrKey;
	    String StrPath;
	    StrKey.format("log.%s.location", name.c_str());
	    StrPath = (logpath + "/" + logfile).c_str();
	    configItems[StrKey] = StrPath;
	    logApp = 
		LogAppender::createLogAppender( nm, LogAppender::ALL_COMPONENTS,
						LogAppender::ALL_CATEGORIES,
#if BLOCXX_LIBRARY_VERSION >= 5
						"%d %-5p %c(%P) %F(%M):%L - %m", 
#else
						"%d %-5p %c %F(%M):%L - %m",
#endif
						LogAppender::TYPE_FILE, 
						configItems );
	    }
	else if( logpath == "STDERR" && logfile == "STDERR" )
	    {
	    logApp = 
		LogAppender::createLogAppender( nm, LogAppender::ALL_COMPONENTS,
						LogAppender::ALL_CATEGORIES,
#if BLOCXX_LIBRARY_VERSION >= 5
						"%d %-5p %c(%P) %F(%M):%L - %m",
#else
						"%d %-5p %c %F(%M):%L - %m",
#endif
						LogAppender::TYPE_STDERR, 
						configItems );
	    }
	else 
	    {
	    logApp = 
		LogAppender::createLogAppender( nm, LogAppender::ALL_COMPONENTS,
						LogAppender::ALL_CATEGORIES,
#if BLOCXX_LIBRARY_VERSION >= 5
						"%d %-5p %c(%P) %F(%M):%L - %m",
#else
						"%d %-5p %c %F(%M):%L - %m",
#endif
						LogAppender::TYPE_SYSLOG, 
						configItems );
	    }
#if BLOCXX_LIBRARY_VERSION >= 5
	LogAppender::setDefaultLogAppender(logApp);
#else
	LoggerRef log( new AppenderLogger( lcomponent.c_str(), E_INFO_LEVEL, 
	                                   logApp));
	Logger::setDefaultLogger(log);
#endif
	}
    return( 0 );
    }

void tolower( string& s )
    {
    for( string::iterator i=s.begin(); i!=s.end(); i++ )
	{
	*i = std::tolower(*i);
        }
    }

void getFindMap( const char* path, map<string,string>& m, bool unique )
    {
    y2mil( "path: " << path << " unique:" << unique );
    m.clear();
    if( access( path, R_OK )==0 )
	{
	string cmd = "/usr/bin/find ";
	cmd += path;
	cmd += " -type l -printf '%f %l\n'";
	SystemCmd findcmd( cmd.c_str() );
	list<string> l;
	findcmd.getStdout( l );
	list<string>::iterator i=l.begin();
	while( i!=l.end() )
	    {
	    list<string> tlist = splitString( *i );
	    if( tlist.size()==2 )
		{
		string& tmp = tlist.back();
		string dsk = tmp.substr( tmp.find_first_not_of( "./" ) );
		map<string,string>::const_iterator mi = m.find(dsk);
		if( unique || mi==m.end() )
		    m[dsk] = tlist.front();
		else
		    {
		    m[dsk] += " ";
		    m[dsk] += tlist.front();
		    }
		}
	    ++i;
	    }
	}
    y2mil( "map: " << m );
    }

void getFindRevMap( const char* path, map<string,string>& ret )
    {
    y2mil( "path: " << path );
    map<string,string> m;
    if( access( path, R_OK )==0 )
	{
	string cmd = "/bin/ls -lt ";
	cmd += path;
	SystemCmd findcmd( cmd.c_str() );
	list<string> l;
	findcmd.getStdout( l );
	list<string>::iterator i=l.begin();
	while( i!=l.end() )
	    {
	    list<string> tlist = splitString( *i );
	    string dev, id;
	    y2mil( "tlist:" << tlist );
	    if( !tlist.empty() )
		{
		dev = tlist.back();
		tlist.pop_back();
		dev.erase( 0, dev.find_first_not_of( "./" ) );
		if( !tlist.empty() && tlist.back() == "->" )
		    {
		    tlist.pop_back();
		    if( !tlist.empty() )
			id = tlist.back();
		    }
		if( !id.empty() && !dev.empty() )
		    {
		    map<string,string>::iterator mi = m.find( dev );
		    if( mi == m.end() )
			{
			m[dev] = id;
			}
		    else
			y2mil( "already here dev:" << mi->first <<
			       " id:" << mi->second );
		    }
		}
	    ++i;
	    }
	ret.clear();
	for( map<string,string>::iterator mi = m.begin(); mi!=m.end(); ++mi )
	    {
	    ret[mi->second] = mi->first;
	    }
	}
    y2mil( "map: " << ret );
    }

unsigned getMajorDevices( const string& driver )
    {
    unsigned ret=0;
    string cmd = "grep \" " + driver + "$\" /proc/devices";
    SystemCmd c( cmd );
    if( c.numLines()>0 )
	{
	extractNthWord( 0, *c.getLine(0)) >> ret;
	}
    y2mil( "driver:" << driver << " ret:" << ret );
    return( ret );
    }

string sformat(const char* format, ...)
{
    char* result;
    va_list ap;

    va_start(ap, format);
    if (vasprintf(&result, format, ap) == -1)
	return string();
    va_end(ap);

    string str(result);
    free(result);
    return str;
}

bool system_cmd_testmode = false;
const string app_ws = " \t\n";

}
