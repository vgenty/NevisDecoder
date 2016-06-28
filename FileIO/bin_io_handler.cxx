#ifndef BIN_IO_HANDLER_CXX
#define BIN_IO_HANDLER_CXX

#include "bin_io_handler.h"

namespace larlite {

  bin_io_handler::bin_io_handler(FORMAT::INPUT_FILE type)
    : larlite_base(),  _format(type)
  {
    _name="bin_io_handler";
    _status=INIT;
    reset();
  }
  
  
  void bin_io_handler::reset(){
    
    //
    // Only function that can assign INIT status other 
    // than the constructor.
    //
    switch(_status){
    case INIT:
    case CLOSED:
      break;
    case OPENED:
    case OPERATING:
      close();
      break;
    }  
    _filename="";
    _nwords_in_file=0;
    _mode=READ;
    _status=INIT;
    _format=FORMAT::UNDEFINED;
    _eof=false;
    _single_word[0]=0x0;
  }
  
  bool bin_io_handler::open(){
    //
    // Only function that can assign OPENED status
    //
    if(_filename.size()==0) {
      Message::send(msg::kERROR,__FUNCTION__,
		    "File name is empty. Provide a file path+name.");
    }else if(_status==OPENED || _status==OPERATING) {
      sprintf(_buf,
	      "File is open: %s ... you must close it first.",
	      _filename.c_str());
      Message::send(msg::kERROR,__FUNCTION__,_buf);
    }else{
      switch(_format){
      case FORMAT::UNDEFINED:
	Message::send(msg::kERROR, __FUNCTION__,
		      "File format unspecified. Cannot open a file.");
	break;
      case FORMAT::BINARY:
	if(_mode==READ) _handler=fopen(_filename.c_str(),"rb");
	else            _handler=fopen(_filename.c_str(),"wb");
	break;
      case FORMAT::ASCII:
	if(_mode==READ) _handler=fopen(_filename.c_str(),"r");
	else            _handler=fopen(_filename.c_str(),"w");
	break;
      }
    }
    
    if(!(_handler)){
      sprintf(_buf,"Failed to open a file: %s",_filename.c_str());
      Message::send(msg::kERROR,__FUNCTION__,_buf);
      return false;
    }
    
    sprintf(_buf,"Opened a file: %s",_filename.c_str());
    Message::send(msg::kNORMAL,__PRETTY_FUNCTION__,_buf);
    
    _eof = false;
    _status=OPENED;
    return true;
  }

  bool bin_io_handler::is_open() const{
    switch(_status){
    case OPENED:
    case OPERATING:
      return true;
    case INIT:
    case CLOSED:
      break;
    }
    return false;
  }
  
  void bin_io_handler::close(){
    //
    // Only function that can assign CLOSED status.
    //
    switch(_status){
    case INIT:
      Message::send(msg::kERROR,__FUNCTION__,"File not yet opened!");
      break;
    case CLOSED:
      Message::send(msg::kERROR,__FUNCTION__,"File already closed!!");
      break;
    case OPENED:
      sprintf(_buf,"Closing: %s ... no I/O operation done.",_filename.c_str());
      Message::send(msg::kWARNING,__FUNCTION__,_buf);
      fclose(_handler);
      break;
    case OPERATING:
      sprintf(_buf,"Closing: %s ... %d words processed",
	      _filename.c_str(),
	      _nwords_in_file);
      Message::send(msg::kNORMAL,__FUNCTION__,_buf);
      fclose(_handler);
      break;
    }
    _handler=0;
    _status=CLOSED;
  }
  
  bool bin_io_handler::write_word(const unsigned word) {
    
    _single_word[0]=word;
    
    return write_multi_word(_single_word,1);
  }
  
  bool bin_io_handler::write_multi_word(const unsigned* words, const size_t entries){
    
    //
    // Only function that can assign OPERATING status in WRITE mode.
    //
    if(_status!=OPENED && _status!=OPERATING) {
      Message::send(msg::kERROR,__FUNCTION__,"Invalid file I/O status. Cannot write output!");
      return false;
    }
    
    if(_mode!=WRITE){
      Message::send(msg::kERROR,__FUNCTION__,
		    "Not allowed as this is output file stream!");
      return false;
    }
    
    if(_format==FORMAT::ASCII) {
      
      _nchars=0;
      
      for(unsigned index=0; index<entries; ++index) {
	
	_nchars+=fprintf(_handler,"%x ",words[index]);
	
	_nwords_in_file++;
	
	_checksum+=words[index];
	
      }

      fprintf(_handler,"\n");
    }
    else {
      
      fwrite(&words[0],sizeof(unsigned),entries,_handler);
      
    }
    
    return true;
  }
  
