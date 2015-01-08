./run.sh no_prefetch $1 $2 $3
#./run.sh next_line_prefetcher
./run.sh tagged_prefetcher $1 $2 $3
./run.sh global_hist_prefetcher $1 $2 $3
./run.sh rpt $1 $2 $3
./run.sh markov_prefetcher $1 $2 $3
mv no_prefetch_$1_$2_$3 tagged_prefetcher_$1_$2_$3 global_hist_prefetcher_$1_$2_$3 rpt_$1_$2_$3 markov_prefetcher_$1_$2_$3 ../../test_prog/prefetch/
