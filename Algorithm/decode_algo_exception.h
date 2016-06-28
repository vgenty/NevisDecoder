/**
 * \file decode_algo_exception.h
 *
 * \ingroup Algorithm
 * 
 * \brief Class def header for exception classes in binary decoder Algorithm package
 *
 * @author kazuhiro
 */

/** \addtogroup Algorithm

    @{*/
#ifndef DECODE_ALGO_EXCEPTION_H
#define DECODE_ALGO_EXCEPTION_H

#include <string>
#include <exception>

namespace larlite {
  /**
     \class decode_algo_exception
     Simple exception class for Decoder Algorithm
  */
  class decode_algo_exception : public std::exception{

  public:

    decode_algo_exception() : std::exception()
    {}

    virtual ~decode_algo_exception() throw(){};
    virtual const char* msg() const throw() 
    {return "Decoder algorithm encountered fatal error..."; }

  private:

  };

}
#endif
/** @} */ // end of doxygen group 

