#ifndef ALGO_TPC_HUFFMAN_CXX
#define ALGO_TPC_HUFFMAN_CXX

#include "algo_tpc_huffman.h"

namespace larlite {

  algo_tpc_huffman::algo_tpc_huffman() : algo_tpc_xmit()
  {
    _name = "algo_tpc_huffman";
    reset();
  }  

  void algo_tpc_huffman::reset() {

    algo_tpc_xmit::reset();
    _ch_last_word_allow=false;
    
  }

  void algo_tpc_huffman::clear_event() {

    _search_header = false;

    algo_tpc_xmit::clear_event();

  }

  bool algo_tpc_huffman::process_word(const UInt_t word) {

    // If in back_trace mode, add this word in record
    if(_bt_mode){

      // Check if buffer is filled
      if(!_bt_nwords_filled)
	_bt_nwords_filled = (_bt_nwords == _bt_words.size());

      // If filled, remove the oldest element
      if(_bt_nwords_filled)
	_bt_words.pop_front();

      // Add new word
      _bt_words.push_back(word);

    }

    if(word==0x0)

      return true;

    bool status = true;
    UInt_t word_class = get_word_class(word);
    UInt_t last_word_class = get_word_class(_last_word);

    if( _search_for_next_event ) {

      if( !(word_class == fem::kEVENT_HEADER && 
	    last_word_class == fem::kEVENT_LAST_WORD ) ) {

	_last_word = word;
	return status;
      }

    }

    //
    // In case of huffman coded data stream, uniquely marked
    // words are only:
    // (*) event header
    // (*) event last word 
    // (*) channel header
    // (*) channel last word
    //
    switch(word_class){

    case fem::kEVENT_HEADER:

      if(_verbosity[msg::kDEBUG])
	Message::send(msg::kDEBUG, __FUNCTION__, Form("See FEM:kEVENT_HEADER 0x%x",word));
      
      
      if( (last_word_class == fem::kEVENT_LAST_WORD && !(_header_info.nwords)) || !(_event_data)) {

	_search_for_next_event = false;

	status = process_event_header(word,_last_word);
      }
      else{

	UInt_t first_word  = (word & 0xffff);
	UInt_t second_word = (word >> 16);

	status = process_ch_word(first_word,_last_word);

	if(status) status = process_ch_word(second_word,_last_word);

      }

      break;

    case fem::kFEM_HEADER:

      if(_verbosity[msg::kDEBUG])
	Message::send(msg::kDEBUG, __FUNCTION__, Form("See FEM:kFEM_HEADER 0x%x",word));
      
      if(_nwords && _nwords == _header_info.nwords){

	//_nwords++;
	if(_ch_data.size()) {
	  _ch_data.set_module_id(_header_info.module_id);
	  _ch_data.set_module_address(_header_info.module_address);
	  if (!_event_data) {
	      Message::send(msg::kWARNING,__FUNCTION__,
			    Form("_event_data is nill (0x%x)",_event_data));
	      throw std::exception();
	    }
	  _event_data->push_back(_ch_data);
	  _ch_data.clear_data();
	}
	status = store_event();

      }

      if(status){ 

	if(!(_header_info.nwords))

	  status = process_fem_header(word,_last_word);

	else{

	  UInt_t first_word  = (word & 0xffff);
	  UInt_t second_word = (word >> 16);

	  status = process_ch_word(first_word,_last_word);

	  if(status) status = process_ch_word(second_word,_last_word);

	}
      }

      break;

    case fem::kEVENT_LAST_WORD:

      if(_verbosity[msg::kDEBUG])
	Message::send(msg::kDEBUG, __FUNCTION__, Form("See FEM:kEVENT_LAST_WORD 0x%x",word));
      
      //if(last_word_class == fem::kCHANNEL_LAST_WORD){

      // Attempt to store data if nwords matches with the expected number
      if(_verbosity[msg::kWARNING])
	Message::send(msg::kWARNING, __FUNCTION__, Form("Trying to store, status is %d and _nwords is %d _header_info.nwords %d",
							status,_nwords,_header_info.nwords));
      if(status && _nwords != _header_info.nwords)
	if(_verbosity[msg::kERROR])
	  Message::send(msg::kERROR, __FUNCTION__, Form("Force the issue, store... this... event!!!!"));
							
	
      //_nwords++;
      _checksum += word;
      if(_ch_data.size()) {
	_ch_data.set_module_id(_header_info.module_id);
	_ch_data.set_module_address(_header_info.module_address);
	_event_data->push_back(_ch_data);
	_ch_data.clear_data();
      }
      status = store_event();
	//}

      status = process_event_last_word(word,_last_word);

      break;

      //}

    default:

      if(_verbosity[msg::kDEBUG])
	Message::send(msg::kDEBUG, __FUNCTION__, Form("See: 0x%x",word));
      
      UInt_t first_word  = (word & 0xffff);
      UInt_t second_word = (word >> 16);

      //size_t current_index=_ch_data.size();
      status = process_ch_word(first_word,_last_word);

      if(status) status = process_ch_word(second_word,_last_word);
      
    }

    if(!status){

      if(_bt_mode)
	backtrace();

      clear_event();

      if(_debug_mode){

	Message::send(msg::kWARNING,__FUNCTION__,"kDEBUG MODE => Continue to next event...");

	_search_for_next_event = true;

      }

    }

    return status;
  }

