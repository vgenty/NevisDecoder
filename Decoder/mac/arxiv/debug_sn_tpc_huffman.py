# Basic python module import
import os, sys, __main__, re
import ROOT


ROOT.gSystem.Load("libDecoder")
from ROOT import *
from ROOT import larlight as fmwk


# Set input root file
files   = []
os.system('rm outfile.dat')
outfile = open('outfile.dat', 'w')

for x in xrange(len(sys.argv)-1):
    files.append(sys.argv[x+1])

problems = { '0' : "no problems",
             '1' : "missing end of channel word",
             '2' : "missing words in last channel end of event",
             '3' : "n word in event not match header",
             '4' : "unexpected event last word",
             '5' : "huffman coded after non adc"}



for file_name in files:
    search = re.search("min_([0-9]+)_max_([0-9]+)",file_name)
    tstart = search.group(1)
    tend   = search.group(2)

    
    out_dir = "./"
    fpath   = file_name;
    fname   = fpath.split('/')[len(fpath.split('/'))-1]
    outname = fname[0:fname.rfind('.')] + ".root"

    
    algo=fmwk.algo_debug_sn_tpc_huffman()
    algo.SetChCheck(63);

    algo.set_verbosity(fmwk.MSG.NORMAL)

    algo.set_backtrace_mode(50)
    decoder=fmwk.decoder_manager()
    decoder.set_decoder(algo);
    decoder.set_format(fmwk.FORMAT.BINARY)
    decoder.set_read_by_block(True)
    decoder.set_read_block_size(50000)
    decoder.add_input_filename(fpath)
    decoder.set_output_filename(outname)
    # decoder.set_verbosity(fmwk.MSG.DEBUG)
    decoder.debug_mode(True)
    status = decoder.run()


    outfile.write('%s,%s,' % (tstart, tend))
    probs = algo.problems()

    
    #for key in problems :
    for key in xrange(6) :
        if ( probs.count( int(key) ) == True ):
            outfile.write("%s," % key)
        else  :
            outfile.write("0," )
            #outfile.write("%s,\t" % problems[key])
            
    outfile.write("\n")
    

    
    decoder.reset()

outfile.close()

print status
print    

