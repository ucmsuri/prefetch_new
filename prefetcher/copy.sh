./run.sh no_prefetch
./run.sh next_line_prefetcher
./run.sh tagged_prefetcher
./run.sh global_hist_prefetcher
./run.sh rpt
./run.sh markov_prefetcher
cp no_prefetch next_line_prefetcher tagged_prefetcher global_hist_prefetcher rpt markov_prefetcher ../../test_prog/prefetch/