  unsigned bin_io_handler::read_multi_word(size_t length){
    
    _word = fem::kEMPTY_WORD;
    if(_status!=OPENED && _status!=OPERATING) {
      Message::send(msg::kERROR,__FUNCTION__,
		    "Invalid file I/O status. Cannot read!");
      return _word;
    }
    
    if(_mode!=READ){
      Message::send(msg::kERROR,__FUNCTION__,
		    "Not allowed as this is input file stream!");    
      return _word;
    }
    
    _status=OPERATING;
    
    // Check if a buffer holds words to be read out
    if(_multi_word_index < _read_word_buffer.size()) {
      
      _word = _read_word_buffer.at(_multi_word_index);
      
      _multi_word_index++;
      
      _nwords_in_file++;
      
      return _word;
    }
    
    if(feof(_handler)){
      Message::send(msg::kDEBUG,__FUNCTION__,"Reached EOF!");
      _eof=true;
      return fem::kEMPTY_WORD;
    }
    
    size_t nwords_to_read = length;
    size_t nwords_read    = 0;
    
    if(!(nwords_to_read)) nwords_to_read = read_word();
    
    if(_format==FORMAT::ASCII){
      
      _read_word_buffer.clear();
      
      _read_word_buffer.reserve(nwords_to_read);
      
      for(nwords_read=0; nwords_read<nwords_to_read && !_eof; nwords_read++){
	
	if(fscanf(_handler,"%x%n",&_word,&_nchars)<1) {
	  Message::send(msg::kDEBUG,__FUNCTION__,"Reached EOF!");
	  _eof=true;
	}
	
	while(!_eof && _nchars<8  ) {
	  if(_verbosity_level<=msg::kWARNING){
	    sprintf(_buf,"Encountered none 32-bit word expression: %x (%d chars)",_word,_nchars);
	    Message::send(msg::kWARNING,__FUNCTION__,_buf);
	  }
	  
	  _eof=feof(_handler);
	  
	  if(fscanf(_handler,"%x%n",&_word,&_nchars)<1) {
	    _eof=true;
	    break;
	  }
	}
	
	if(!_eof) _read_word_buffer.push_back(_word);
      }
      
    }
    else{
      
      _read_word_buffer.clear();
      _read_word_buffer.resize(nwords_to_read,0);
      
      nwords_read=fread(&_read_word_buffer[0],sizeof(_word),nwords_to_read,_handler);
      
      if(nwords_read < nwords_to_read) {
	
	_eof = true;
	
	_read_word_buffer.resize(nwords_read);
	
      }
    }
    
    if(_verbosity[msg::kINFO]){
      
      sprintf(_buf,"Read-in %zu/%zu words from the file...",nwords_read,nwords_to_read);
      Message::send(msg::kINFO,__FUNCTION__,_buf);
    }
    
    _multi_word_index = 0;
    
    _word = _read_word_buffer[_multi_word_index];
    
    _multi_word_index++;
    
    return _word;
  }
  
  unsigned bin_io_handler::read_word() {
    
    if(_eof) return fem::kEMPTY_WORD;
    
    _word=fem::kEMPTY_WORD;
    _nchars=fem::kEMPTY_WORD;
    
    if(_status!=OPENED && _status!=OPERATING) {
      Message::send(msg::kERROR,__FUNCTION__,
		    "Invalid file I/O status. Cannot read!");
      return _word;
    }
    
    if(_mode!=READ){
      Message::send(msg::kERROR,__FUNCTION__,
		    "Not allowed as this is input file stream!");    
      return _word;
    }
    
    _status=OPERATING;
    
    //
    // Different handling for ASCII v.s. BIN
    //
    if(feof(_handler)){
      Message::send(msg::kDEBUG,__FUNCTION__,"Reached EOF!");
      _eof=true;
      return fem::kEMPTY_WORD;
    }
    
    // Check if fscanf returns < 1 ... then it is EOF
    if(_format==FORMAT::ASCII){
      
      if(fscanf(_handler,"%x%n",&_word,&_nchars)<1) {
	Message::send(msg::kDEBUG,__FUNCTION__,"Reached EOF!");
	_eof=true;
      }
      
      while(!_eof && _nchars<8  ) {
	if(_verbosity_level<=msg::kWARNING){
	  sprintf(_buf,"Encountered none 32-bit word expression: %x (%d chars)",_word,_nchars);
	  Message::send(msg::kWARNING,__FUNCTION__,_buf);
	}
	
	_eof=feof(_handler);
	
	if(fscanf(_handler,"%x%n",&_word,&_nchars)<1) {
	  _eof=true;
	  break;
	}
      }
    }
    else
      _eof=!(fread(&_word,sizeof(_word),1,_handler));
    
    if(_eof)
      _word=fem::kEMPTY_WORD;
    else{
      _checksum+=_word;
      _nwords_in_file+=1;
    }
    
    return _word;
  }

}
#endif