  bool algo_tpc_huffman::process_event_header(const UInt_t word,
					      UInt_t &last_word) 
  {
    bool status = true;
    Message::send(msg::kINFO,__FUNCTION__,
		Form("Processing event header with _event_data %x",_event_data));

    if(!_event_data)

      _event_data = (event_tpcfifo*)(_storage->get_data<event_tpcfifo>("tpcfifo"));

    else if(get_word_class(last_word)!=fem::kEVENT_LAST_WORD){

      Message::send(msg::kERROR,__FUNCTION__,
		    Form("Unexpected event header (%x) with the previous word %x!",word,last_word));

      status = false;

    }

    last_word = word;

    return status;
  }

  //#########################################################
  bool algo_tpc_huffman::check_event_quality(){
  //#########################################################

    bool status = true;

    if(_verbosity[msg::kINFO]) Message::send( msg::kINFO,__FUNCTION__, "algo_tpc_xmit" );
    // Check if _checksum and _nwords agrees with that of event header.
    // Yun-Tse 2014/11/19: Perhaps this _nwords-=1; was for some old data format?
    // _nwords-=1;
    if(_nwords!=_header_info.nwords){

      Message::send(msg::kERROR,__FUNCTION__,
		    Form("Disagreement on nwords: counted=%u, expected=%u",_nwords,_header_info.nwords));
      Message::send(msg::kWARNING,__FUNCTION__,
		    Form("VIC DOESNT CARE"));
      //status = false;
      status = true;

    }

    //if(_checksum != _header_info.checksum)
    if((_checksum & 0xffffff) !=_header_info.checksum){
      
      if( ((_checksum + 0x503f) & 0xffffff) == _header_info.checksum) {
	Message::send(msg::kWARNING,__FUNCTION__,
		      Form("Fix-able checksum disagreement: summed=%x, expected=%x (Event=%d,FEM=%d), w/ word count=%d (#ch = %zu)",
			   _checksum,
			   _header_info.checksum,
			   _header_info.event_number,
			   _header_info.module_address,
			   _nwords,
			   _event_data->size()));
	_ch_last_word_allow = true;
      }
      else {
	Message::send(msg::kERROR,__FUNCTION__,
		      Form("Disagreement on checksum: summed=%x, expected=%x",_checksum,_header_info.checksum));
	Message::send(msg::kWARNING,__FUNCTION__,
		      Form("VIC DOESNT CARE EITHER"));
	//status = false;
	status = true;
      }
    }

    return status;

  }

  bool algo_tpc_huffman::process_fem_header(const UInt_t word,
					    UInt_t &last_word) 
  {
    bool status = true;
    //
    // This function must be called for true FEM headers.
    // Hence we implement a strict check to make sure this is FEM header
    //
    UInt_t last_word_class = get_word_class(last_word);

    if(_ch_last_word_allow)
      Message::send(msg::kWARNING,__FUNCTION__,
		    "Detected last-ch-last-word-missing issue in the previous event FYI...");

    if(last_word_class != fem::kEVENT_HEADER &&
       last_word_class != fem::kFEM_HEADER &&
       last_word_class != fem::kCHANNEL_LAST_WORD &&
       !_ch_last_word_allow) {

      Message::send(msg::kERROR,__FUNCTION__,
		    Form("Unexpected FEM header (%x) with the previous word %x!",word,last_word));

      status = false;
    }else if(get_word_class(word>>16)!=fem::kFEM_HEADER) {

      // Event header should come as a 32-bit word which is a pair of two 16-bit header words.
      // The first 16-bit is already checked by this point. Check the second part.

      Message::send(msg::kERROR,__FUNCTION__,Form("Found an odd event header word: %x",word));

      status = false;    
    }else{

      _event_header_words[_event_header_count] = word;

      _event_header_count++;


      if(_event_header_count == kFEM_HEADER_COUNT)

	status = decode_fem_header(_event_header_words);

      //Message::send(msg::kINFO,__FUNCTION__,Form("Wow"));
    }
    
    _ch_last_word_allow = false;
    last_word = word;

    return status;
  }

  bool algo_tpc_huffman::process_event_last_word(const UInt_t word,
						 UInt_t &last_word) 
  {
    Message::send(msg::kINFO,__FUNCTION__,
		  Form(" "));
    

    bool status = true;

    //
    // Make an explicit check.
    // Previous word should be the channel last word
    // By this point data should be saved and _header_info should be cleared.
    //
    
    UInt_t last_word_class = get_word_class(last_word);

    if(last_word_class != fem::kCHANNEL_LAST_WORD) {

      if(_ch_last_word_allow) {

	Message::send(msg::kWARNING,__FUNCTION__,
		      "Detected last-ch-last-word-missing issue in the previous event FYI...");

	_ch_last_word_allow = false;
	
      }else {
	Message::send(msg::kERROR,__FUNCTION__,
		      Form("Unexpected event last word (%x) with the previous word %x!",word,last_word));
	
	status = false;
      }

    }else if(_header_info.event_number!=fem::kINVALID_WORD){

      Message::send(msg::kERROR,__FUNCTION__,
		    Form("End of event (%x) ... but buffer holds data for event %d! Last word class",word,_header_info.event_number));
      Message::send(msg::kERROR,__FUNCTION__,
		    Form("last word is %x",last_word));
      Message::send(msg::kERROR,__FUNCTION__,
		    Form("with class %d",last_word_class));
      Message::send(msg::kWARNING,__FUNCTION__,
		    Form("SO WHAT!"));

      //status = false;
      status = true;

    }

    last_word = word;

    return status;
  }

