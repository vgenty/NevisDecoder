#include "algo_sn_tpc_huffincompressible.h"

namespace larlite {

  algo_sn_tpc_huffincompressible::algo_sn_tpc_huffincompressible() : algo_tpc_huffman()
  {
    _pre_samples = 0;
    _name = "algo_sn_tpc_huffincompressible";
    _search_for_next_channel = false;
    reset();
  }  

  fem::FEM_WORD algo_sn_tpc_huffincompressible::get_word_class(const UInt_t word) const {

    if( word == 0x0 ) return fem::kUNDEFINED_WORD;
    else if( (word & 0xffffffff) == 0xffffffff ) // Unique marker, but with the huffman
      // coding, not unique anymore
      return fem::kEVENT_HEADER;
      
    else if( (word & 0xffffffff) == 0xe0000000 ) // Unique marker, but with the huffman
      // coding, not unique anymore
      return fem::kEVENT_LAST_WORD;
      
    if( (word & 0xffff) == 0xffff )              // Unique marker, but with the huffman
      // coding, not unique anymore
      return fem::kFEM_HEADER;

    else if( (word & 0xf000) == 0xf000 )         // Could be ADC word
      return fem::kFEM_HEADER;
      
    else if( !((word>>15) & 0x1) ) {

      if ( ( word & 0xf000 ) == 0x1000 )         // Channel time word
	return fem::kCHANNEL_TIME;
	
      else if( (word & 0xf000) == 0x2000 )       // Uncompressed ADC word
	return fem::kCHANNEL_WORD;
	
      else if ( ( word & 0xf000 ) == 0x3000 )    // Channel: last word of the packet
	return fem::kCHANNEL_PACKET_LAST_WORD;
	
      else if( (word & 0xf000) == 0x4000 )       // Channel first word
	return fem::kCHANNEL_HEADER;
      
      else
	return fem::kUNDEFINED_WORD;              // Undefined
    }
    else if( ((word>>15) & 0x1) ){
      return fem::kCHANNEL_WORD;                  // compressed ADC word
    }else {
      return fem::kUNDEFINED_WORD;
    }

    // fem::FEM_WORD code = algo_tpc_huffman::get_word_class(word); 
  }

  //#################################################
  bool algo_sn_tpc_huffincompressible::decode_fem_header(const UInt_t *event_header){
    //#################################################

    Message::send(msg::kINFO,"Decoding FEM header word");
    
    bool status=true;
    //
    // Get event information
    //

    // (1) check if the very first 16-bit word is as expected
    if(!( event_header[0] & 0xffff))
      Message::send(msg::kERROR,"Unexpected first word in event headers!");

    if(!status) return status;

    // Initialize data holder
    _header_info.clear_event();

    // (2) get module address ... lowest 5 of 12 bits
    _header_info.module_address = ( (event_header[0]>>16 & 0xfff)    & 0x1f);

    // (3) get module ID number ... 8:5 bit of 12 bits
    _header_info.module_id      = ( (event_header[0]>>16 & 0xfff)>>5 & 0xf);

    // (4) get number of 16-bit words to be read in this event.
    // Lower 12 bits of two 16-bit words.
    // The very last "last word for channel info" is not included in this.
    // For later checking purpose, increment by 1.
    _header_info.nwords         = ( (((event_header[1]>>16) & 0xfff) + ((event_header[1] & 0xfff)<<12)));

    // (5) get event ID
    // Lower 12 bits of two 16-bit words.
    _header_info.event_number       = ( (((event_header[2]>>16) & 0xfff) + ((event_header[2] & 0xfff)<<12)));

    // (6) get frame ID
    // Lower 12 bits of two 16-bit words.
    _header_info.event_frame_number = ( (((event_header[3]>>16) & 0xfff) + ((event_header[3] & 0xfff)<<12)));

    // (7) get checksum
    _header_info.checksum       = ( (((event_header[4]>>16) & 0xfff) + ((event_header[4] & 0xfff)<<12)));

    // Set the trigger frame to 0 to avoid confusion (any smart guess will only hurt)
    _header_info.fem_trig_frame_number = 0;

    _header_info.fem_trig_sample_number = 0;

    // Report if verbosity is set.
    // if(_verbosity[msg::kINFO])
    //   {
    std::string msg;
    for(size_t i=0; i<kFEM_HEADER_COUNT; ++i)
      msg += Form("%x ", event_header[i]);
    Message::send(msg::kINFO, __FUNCTION__, Form("Decoded Header: %s",msg.c_str()));
    Message::send(msg::kINFO, __FUNCTION__, Form("Module %d (ID=%d)", _header_info.module_address, _header_info.module_id));
    Message::send(msg::kINFO, __FUNCTION__, Form("Event ID %d",_header_info.event_number));
    Message::send(msg::kINFO, __FUNCTION__, Form("Frame ID %d",_header_info.event_frame_number));
    Message::send(msg::kINFO, __FUNCTION__, Form("Number of Words = %d",_header_info.nwords));
    Message::send(msg::kINFO, __FUNCTION__, Form("Checksum = %x", _header_info.checksum));

    //  }

    _checksum=0;

    _nwords=0;

    return status;
  }

