/**
 * \file event_builder.h
 *
 * \ingroup EventBuilder
 * 
 * \brief Event builder class def 
 *
 * @author Kazu - Nevis 2014
 */

/** \addtogroup EventBuilder

    @{*/

#ifndef EVENT_BUILDER_H
#define EVENT_BUILDER_H

#include <algorithm>
#include <iterator>
#include <vector>
#include <time.h>
#include <TStopwatch.h>

#include "Base/Base-TypeDef.h"
#include "evb_algo_pmt.h"
#include "evb_algo_tpc.h"
#include "evb_algo_trigger.h"

namespace larlite {

  /**
    \class event_builder
    
   */
  class event_builder : public larlite_base {

  public:

    /// Default constructor
    event_builder();

    /// Default destructor
    virtual ~event_builder(){};

    /// Method to set the output file name
    void set_output_filename(std::string name){ _out_storage->set_out_filename(name);}

    /// Method to set the analysis output file (users' plots/data product)
    void set_ana_output_filename(std::string name) { _ana_filename = name; }
    
    /// Method to add an algorithm/input file
    bool add_input(data::DATA_TYPE type, std::string fname);

    /// Method to execute the event building algorithms
    bool run();

    /// Method to set the reference (main) readout stream. Default=trigger.
    void set_master_stream(data::DATA_TYPE type);
    
    /// Method to run in debug mode
    void debug_mode(bool debug_mode){ _debug_mode = debug_mode;}

  protected:

    bool supported_stream(data::DATA_TYPE type);

    std::vector<int> _algo_index;

    std::vector<larlite::evb_algo_base*> _evb_algo_v;

    data::DATA_TYPE _ref_data;

    storage_manager* _out_storage;

    std::string _ana_filename;

    TFile* _fout;

    bool _debug_mode;

  };
}
#endif
/** @} */ // end of doxygen group 
