# Code For DeRAIS

These are the code need for DeRAIS. You should integrate these code into disksim. 

In order to make it clear, I only keeps those code I modified. Disksim and SSD sim can be downloaded from their source.

Detailed explaination is already present in the code, here I provide some basic explaination about how to use these codes.

- `disksim_iodriver.c` is where I modifed to make my own warm up
- `disksim_iotrace.c` is a very import place, trace get read here, so I modified this to make deduplication and distribution for DeRAIS
- `disksim_redun.c`, since disksim works weird, I don't konw how to exactly make a full stripe write without increasing block count, So I wrote my own, I modifed one function to make it compatiable with my code and assembly requests just as I wish. 
- `ssd_dedupRAID.c`, `ssd_dedupRAID.h` are two main parts for DeRAIS, all functions are implemented here


## How To Understand

start from `disksim_iotrace.c`, which contains routine for DeRAIS, and you can jump to `ssd_dedupRAID.c` to know how the codes are implemented.