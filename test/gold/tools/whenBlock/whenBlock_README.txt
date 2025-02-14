whenBlock argc: 2 [1:-th] 
whenBlock -th 
#### Usage

`Usage:`    whenBlock [-l|-v|-h] &lt; block | date &gt; [ block... | date... ]  
`Purpose:`  Finds the nearest block prior to a date, or the nearest date prior to a block.
         Alternatively, search for one of special 'named' blocks.

`Where:`  

| Short Cut | Option | Description |
| -------: | :------- | :------- |
|  | block_list | one or more block numbers (or a 'special' block), or... |
|  | date_list | one or more dates formatted as YYYY-MM-DD[THH[:MM[:SS]]] |
| -l | --list | export all the named blocks |

#### Hidden options (shown during testing only)
| -x | --fmt <fmt> | export format (one of [none&#124;json*&#124;txt&#124;csv&#124;api]) |
#### Hidden options (shown during testing only)

| -v | --verbose | set verbose level. Either -v, --verbose or -v:n where 'n' is level |
| -h | --help | display this help screen |

`Notes:`

- Add custom special blocks by editing ~/.quickBlocks/whenBlock.toml.
- Use the following names to represent `special` blocks:
  - first (0), firstTrans (46147), iceage (200000), devcon1 (543626)
  - homestead (1150000), daofund (1428756), daohack (1718497), daofork (1920000)
  - devcon2 (2286910), tangerine (2463000), spurious (2675000), stateclear (2717576)
  - eea (3265360), ens2 (3327417), parityhack1 (4041179), byzantium (4370000)
  - devcon3 (4469339), parityhack2 (4501969), kitties (4605167), devcon4 (6610279)
  - constantinople (7280000), latest ()

