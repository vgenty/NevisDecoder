/**
 * \file evb_algo_trigger.h
 *
 * \ingroup EventBuilder
 * 
 * \brief Event builder algorithm class def for TRIGGER data stream
 *
 * @author Kazu - Nevis 2014
 */

/** \addtogroup EventBuilder

    @{*/

#ifndef EVB_ALGO_TRIGGER_H
#define EVB_ALGO_TRIGGER_H

#include "evb_algo_base.h"

namespace larlite {

  class evb_algo_trigger : public evb_algo_base {

  public:

    evb_algo_trigger() : evb_algo_base(data::Trigger) { _name = "evb_algo_trigger";}

    virtual ~evb_algo_trigger(){}

    virtual bool initialize();

    virtual bool finalize();

  protected:

    virtual bool build(storage_manager *out_storage, UInt_t id);

  };
}
#endif
/** @} */ // end of doxygen group 
