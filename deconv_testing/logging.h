#ifndef __LOGGING_INCLUDED__
#define __LOGGING_INCLUDED__
#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <ctime>

#include "macros.h"
#include "str_printf.h"

class Logger {
public:

	inline static std::string getPrefixStringFuncFileLine(const char* tag, const char* lineNum, const char* function, const char* file) {
		std::string prefixString(std::string(tag) + " \"" + function + "\" " + file + "-" + lineNum + ": ");
		return(prefixString);
	}

	inline static std::string getPrefixStringFileLine(const char* tag, const char* lineNum, const char* function, const char* file) {
		std::string prefixString(std::string(tag) + " " + file + "-" + lineNum + ": ");
		return(prefixString);
	}

	inline static std::string getPrefixStringTimeFileLine(const char* tag, const char* lineNum, const char* function, const char* file) {
		struct tm t;
		std::time_t tbuf=std::time(nullptr);
		char timebuf[32];

		gmtime_r(&tbuf, &t);
		std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S", &t);

		std::string prefixString(std::string(tag) + " "+ timebuf + " " + file + "-" + lineNum + ": ");
		return(prefixString);
	}


	inline static std::string getLineContinueStringDashes(std::string prefixString) {
		for (unsigned i = 0; i < prefixString.size() - 2; i++) {
			prefixString[i] = '-';
		}
		return(prefixString);
	}


	// Instance data
	typedef std::string(*PrefixStringGetter)(const char*, const char*, const char*, const char*);

	const char* file = "\0";
	const char* function = "\0";
	std::ostream& info_stream = std::cout;
	std::ostream& err_stream = std::cerr;
	PrefixStringGetter getPrefixString = getPrefixStringFileLine;

	// Constructors
	Logger(const char* _file, const char* _function, std::ostream& _ios, std::ostream& _eos, PrefixStringGetter _getPrefixString) :
		file(_file), function(_function), info_stream(_ios), err_stream(_eos),
		getPrefixString(_getPrefixString) {}

	Logger(std::string _file, std::string _function, std::ostream& _ios, std::ostream& _eos, PrefixStringGetter _getPrefixString) :
		file(_file.c_str()), function(_function.c_str()), info_stream(_ios), err_stream(_eos),
		getPrefixString(_getPrefixString) {}

	Logger(const char* _file, const char* _function) : 
		Logger(_file, _function, std::cout, std::cerr, getPrefixStringFileLine) {}

	Logger(std::string _file, std::string _function) :
		Logger(_file, _function, std::cout, std::cerr, getPrefixStringFileLine) {}

	Logger() : 
		Logger("\0", "\0", std::cout, std::cerr, getPrefixStringFileLine) {}

	Logger(const Logger& lgr) : 
		Logger(lgr.file, lgr.function, lgr.info_stream, lgr.err_stream, lgr.getPrefixString) {};


	// Member functions

	template <class ...Types>
	inline void log_message(std::ostream& os, const char* tag, const char* line, const char* fmt, Types&& ...args) {
		std::string prefixString = getPrefixString(tag, line, function, file);
		os << prefixString;
		_oprintf(os, getLineContinueStringDashes(prefixString).c_str(), fmt, args...);
	}

	template <class ...Types>
	inline void log(std::ostream& os, const char* tag, const char* line, const char* fmt, Types&& ...args) {
		log_message(os, tag, line, fmt, args...);
	}

	template <class ...Types>
	inline void logToInfoOut(const char* tag, const char* line, const char* fmt, Types&& ...args) {
		log_message(info_stream, tag, line, fmt, args...);
	}

	template <class ...Types>
	inline void logToErrorOut(const char* tag, const char* line, const char* fmt, Types&& ...args) {
		log_message(err_stream, tag, line, fmt, args...);
	}
};



class LoggerManager {
public:
	inline static std::map<std::string, unsigned> logReportLevels{ {"UNDEFINED",0},{"DEBUG",10},{"INFO",20},{"WARN",30},{"ERROR",40},{"CRIT",50},{"HIGHEST",100} };

	inline static std::map<std::string, Logger> loggers;
	inline static unsigned globalReportLevel = logReportLevels["WARN"];
	inline static unsigned overwriteReportLevel = logReportLevels["HIGHEST"];
	inline static std::map<std::string, unsigned> localReportLevel;
	inline static std::string _tempMessageBuf;

