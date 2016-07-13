//
// cint script to generate libraries
// Declaire namespace & classes you defined
// #pragma statement: order matters! Google it ;)
//

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class larlite::decode_algo_exception+;

#pragma link C++ class larlite::algo_base+;
#pragma link C++ class larlite::algo_slow_readout_decoder+;
#pragma link C++ class larlite::algo_xmit_decoder+;
#pragma link C++ class larlite::algo_tpc_xmit+;
#pragma link C++ class larlite::algo_pmt_xmit+;
#pragma link C++ class larlite::algo_tpc_huffman+;
#pragma link C++ enum larlite::search+;
#pragma link C++ class larlite::algo_sn_tpc_huffman+;


//#pragma link C++ class larlite::algo_debug_sn_tpc_huffman+;
//#pragma link C++ class larlite::algo_sn_tpc_huffincompressible+;

#pragma link C++ class larlite::algo_trig_decoder+;
#pragma link C++ class larlite::algo_fem_decoder_base+;



#endif
