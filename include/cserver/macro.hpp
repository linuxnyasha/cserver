#pragma once
#define LOG_TRACE                                            \
  if(this->logging.level == ::cserver::LoggingLevel::kTrace) \
  this->logging.Trace
#define LOG_DEBUG                                            \
  if(this->logging.level <= ::cserver::LoggingLevel::kDebug) \
  this->logging.Debug
#define LOG_INFO                                            \
  if(this->logging.level <= ::cserver::LoggingLevel::kInfo) \
  this->logging.Info
#define LOG_WARNING                                            \
  if(this->logging.level <= ::cserver::LoggingLevel::kWarning) \
  this->logging.Warning
#define LOG_ERROR                                            \
  if(this->logging.level <= ::cserver::LoggingLevel::kError) \
  this->logging.Error