	inline static std::string getLoggerFileFunctionId(Logger& lgr) {
		std::string ffid = std::string(lgr.file) + lgr.function;
		return(ffid);
	}

	template <class ...Types>
	inline static Logger& create(std::string reportLevelName, Types&& ... args) {
		Logger lgr(args...);
		std::string lgrId = getLoggerFileFunctionId(lgr);
		if (loggers.count(lgrId) < 1) {
			loggers.emplace(lgrId, lgr);
		}
		localReportLevel[lgrId] = logReportLevels[reportLevelName];
		return(loggers[lgrId]);
	}

	template <class ...Types>
	inline static void logToInfoOut(
			Logger& lgr, 
			const char* tag, 
			const char* lineNum, 
			const char* fmt, 
			Types&& ... args
		) {
		lgr.logToInfoOut(tag, lineNum, fmt, args...);
	}

	template <class ...Types>
	inline static void logToErrorOut(
			Logger& lgr, 
			const char* tag, 
			const char* lineNum, 
			const char* fmt, 
			Types&& ... args
		) {
		lgr.logToErrorOut(tag, lineNum, fmt, args...);
	}

	template <class ...Types>
	inline static void logAtLevel(
			Logger& lgr, 
			std::string reportLevelName, 
			const char* lineNum, 
			const char* fmt, 
			Types&& ... args
		) {
		unsigned reportLevel = logReportLevels[reportLevelName];
		//std::cout << reportLevel << std::endl;
		//std::cout << reportLevelName << std::endl;
		//std::cout << localReportLevel[getLoggerFileFunctionId(lgr)] << std::endl;

		if (reportLevel >= overwriteReportLevel || ((reportLevel >= globalReportLevel) && (reportLevel >= localReportLevel[getLoggerFileFunctionId(lgr)]))) {
			if (reportLevel <= logReportLevels["WARN"]) {
				lgr.logToInfoOut(reportLevelName.c_str(), lineNum, fmt, args...);
			}
			else {
				lgr.logToErrorOut(reportLevelName.c_str(), lineNum, fmt, args...);
			}
		}
	}
	template <class ...Types>
	inline static void logAtLevelNewline(
			Logger& lgr, 
			std::string reportLevelName, 
			const char* lineNum, 
			const char* fmt, 
			Types&& ... args
		) {
		std::string nl_fmt = std::string(fmt) + "\n";
		logAtLevel(lgr, reportLevelName, lineNum, nl_fmt.c_str(), args...);
	}

	inline static std::string stringifiedVariadicArgsToFmt(const char* astrs) {
		std::string str;

		for (; *astrs != '\0'; ++astrs) {
			if (*astrs == ',') {
				str += " %";
			}
			else {
				str += *astrs;
			}
		}
		str += " %";
		return(str.c_str());
	}

	template <class ...Types>
	inline static void startTempMessage(Types&& ...args) {
		_tempMessageBuf = _sprintf(args...);
	}
	template <class ...Types>
	inline static void continueTempMessage(Types&& ...args) {
		_tempMessageBuf += _sprintf(args...);
	}
};



#ifndef LOGGING_ENABLED
	#define LOGGING_ENABLED true
#endif

