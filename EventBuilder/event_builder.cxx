#ifndef EVENT_BUILDER_CXX
#define EVENT_BUILDER_CXX

#include <event_builder.h>

namespace larlite {
  
  event_builder::event_builder() : larlite_base(), 
				   _algo_index(data::DATA_TYPE_MAX,-1),
				   _evb_algo_v()

  { 
    _name = "event_builder"; 
    _ana_filename = "";
    _ref_data = data::Trigger;
    _out_storage = 0;
    _debug_mode = false;
    // Create output data stream
    _out_storage = new storage_manager();
    _out_storage->set_io_mode(_out_storage->kWRITE);    
  }

  bool event_builder::supported_stream(data::DATA_TYPE type)
  {
    return ( type==data::PMTFIFO || type==data::TPCFIFO || type==data::Trigger );    
  }

  bool event_builder::add_input(data::DATA_TYPE type, std::string fname)
  {
    if( !supported_stream(type) ) {

      print(msg::kERROR,Form("Un-supported type: %d",type));

      return false;

    }

    if(_algo_index[type] < 0) {

      evb_algo_base* algo = 0;
      if      ( type == data::PMTFIFO ) algo = new evb_algo_pmt;
      else if ( type == data::TPCFIFO ) algo = new evb_algo_tpc;
      else if ( type == data::Trigger ) algo = new evb_algo_trigger;

      _algo_index[type]=_evb_algo_v.size();
      _evb_algo_v.push_back(algo);
      
      print(msg::kNORMAL,__FUNCTION__,Form("Created a new algorithm: %s",_evb_algo_v[_algo_index[type]]->class_name().c_str()));
      
    }

    _evb_algo_v[_algo_index[type]]->add_input_file(fname);

    return true;
  }

  void event_builder::set_master_stream(data::DATA_TYPE type)
  {

    if( !supported_stream(type) ) {

      print(msg::kERROR,Form("Un-supported type: %d",type));

      return;
    }
    
    _ref_data = type;
  }

  bool event_builder::run()
  {
    if(_algo_index[_ref_data] < 0) {

      print(msg::kERROR,__FUNCTION__,
	    Form("No input found for the reference data stream (\"%s\")! Aborting... ", 
		 data::DATA_TREE_NAME[_ref_data].c_str())
	    );

      return false;

    }

    _out_storage->set_verbosity(get_verbosity());

    for(auto const algo : _evb_algo_v)

      algo->set_verbosity(get_verbosity());

    bool status=true;

    if(!_out_storage->open()) return false;

    if(!_ana_filename.empty())
      
      _fout = TFile::Open(_ana_filename.c_str(),"RECREATE");

    for(auto &algo : _evb_algo_v)

      algo->SetPlotFile(_fout);

    // Initialize all algorithms
    for(auto &algo : _evb_algo_v)

      status = status && algo->initialize();

    if(!status) {

      for(auto const algo : _evb_algo_v) 

	algo->finalize();
      
      return false;
    }
	
    // First process master stream, then process other data stream.
    // Save if all stream gets processed correctly
    bool   eof      = false;
    UInt_t ctr      = 0;
    UInt_t event_id = 0;
    TStopwatch clock;
    clock.Start();
    while( (_debug_mode || status) && !eof) {
      
      try{
	status = _evb_algo_v[_algo_index[_ref_data]]->process(_out_storage);
      }catch(evb_exception &e) {
	  std::cerr<<e.what()<<std::endl;
	status=false;
      }
      eof = (eof || _evb_algo_v.at(_algo_index.at(_ref_data))->eof());

      event_id = _out_storage->get_data(_ref_data)->event_id();

      for(auto const algo : _evb_algo_v) {

	const data::DATA_TYPE type = algo->data_type();

	// Skip if main stream
	if(type == _ref_data) continue;

	// Prepare
	_out_storage->get_data(type)->set_event_id(event_id);

	if(type == data::PMTFIFO)
	  
	  ((event_pmtfifo*)(_out_storage->get_data(type)))->set_event_number(data::INVALID_UINT);

	else if(type == data::TPCFIFO)

	  ((event_tpcfifo*)(_out_storage->get_data(type)))->set_event_number(data::INVALID_UINT);

	// Process
	try{

	  if(_debug_mode)
	    status = algo->process(_out_storage,event_id) && status;
	  else
	    status = status && algo->process(_out_storage,event_id);
	    
	}catch(evb_exception &e){
	  std::cerr<<e.what()<<std::endl;
	  status=false;
	}
	
	// Check
	try{
	  if(_debug_mode)
	    status = algo->check_event_quality() && status;
	  else
	    status = status && algo->check_event_quality();
	}catch(evb_exception &e){
	  std::cerr<<e.what()<<std::endl;
	  status=false;
	}

	eof = (eof || algo->eof());
      }

      if(status) {

	_out_storage->next_event();

	if(_verbosity[msg::kINFO]) {
	  
	  print(msg::kINFO,__FUNCTION__,
		Form("done for Event ID = %d (master=\"%s\")", event_id, data::DATA_TREE_NAME[_ref_data].c_str()));
	  
	}

	ctr++;
	
	if(ctr%100==0) 
	  print(msg::kNORMAL,__FUNCTION__,
		Form("  Done %d events @ %g [s]",ctr,clock.RealTime()));

      }else{

	print(msg::kERROR,__FUNCTION__,
	      Form("skip storing count %d as status return from algorithm is false...",ctr));

	for(auto const &algo : _evb_algo_v)
	  
	  _out_storage->get_data(algo->data_type())->clear_data();

      }
      
    }

    for(auto const algo : _evb_algo_v)
      
      _out_storage->get_data(algo->data_type())->clear_data();

    print(msg::kNORMAL,__FUNCTION__,
	  Form(" Finished processing %d events (last event id = %d)", ctr, event_id));
    
    if(status) {

      // Initialize all algorithms
      for(auto const algo : _evb_algo_v)
	
	status = status && algo->finalize();
      
    }

    if(_fout) _fout->Close();

    _out_storage->close();

    return status;    
  }

}

#endif
