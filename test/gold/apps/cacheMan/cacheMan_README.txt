cacheMan argc: 2 [1:-th] 
cacheMan -th 
#### Usage

`Usage:`    cacheMan [-c|-d|-s|-f|-l|-i|-r|-t|-k|-m|-x|-v|-h] fn_list  
`Purpose:`  Show the contents of an account cache and/or fix it by removing duplicate records.
        
`Where:`  

| Short Cut | Option | Description |
| -------: | :------- | :------- |
|  | fn_list | path(s) of files to check, merge, fix or display (default=display) (required) |
| -c | --check | check for duplicates and other problems in the cache |
| -d | --data | in 'list' mode, render results as data (i.e export mode) |
| -s | --sort | sort the list of transactions and re-write (precludes other modes, other than --dedup) |
| -f | --fix | remove duplicates from the cache (if any) |
| -l | --list | list the contents of the cache (the default if no other option) |
| -i | --import | import transactions if import.txt file exists in current folder |
| -r | --remove | remove transactions if remove.txt file exists in current folder |
| -t | --truncate <num> | truncate the cache at block :n (keeps block 'n' and before, implies --fix) |
| -k | --maxBlock <num> | for testing, max block to visit |
| -m | --merge | merge two or more caches into a single cache |
| -x | --fmt <fmt> | export format (one of [json*&#124;txt&#124;csv]) |

#### Hidden options (shown during testing only)
| -k | --skip | skip value for testing |
| -s | --start <num> | un-used hidden value - do not remove |
| -o | --noHeader | do not show header row of data |
#### Hidden options (shown during testing only)

| -v | --verbose | set verbose level. Either -v, --verbose or -v:n where 'n' is level |
| -h | --help | display this help screen |

