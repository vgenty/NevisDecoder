/**
 * \file bin_io_handler.h
 *
 * \ingroup FileIO
 * 
 * \brief Class def header for binary/ascii file I/O
 *
 * @author Kazu - Nevis 2013
 */

/** \addtogroup FileIO

    @{*/
#ifndef BIN_IO_HANDLER_H
#define BIN_IO_HANDLER_H

#include "Base/larlite_base.h"
//forward declarations
#include "Base/Base-TypeDef.h"
#include "Base/DecoderConstants.h"
#include "Base/FEMConstants.h"
#include <cstdio>

namespace larlite {

  /**
     \class bin_io_handler
     Operates on bin/ASCII file and read-out 32-bit words consecutively.   
  */
  class bin_io_handler : public larlite_base {

  public:
    
    /// I/O mode enum
    enum MODE{
      READ, ///< READ mode
      kWRITE ///< kWRITE mode
    };
    
    /// Process status enum
    enum STATUS{
      INIT,      ///< Status initialized, before opening a file
      OPENED,    ///< File opened, before start operation
      OPERATING, ///< Operating I/O, before closing file
      CLOSED     ///< Final status: closed a file.
    };
    
    /// Default constructor
    bin_io_handler(FORMAT::INPUT_FILE type=FORMAT::UNDEFINED);

    /// Default destructor
    ~bin_io_handler(){};
    
    /// Send a word to this method to write out in kWRITE mode.
    bool write_word(const unsigned word);
    
    /// Send an array of words to this method to write out in kWRITE mode.
    bool write_multi_word(const unsigned *words, const size_t entries);
    
    /// Receive 32-bit word from this method in READ mode.
    unsigned read_word();
    
    /// Setter for the input file format defined in FORMAT::INPUT_FILE
    void set_format(FORMAT::INPUT_FILE f) {_format=f;};
    
    /// Setter for I/O mode defined in bin_io_handler::MODE
    void set_mode(MODE mode)              {_mode=mode;};
    
    /// Setter for input/output file name
    void set_filename(std::string name)   {_filename=name;};
    
    /// A method to return if eof is reached or not.
    bool eof() const {return _eof;};
    
    /// A method to open input/output file
    bool open();

    /// A method to check if a file is still in open state.
    bool is_open() const;
    
    /// A method to close input/output file
    void close();
    
    /// A method to initialize the class instance.
    void reset();
    
    unsigned read_multi_word(size_t length=0);
    
  private:
    
    std::string _filename;    ///< Input/Output filename
    
    bool   _eof;                  ///< EOF boolean
    unsigned short _nwords_in_file;   ///< Number of words read or written per file
    unsigned   _checksum;         ///< checksum of operated 32-bit words
    size_t   _file_suffix;      ///< A counter for file suffix in case output is too big.
    
    unsigned _single_word[1];///< A place holder variable to process a single word
    unsigned _word;          ///< A word set to be read/write-out
    unsigned _nchars;        ///< Number of chars read in case of ASCII file
    FORMAT::INPUT_FILE _format; ///< Input file format
    STATUS _status;             ///< Process status flag
    MODE   _mode;               ///< I/O mode
    FILE* _handler;             ///< File handler pointer
    
    
    std::vector<unsigned> _read_word_buffer;
    size_t _multi_word_index;
  };
}  
#endif
/** @} */ // end of doxygen group 
