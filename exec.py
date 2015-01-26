
#!/usr/bin/python
import os;
import sys;
import optparse;
import threading;
import subprocess

opt_parser = optparse.OptionParser()
opt_parser.add_option('-i',
                  dest="arg_input",
                  action="store",
                  default="all"
                  )
opt_parser.add_option('-m',
                  dest="arg_memlat",
                  action="store",
                  default="1"
                  )
opt_parser.add_option('-l',
                  dest="arg_l2lat",
                  action="store",
                  default="count"
                  )
opt_parser.add_option('-p',
                  dest="arg_prefetcher",
                  action="store",
                  default="count"
                  )
opt_parser.add_option('-o',
                  dest="arg_output",
                  action="store",
                  default="count"
                  )
opt_parser.add_option('-a',
                  dest="arg_size",
                  action="store",
                  default="count"
                  )
opt_parser.add_option('-b',
                  dest="arg_dist",
                  action="store",
                  default="count"
                  )
opt_parser.add_option('-c',
                  dest="arg_degree",
                  action="store",
                  default="count"
                  )
options,rem = opt_parser.parse_args();
inp = options.arg_input;
l2_lat= options.arg_l2lat;
mem_lat=options.arg_memlat;
pref=options.arg_prefetcher;
outp=options.arg_output;
size=options.arg_size;
dist=options.arg_dist;
deg=options.arg_degree;
pref_path="/udd/snataraj/pin-2.12-53271-gcc.4.4.7-ia32_intel64-linux/source/tools/test_prog/prefetch/"+pref+"_"+size+"_"+dist+"_"+deg+" "+inp+ " " + l2_lat + " " + mem_lat +" > "+ outp; 
print(pref_path);
os.system(pref_path);
