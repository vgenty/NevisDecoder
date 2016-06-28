#ifndef ALGO_SN_TPC_HUFFMAN_HH
#define ALGO_SN_TPC_HUFFMAN_HH

#include "algo_tpc_huffman.hh"

namespace larlight {

  class algo_sn_tpc_huffman : public algo_tpc_huffman {

  public:

    algo_sn_tpc_huffman();

    virtual ~algo_sn_tpc_huffman(){}

    /// Override of algo_base::get_word_class method
    virtual FEM::FEM_WORD get_word_class(const UInt_t word) const;
    
    /**
       Override the fem header decoding as SN stream does not care trigger frame.
       The implementation is basically identical to the base class EXCEPT roll
       over is not treated. 3 bits trigger frame information is not useful for
       SN.
    */
    virtual bool decode_fem_header(const UInt_t *event_header);

    virtual bool process_word(const UInt_t word);
    
  protected:  

    // virtual bool process_fem_header
    // (const UInt_t word,UInt_t &last_word) override;

    virtual bool process_event_last_word
    (const UInt_t word,UInt_t &last_word);

    virtual bool process_ch_word
    (const UInt_t word,UInt_t &last_word);

    // SN compression has changed (?)
    virtual inline bool is_compressed(const UInt_t word)
    { return ( (word>>15) == 0x1 ); };

    //Decoding huffman word also changed (?)
    virtual bool decode_ch_word(const UInt_t word, UInt_t &last_word);

    //Let inherit and remove checksum :-)
    virtual bool check_event_quality();
    
    void store_ch_data();

    void set_pre_samples(UInt_t pre_samples) { _pre_samples = pre_samples; }
    
  protected:

    /// These are the same as the ones defined in fifo.hh, and used for
    /// the second and later packets which have no channel header in front
    UShort_t  _channel_number_holder;
    UInt_t    _readout_frame_number_holder;

    UInt_t _pre_samples;
  };

}

#endif
