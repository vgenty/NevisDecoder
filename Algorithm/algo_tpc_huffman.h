#ifndef ALGO_TPC_HUFFMAN_H
#define ALGO_TPC_HUFFMAN_H

#include "algo_tpc_xmit.h"
#include <deque>

namespace larlite {

  class algo_tpc_huffman : public algo_tpc_xmit {

  public:

    algo_tpc_huffman();

    ~algo_tpc_huffman(){};

    /// Override of algo_base::get_word_class method
    virtual inline fem::FEM_WORD get_word_class(const UInt_t word) const {
      // One of core functions to identify PMT binary word format
      if( word == 0x0 ) return fem::kUNDEFINED_WORD;
      else if( (word & 0xffffffff) == 0xffffffff ) // Unique marker
	return fem::kEVENT_HEADER;
      else if( (word & 0xffffffff) == 0xe0000000 ) // Unique marker
	return fem::kEVENT_LAST_WORD;
      if( (word & 0xffff) == 0xffff )              // Unique marker
	return fem::kFEM_HEADER;
      else if( (word & 0xf000) == 0xf000 )         // Could be ADC word
	return fem::kFEM_HEADER;
      else if( !((word>>15) & 0x1) ) {

	if( (word & 0xf000) == 0x0000 )            // Uncompressed ADC word
	  return fem::kCHANNEL_WORD;           
	else if( (word & 0xf000) == 0x4000 )       // Channel first word
	  return fem::kCHANNEL_HEADER;
	else if( (word & 0xf000) == 0x5000 )       // Channel last word
	  return fem::kCHANNEL_LAST_WORD;
	else
	  return fem::kUNDEFINED_WORD;              // Undefined
      }
      else if( ((word>>15) & 0x1) ){
	if( (word & 0xffff) == 0xc000 )            // end of FEM word
	  return fem::kFEM_LAST_WORD;
	else
	  return fem::kCHANNEL_WORD;                // compressed ADC word
      }else
	return fem::kUNDEFINED_WORD;
    };


    /**
       Re-implementation of process_word. All machinaries based on
       word maker are useless in case of huffman coded TPC data
       because an entire 15 bits are used for compressed data bits.
     */
    virtual bool process_word(const UInt_t word);

    virtual void reset();

  protected:  
    
    virtual bool check_event_quality();

    virtual void clear_event();

    virtual bool process_event_header
    (const UInt_t word, UInt_t &last_word);

    virtual bool process_fem_header
    (const UInt_t word,UInt_t &last_word);

    virtual bool process_ch_word
    (const UInt_t word,UInt_t &last_word);

    virtual bool process_fem_last_word
    (const UInt_t word,UInt_t &last_word);

    virtual bool process_event_last_word
    (const UInt_t word,UInt_t &last_word);

  protected:

    bool _search_header;
    bool _ch_last_word_allow;
  };

}

#endif