  bool algo_sn_tpc_huffincompressible::process_word(const UInt_t word) {

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

    if ( word == 0x0 ) {
      Message::send( msg::kWARNING, __FUNCTION__, Form("Unexpected 32-bit zero-padding, last word 0x%x.", _last_word) );
      return true;
    }

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

    //decoder is fucked up, can we make it to the next channel?
    if ( _search_for_next_channel ) {

      if ( !( word_class == fem::kCHANNEL_HEADER ) ) {

	_last_word = word;
	return status;
	
      }

    }
    
    //
    // In case of huffman coded SN data stream, uniquely marked
    // words are only:
    // (*) channel header
    // (*) channel time word
    // (*) channel last word of packet
    // (*) channel ADC word (non-Huffman compression)
    //

    //current word class
    switch(word_class) {
      
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
      
      if(status){ 

	if(!(_header_info.nwords))

	  status = process_fem_header(word,_last_word);

	else {

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
      
      if ( last_word_class == fem::kCHANNEL_PACKET_LAST_WORD ) {

        // This is the normal end of event, with some packets in the last channel
	
        status = process_event_last_word(word,_last_word);

      }

      else if ( last_word_class == fem::kCHANNEL_WORD ) {
	Message::send( msg::kWARNING,__FUNCTION__,
		       Form("Possible frame rollover detected") );
	
	Message::send( msg::kWARNING,__FUNCTION__,
		       Form("word = event last word, last word = channel word. nwords = %d, header info nwords = %d" , _nwords, _header_info.nwords) );
	
	status = process_event_last_word(word,_last_word);
	
      }
      else if ( ( last_word_class == fem::kCHANNEL_HEADER ) || ( _last_word == 0x0 ) ) {

        // If the last channel contains no packet after zero-suppression,
        // we have to store this channel when the end of FEM or the end of event is met.
        // In this decoder, an "event" contains a single FEM;
        // if we have multiple FEMs in an event, we will have multiple events
        // in the frame of the decoder.
        // Therefore we only need to deal with the end of event.
        // There could be two possibilities for the previous word in this case
        // 1) Channel header
        // 2) a 16-bit zero-padding
	
        store_ch_data();
	status = process_event_last_word(word,_last_word);
	
      } else { // on __event last word__, last word not __channel word__ or __channel header__ or __0x0__ or __channel packet last word__

	Message::send( msg::kWARNING,__FUNCTION__,
		       Form("EVENT end unexpected... word = event last word, nwords = %d, header info nwords = %d" , _nwords, _header_info.nwords) );
	// Store
	
	// Attempt to store data if nwords matches with the expected number
	if(status && _nwords == _header_info.nwords){
	  Message::send( msg::kINFO,__FUNCTION__, Form("STATUS ok... number of words matched to header. word = event last word, nwords = %d, header info nwords = %d, we have _nwords++, checksum, and store_event()" , _nwords, _header_info.nwords) );

	  _nwords++;

	  // is line below correct?
	  _checksum += word;

	  _last_word = word;
	  status  = store_event();
	}
	
      }

      break;
      
    
    default:

      if(_verbosity[msg::kDEBUG])
	Message::send(msg::kDEBUG, __FUNCTION__, Form("See: 0x%x",word));

      UInt_t first_word  = (word & 0xffff);
      UInt_t second_word = (word >> 16);

      status = process_ch_word(first_word,_last_word);

      if(status) status = process_ch_word(second_word,_last_word);

    }

    if(!status){

      if(_bt_mode)
	backtrace();


      //start vic
      //clear_event();
      //end vic
      
      if(_debug_mode){

	//start vic
	//Message::send(msg::kWARNING,__FUNCTION__,"kDEBUG MODE => Continue to next event...");
	_search_for_next_event   = false;//=true;
	//end vic
	
	Message::send(msg::kWARNING,__FUNCTION__,"kDEBUG MODE => Continue to next channel...");
	_search_for_next_channel = true;

      }

    }

    return status;
  }

  bool algo_sn_tpc_huffincompressible::process_event_last_word(const UInt_t word,
							       UInt_t &last_word)
  {
    bool status = true;

    //
    // Make an explicit check.
    // Previous word should be the channel last word of packet 
    //
    // vic: sometimes it's not clear... for frame crossing as example
    
    UInt_t last_word_class = get_word_class(last_word);

    if ( last_word_class == fem::kCHANNEL_WORD ) {

      Message::send(msg::kINFO,__FUNCTION__,
                    Form("Frame crossing detected : event last word (%x) with the previous word %x!",word,last_word));
      
      status = true;
      
    }
    
    // Yun-Tse 2015/1/20: Add the possibility to have a zero-padding word before
    else if ( ( last_word_class != fem::kCHANNEL_PACKET_LAST_WORD ) && ( last_word != 0x0 ) ) {

      Message::send(msg::kERROR,__FUNCTION__,
                    Form("Unexpected event last word (%x) with the previous word %x!",word,last_word));
      
      status = false;
      
    }
    
    last_word = word;
    status = store_event();

    return status;
  }

  bool algo_sn_tpc_huffincompressible::process_ch_word(const UInt_t word,
						       UInt_t &last_word) 
  {

    //
    // This function expects either of four kinds:
    // ch header ... marked
    // ch packet time
    // ch data (adc) ... unmarked, either Huffman compressed or not
    // ch packet last word ... marked
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
      
      if ( get_word_class(last_word) == fem::kCHANNEL_PACKET_LAST_WORD ) {

	Message::send(msg::kINFO,__FUNCTION__,
		      Form("Zero-padding found after %x",last_word));

	return status;
      }

    }

    switch(word_class){

    case fem::kCHANNEL_HEADER: //0x4000
      
      if(_verbosity[msg::kDEBUG])
	Message::send(msg::kDEBUG,__FUNCTION__,
		      Form("\t is fem::kCHANNEL_HEADER") );

      _search_for_next_channel = false;
      
      // If this channel is NOT the first channel in a FEM
      if ( last_word_class == fem::kCHANNEL_PACKET_LAST_WORD ) {
	
	if(_verbosity[msg::kDEBUG])
	  Message::send(msg::kDEBUG,__FUNCTION__,
			Form("\t last was fem::kCHANNEL_PACKET_LAST_WORD") );
	
        _channel_number_holder = (word & 0x3f);
        _readout_frame_number_holder = ( ( word >> 6) & 0x3f );

        if(_verbosity[msg::kDEBUG])
          Message::send( msg::kDEBUG,__FUNCTION__, 
                         Form("New channel number: %d, New frame number: %d", _channel_number_holder, _readout_frame_number_holder ) ); 
      }       
      // Yun-Tse 2015/1/20: Add the option "last_word_class == fem::kCHANNEL_HEADER"
      // for the case that the last channel has no data after 0-suppression
      else if ( last_word_class == fem::kCHANNEL_HEADER ) { //two headers back to back

	if(_verbosity[msg::kDEBUG])
	  Message::send(msg::kDEBUG,__FUNCTION__,
			Form("\t last was fem::kCHANNEL_HEADER") );
	
        // Store and clear
        store_ch_data();

        // Set the new channel info
        _channel_number_holder = (word & 0x3f);
        _readout_frame_number_holder = ( ( word >> 6) & 0x3f );

        if(_verbosity[msg::kDEBUG])
          Message::send( msg::kDEBUG,__FUNCTION__, 
                         Form("New channel number: %d, New frame number: %d", _channel_number_holder, _readout_frame_number_holder ) ); 

      }
      // Check if the last word was an FEM header, i.e. this is the first channel
      // in a FEM
      else if ( last_word_class == fem::kFEM_HEADER ) {

	if(_verbosity[msg::kDEBUG])
	  Message::send(msg::kDEBUG,__FUNCTION__,
			Form("\t last was fem::kFEM_HEADER") );
	
        if(_verbosity[msg::kDEBUG])
          Message::send(msg::kINFO,__FUNCTION__, "word = channel header, last word = FEM header");

        // New data starts here.
        // Clear data content & assign channel number.
        _ch_data.clear_data();

        _channel_number_holder = (word & 0x3f);
        _readout_frame_number_holder = ( ( word >> 6) & 0x3f );

        if(_verbosity[msg::kINFO])
          Message::send(msg::kINFO,__FUNCTION__,
                        Form("New channel header: %d, New frame number: %d", _channel_number_holder, _readout_frame_number_holder ) );

      }

      //vic: current word is channel header but packet didn't end cleanly (i.e. with end of channel packet)
      else if ( last_word_class == fem::kCHANNEL_WORD ) { 

	if(_verbosity[msg::kDEBUG])
	  Message::send(msg::kDEBUG,__FUNCTION__,
			Form("\t last was fem::kCHANNEL_WORD") );
	
	// Store and clear
	if(_verbosity[msg::kWARNING])
	  Message::send(msg::kWARNING,__FUNCTION__,
			Form("Storing channel data") );
	
	store_ch_data();
	
        // Set the new channel info
        _channel_number_holder = (word & 0x3f);
        _readout_frame_number_holder = ( ( word >> 6) & 0x3f );

	if(_verbosity[msg::kWARNING])
          Message::send( msg::kWARNING,__FUNCTION__, 
                         Form("Frame rollover? New channel number: %d, New frame number: %d", _channel_number_holder, _readout_frame_number_holder ) );
	
	
      }
      else {
	Message::send(msg::kERROR,__FUNCTION__,
		      Form("Unexpected channel header (%x)! Last word = %x",word,last_word));

	status = false;

      }

      break;
    
    case fem::kCHANNEL_TIME: {
      if(_verbosity[msg::kDEBUG])
	Message::send(msg::kDEBUG,__FUNCTION__,
		      Form("\t is fem::kCHANNEL_TIME") );
      
      if ( ( last_word_class != fem::kCHANNEL_HEADER ) && ( last_word_class != fem::kCHANNEL_PACKET_LAST_WORD ) ) {
	if(_verbosity[msg::kDEBUG])
	  Message::send(msg::kDEBUG,__FUNCTION__,
			Form("\t last was not fem::kCHANNEL_HEADER or fem::kCHANNEL_PACKET_LAST_WORD") );
	
	

        status = false;
        Message::send(msg::kERROR,__FUNCTION__,
		      Form("Unexpected channel time word (%x) with the previous word %x (word type %d)!", word, last_word, last_word_class ) );
      } else {
	if(_verbosity[msg::kDEBUG])
	  Message::send(msg::kDEBUG,__FUNCTION__,
			Form("\t setting readout sample number 0x%x",word & 0xfff));
	
        _ch_data.set_readout_sample_number( (word & 0xfff) );
	
      }
      break;
    }

    case fem::kCHANNEL_PACKET_LAST_WORD: { //0x3000
      if(_verbosity[msg::kDEBUG])
	Message::send(msg::kDEBUG,__FUNCTION__,
		      Form("\t is FEM:kCHANNEL_PACKET_LAST_WORD"));
	
      status = decode_ch_word( ( word & 0xfff ), last_word );

      // Store the channel data
      store_ch_data();

      break;
    }

    case fem::kUNDEFINED_WORD:

      Message::send( msg::kWARNING, __FUNCTION__, Form("Expected 16-bit zero-padding, last word 0x%x.", last_word) );

      break;

    default:
      if(_verbosity[msg::kDEBUG])

	Message::send(msg::kDEBUG,__FUNCTION__,
		      Form("\t is an ADC!"));
      
      status = decode_ch_word(word,last_word);

      if ( !status ) Message::send( msg::kERROR, __FUNCTION__, Form("Error in Event 0x%x, Channel 0x%x, Readout sample number 0x%x", _header_info.event_number-1, _channel_number_holder, _ch_data.readout_sample_number_RAW() ) );
    }

    if(status && _header_info.nwords){

      _nwords++;
      _checksum += word;

    }    

    last_word = word;

    return status;
  }
  
  //#########################################################
  bool algo_sn_tpc_huffincompressible::check_event_quality(){
    //#########################################################

    bool status = true;

    if(_verbosity[msg::kINFO]) Message::send( msg::kINFO,__FUNCTION__, "algo_sn_tpc_huffincompressible" );
    // Check if _checksum and _nwords agrees with that of event header.
    // Yun-Tse 2014/11/19: Perhaps this _nwords-=1; was for some old data format?
    // _nwords-=1;
    if(_nwords!=_header_info.nwords){

      Message::send(msg::kERROR,__FUNCTION__,
		    Form("Disagreement on nwords (so?): counted=%u, expected=%u",_nwords,_header_info.nwords));

      //status = false;

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
		      Form("Disagreement on checksum (so?): summed=%x, expected=%x",_checksum,_header_info.checksum));
	//status = false;
	
      }
    }