  bool algo_tpc_huffman::process_fem_last_word(const UInt_t word,
					       UInt_t &last_word) 
  {
    //
    // This function should not be used as of now
    //
    Message::send(msg::kERROR,__FUNCTION__,
		  "This function should not be called...");

    last_word = word;
    return false;
  }

  bool algo_tpc_huffman::process_ch_word(const UInt_t word,
					 UInt_t &last_word) 
  {

    //
    // This function expects either of three kinds:
    // ch header ... marked
    // ch data (adc) ... unmarked
    // ch last word ... marked
    //
    bool status = true;
    UInt_t word_class      = get_word_class(word);
    UInt_t last_word_class = get_word_class(last_word);

    // Check for zero-padding that is allowed after channel last word
    if(word == 0x0){

      /*
      if(get_word_class(last_word)!=fem::kCHANNEL_LAST_WORD){

	Message::send(msg::kERROR,__FUNCTION__,
		      Form("Unexpected Zero-padding found after %x",last_word));
	status = false;
      }else if(_verbosity[msg::kINFO])
      */
      if(get_word_class(last_word)==fem::kCHANNEL_LAST_WORD) {

	if(_verbosity[msg::kINFO])
	  Message::send(msg::kINFO,__FUNCTION__,
			Form("Zero-padding found after %x",last_word));

	return status;
      }
    }

    switch(word_class){

    case fem::kCHANNEL_HEADER:

      //
      // Check if the last word was event header or channel last word
      //
      if(last_word_class == fem::kFEM_HEADER ||
	 last_word_class == fem::kCHANNEL_LAST_WORD ) {

	// New data starts here. 
	// Clear data content & assign channel number.
	_ch_data.clear_data();

	_ch_data.set_channel_number( (word & 0xfff) );

	if(_verbosity[msg::kDEBUG])

	  Message::send(msg::kDEBUG,__FUNCTION__,
			Form("New channel header: %d",_ch_data.channel_number()));

      }else{

	Message::send(msg::kERROR,__FUNCTION__,
		      Form("Unexpected channel header (%x)! Last word = %x",word,last_word));

	status = false;

      }
      
      break;

    case fem::kCHANNEL_LAST_WORD:{

      // Check channel number is same as the header
      UShort_t ch( (word & 0xfff) );
      if(ch == _ch_data.channel_number()) {

	if(_verbosity[msg::kDEBUG]) {

	  Message::send(msg::kDEBUG,__FUNCTION__,
			Form("Finished reading %zu samples for Ch %d",
			     _ch_data.size(),_ch_data.channel_number()));
	}

	//vic
	if(!_event_data)
	  _event_data = (event_tpcfifo*)(_storage->get_data<event_tpcfifo>("tpcfifo"));
	
	// Store
	if(_verbosity[msg::kDEBUG])
	  Message::send( msg::kDEBUG,
			 __FUNCTION__,
			 Form("_event_data ptr %x",_event_data) );
	
	_ch_data.set_module_id(_header_info.module_id);
	_ch_data.set_module_address(_header_info.module_address);
	_event_data->push_back(_ch_data);

	_ch_data.clear_data();

      }else{

	  Message::send(msg::kERROR,__FUNCTION__,
			Form("Ch. number disagreement! Header: %u ... Last: %u!",
			     _ch_data.channel_number(),ch) );

	  status = false;
      }

      // Attempt to store data if nwords matches with the expected number
      if(status && _nwords == _header_info.nwords){
	//_nwords++;
	_checksum += word;
	if(_ch_data.size()) {
	  _ch_data.set_module_id(_header_info.module_id);
	  _ch_data.set_module_address(_header_info.module_address);
	  _event_data->push_back(_ch_data);
	  _ch_data.clear_data();
	}
	status = store_event();
      }

      break;
    }
    case fem::kFEM_LAST_WORD:

      if(last_word_class == fem::kCHANNEL_LAST_WORD)

	status = process_fem_last_word(word,last_word);

      else status = decode_ch_word(word,last_word);

      break;

    default:
      if(_verbosity[msg::kDEBUG])
	Message::send(msg::kDEBUG,__FUNCTION__,Form("\t is an ADC!"));

      status = decode_ch_word(word,last_word);
    }

    if(status && _header_info.nwords){

      _nwords++;
      _checksum += word;

    }    

    last_word = word;

    return status;
  }
  
}

#endif
