/**
 * \file evb_exception.h
 *
 * \ingroup EventBuilder
 * 
 * \brief Class def header for exception classes in EventBuilder package
 *
 * @author kazuhiro
 */

/** \addtogroup EventBuilder

    @{*/
#ifndef EVB_EXCEPTION_H
#define EVB_EXCEPTION_H

#include <iostream>
#include <exception>

namespace larlite {
  /**
     \class evb_exception
     Generic (base) exception class
  */
  class evb_exception : public std::exception{

  public:

    evb_exception(std::string msg="") : std::exception(),
					  _msg("")
    {
      _msg += "\033[93m";
      _msg += "[EXCEPTION] : ";
      _msg += "\033[0m";
      _msg += "\033[91m";
      _msg += msg;
      _msg += "\033[0m";
    }

    virtual ~evb_exception() throw(){};
    virtual const char* what() const throw() 
    {return _msg.c_str(); }

  private:

    std::string _msg;
  };

}
#endif
/** @} */ // end of doxygen group 

