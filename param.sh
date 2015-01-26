#declare -a bm=("canneal" "swaptions" "bodytrack" "blackscholes" "fluidanimate"  "streamcluster")
#declare -a bm=( "backprop" "kmeans" "nn" "nw" "particle" "srad")
#declare -a bm=("black" "fluid" "can" "back" "kmeans" "particle" "srad" "plast")
declare -a bm=("black" "back" "kmeans" "particle" "plast")
#declare -a bm=("can")
declare -a thread=( '0' '1')
#declare -a prefetch=("no_prefetch" "next_line_prefetcher" "tagged_prefetcher" "global_hist_prefetcher" "rpt" "markov_prefetcher");
declare -a prefetch=("no_prefetch" "tagged_prefetcher" "global_hist_prefetcher" "rpt" "markov_prefetcher");
#declare -a conf=("in_4")
declare -a L2_lat=('1')
declare -a mem_lat=('1')

mkdir -p /temp_dd/alf_2/snataraj/parma/prefetch/pref_$1_$2_$3

for i in ${prefetch[@]}
do
for k in ${L2_lat[@]}
do
for m in ${mem_lat[@]}
do
for b in ${bm[@]}
do
for j in ${thread[@]}
#for j in {2..32}
do                   
echo "exec.py -p $i -i /temp_dd/alf_2/snataraj/parma/pref_trace/memtrace_$b.$j -l $k -m $m -a $1 -b $2 -c $3 -o /temp_dd/alf_2/snataraj/parma/prefetch/pref_$1_$2_$3/$i"_"$b"_"$j"_"$k"_"$m"
done
done
done
done
done
