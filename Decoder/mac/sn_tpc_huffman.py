#/usr/bin/python

# Basic python module import
import os, sys, __main__

# Expect the 1st argument to be an input file.
if len(sys.argv)<2:
    sys.stderr.write('\n   Usage: %s $INPUT_FILE\n\n' % __main__.__file__)
    sys.exit(1)
elif not os.path.isfile(sys.argv[1]):
    sys.stderr.write('\n   Could not recognize an input file: \"%s\"\n\n' % sys.argv[1])
    sys.exit(1)

# Temporary place input argument array in elsewhere
# otherwise ROOT recognize them as input argument.
argv=sys.argv
sys.argv=[]

# Import ROOT
import ROOT

# Load decoder library
#ROOT.gSystem.Load("libDecoder")

# Load decoder class/namespace
# from ROOT import *
#from ROOT import larlight as fmwk
from larlite import larlite as ll

#
# Four lines to just get the output filename from input file name.
# Given the input file XXX.yyy, output file would be XXX.root.
# The output file is created in $PWD defined in out_dir variable below.
#
out_dir = "./"
fpath   = argv[1]
fname   = fpath.split('/')[len(fpath.split('/'))-1]
outname = fname[0:fname.rfind('.')] + ".root"

# Decoder algorithm instance ... currently xmit, slow, or trigger

#algo=ll.algo_pmt_xmit()
#algo=ll.algo_slow_readout_decoder()
#algo=ll.algo_tpc_xmit()
#algo=ll.algo_trig_decoder()
#algo=ll.algo_tpc_huffman()
#algo=ll.algo_sn_tpc_huffman()

algo=ll.algo_sn_tpc_huffman()

algo.set_skip(ll.kNextPacket)

# Set algorithm's back-trace mode with positive int argument (=# words to be backtraced)
algo.set_backtrace_mode(50)

# Create the decoder instance
decoder=ll.decoder_manager()

# Attach algorithm to the decoder.
decoder.set_decoder(algo);

# Set input file format ... ASCII or BINARY
#decoder.set_format(ll.FORMAT.ASCII)
decoder.set_format(ll.FORMAT.BINARY)

# Set whether or not to ready by block 
#decoder.set_read_by_block(True)

# Set read-block size 

#How many words?
#decoder.set_read_block_size(734)
#decoder.set_read_block_size(49224) # 49224

# Set input file path
decoder.add_input_filename(fpath)

# Set output file path
decoder.set_output_filename(outname)

# Set message level
#    - DEBUG   ... for debugging purpose. Very verbose.
#    - INFO    ... for validation purpose. Spits out summary info of channel/event
#    - NORMAL  ... default mode. Only spits out run-relevant information
#    - WARNING ... suppress NORMAL information
#    - ERROR   ... suppress WARNING information
# For the given info level, all lower level information will be suppressed.
decoder.set_verbosity(ll.msg.kINFO)
#decoder.set_verbosity(ll.msg.kINFO)
#decoder.set_verbosity(ll.msg.kNORMAL)

# Set debug mode True if you wish to continue in the decoding event
# loop with exception handling. This avoids saving an event with
# missing event words or conflicting encoding algorithm, and continue
# in the decoding event loop. When turned off, the program exits as
# soon as it encounters any issue.

decoder.debug_mode(True)
#decoder.debug_mode(True)

# Finish configuration. Now run it.
status = decoder.run()

# Reset properly before exits.
decoder.reset()

print status
print
    