    return status;

  }

  bool algo_sn_tpc_huffincompressible::decode_ch_word(const UInt_t word, 
						      UInt_t &last_word)
  {
    //#########################################################

    bool status = true;
    // Simply append if it is not compressed
    if( !(is_compressed(word)) ) _ch_data.push_back( (word & 0xfff) );
    
    else if(!(_ch_data.size())){

      // This is a problem: if huffman coded, then we must have a previous ADC sample
      // as a reference. Raise an error.
      
      Message::send(msg::kERROR,__FUNCTION__,
		    Form("Huffman coded word %x found while the previous was non-ADC word (%x)!",
			 word,last_word));

      status = false;

    }
    else{

      // Compresed data is in last 15 bit of this word.
      
      UInt_t data = (word & 0xffff);
      
      size_t zero_count = 0;
      bool   non_zero_found = false;
      for(short index=14; index>=0 && status; --index){

	if( !((data >> index) & 0x1) )

	  zero_count += 1;

	else {

	  status = add_huffman_adc(_ch_data,zero_count);
	  
	  zero_count = 0;
	  if(!status) {
	    Message::send(msg::kERROR,__FUNCTION__,
			  Form("Error in decoding huffman data word: 0x%x",data));
	    break;
	  }
	}
      }

      if(!status)

	Message::send(msg::kERROR,__FUNCTION__,
		      Form("Encountered unexpected number of zeros (=%zu) in the compressed word %x!",
			   zero_count,word));

    }

    return status;

  }


  void algo_sn_tpc_huffincompressible::store_ch_data() {
    // Save

    if(!_event_data)

      _event_data = (event_tpcfifo*)(_storage->get_data<event_tpcfifo>("tpcfifo"));
    
    _ch_data.set_module_id( _header_info.module_id );
    _ch_data.set_module_address( _header_info.module_address );
    _ch_data.set_channel_number( _channel_number_holder );
    _ch_data.set_readout_frame_number( _readout_frame_number_holder );

    _event_data->push_back( _ch_data );
    Message::send( msg::kINFO, __FUNCTION__, Form("Ch 0x%x, stored %zu adc words", _channel_number_holder, _ch_data.size() ) );
    
    // Clear
    _ch_data.clear_data();
    return;

  }
  
}
