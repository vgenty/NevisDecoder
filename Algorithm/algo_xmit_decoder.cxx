#ifndef ALGO_XMIT_DECODER_CXX
#define ALGO_XMIT_DECODER_CXX
#include "algo_xmit_decoder.h"

namespace larlite {

  bool algo_xmit_decoder::process_header(UInt_t word){
    
    bool status=true;
    if(get_word_class(word)==fem::kEVENT_HEADER)
      {
	if(_verbosity[msg::kINFO]) {
	  sprintf(_buf,"Found event header word: %x",word);
	  Message::send(msg::kINFO,__FUNCTION__,_buf);
	}
      }else
      status = algo_slow_readout_decoder::process_header(word);
    return status;

  }

}

#endif