#if LOGGING_ENABLED
	// mangled variable name of logger so we shouldn't have name collisions easily.
	#define _LGR _some_name_mangled_variable_name_u743t2996 

	// set up logging for whole program
	#define INIT_LOGGING(level) LoggerManager::globalReportLevel = LoggerManager::logReportLevels[(level)]

	// overwrite minimum logging level for everything
	#define	FORCE_LOGGING(level) LoggerManager::overwriteReportLevel = LoggerManager::logReportLevels[(level)]

	// call to get a logger for a function
	#define GET_LOGGER Logger& _LGR = LoggerManager::create("UNDEFINED", SRC_FILE, __PRETTY_FUNCTION__)
	#define GET_LOGGER_AT_LEVEL(level) Logger& _LGR = LoggerManager::create((level), SRC_FILE, __PRETTY_FUNCTION__)

	// call to use temporary message space
	#define LOG_MSG_START(...) LoggerManager::startTempMessage(__VA_ARGS__)
	#define LOG_MSG_CONT(...) LoggerManager::continueTempMessage(__VA_ARGS__)
	#define LOG_MSG_GET LoggerManager::_tempMessageBuf

	// call to send messages
	// append newline
	// lOG_XXX(var1, var2); -> "XXX <log info>: <formatted output>\n"
	#define LOG_DEBUG(...) LoggerManager::logAtLevelNewline(_LGR, "DEBUG", STR(__LINE__), __VA_ARGS__)
	#define LOG_INFO(...) LoggerManager::logAtLevelNewline(_LGR, "INFO", STR(__LINE__), __VA_ARGS__)
	#define LOG_WARN(...) LoggerManager::logAtLevelNewline(_LGR, "WARN", STR(__LINE__), __VA_ARGS__)
	#define LOG_ERROR(...) LoggerManager::logAtLevelNewline(_LGR, "ERROR", STR(__LINE__), __VA_ARGS__)
	#define LOG_CRIT(...) LoggerManager::logAtLevelNewline(_LGR, "CRIT", STR(__LINE__), __VA_ARGS__)
	// does not append newline
	// lOG_XXX(var1, var2); -> "XXX <log info>: <formatted output>"
	#define LOGP_DEBUG(...) LoggerManager::logAtLevel(_LGR, "DEBUG", STR(__LINE__), __VA_ARGS__)
	#define LOGP_INFO(...) LoggerManager::logAtLevel(_LGR, "INFO", STR(__LINE__), __VA_ARGS__)
	#define LOGP_WARN(...) LoggerManager::logAtLevel(_LGR, "WARN", STR(__LINE__), __VA_ARGS__)
	#define LOGP_ERROR(...) LoggerManager::logAtLevel(_LGR, "ERROR", STR(__LINE__), __VA_ARGS__)
	#define LOGP_CRIT(...) LoggerManager::logAtLevel(_LGR, "CRIT", STR(__LINE__), __VA_ARGS__)
	// logs variable name and value, appends newline
	// lOGV_XXX(var1, var2); -> "XXX <log info>: var1 <value> var2 <value>\n"
	#define LOGV_DEBUG(...) LoggerManager::logAtLevelNewline(_LGR, "DEBUG", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
	#define LOGV_INFO(...) LoggerManager::logAtLevelNewline(_LGR, "INFO", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
	#define LOGV_WARN(...) LoggerManager::logAtLevelNewline(_LGR, "WARN", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
	#define LOGV_ERROR(...) LoggerManager::logAtLevelNewline(_LGR, "ERROR", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
	#define LOGV_CRIT(...) LoggerManager::logAtLevelNewline(_LGR, "CRIT", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
	// logs variable name and value, does not append newline
	// lOGVP_XXX(var1, var2); -> "XXX <log info>: var1 <value> var2 <value>"
	#define LOGVP_DEBUG(...) LoggerManager::logAtLevel(_LGR, "DEBUG", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
	#define LOGVP_INFO(...) LoggerManager::logAtLevel(_LGR, "INFO", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
	#define LOGVP_WARN(...) LoggerManager::logAtLevel(_LGR, "WARN", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
	#define LOGVP_ERROR(...) LoggerManager::logAtLevel(_LGR, "ERROR", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
	#define LOGVP_CRIT(...) LoggerManager::logAtLevel(_LGR, "CRIT", STR(__LINE__), LoggerManager::stringifiedVariadicArgsToFmt(#__VA_ARGS__).c_str(),__VA_ARGS__)
#else
	// define all logging macros to be empty
	#define _LGR 

	#define INIT_LOGGING(level)
	
	#define	FORCE_LOGGING(level)

	#define GET_LOGGER 
	#define GET_LOGGER_AT_LEVEL(level)

	#define LOG_MSG_START(...) 
	#define LOG_MSG_CONT(...) 
	#define LOG_MSG_GET 

	#define LOG_DEBUG(...) 
	#define LOG_INFO(...)
	#define LOG_WARN(...) 
	#define LOG_ERROR(...)
	#define LOG_CRIT(...) 

	#define LOGP_DEBUG(...) 
	#define LOGP_INFO(...) 
	#define LOGP_WARN(...) 
	#define LOGP_ERROR(...) 
	#define LOGP_CRIT(...)

	#define LOGV_DEBUG(...) 
	#define LOGV_INFO(...) 
	#define LOGV_WARN(...) 
	#define LOGV_ERROR(...) 
	#define LOGV_CRIT(...) 

	#define LOGVP_DEBUG(...) 
	#define LOGVP_INFO(...) 
	#define LOGVP_WARN(...) 
	#define LOGVP_ERROR(...) 
	#define LOGVP_CRIT(...) 
#endif //LOGGING_ENABLED

#endif //__LOGGING_INCLUDED__
