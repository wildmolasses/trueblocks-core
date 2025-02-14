/*-------------------------------------------------------------------------
 * This source code is confidential proprietary information which is
 * Copyright (c) 2017 by Great Hill Corporation.
 * All Rights Reserved
 *------------------------------------------------------------------------*/
#include "options.h"

//---------------------------------------------------------------------------------------------------
static const COption params[] = {
// BEG_CODE_OPTIONS
    COption("fn_list", "", "list<fn>", OPT_REQUIRED | OPT_POSITIONAL, "path(s) of files to check, merge, fix or display (default=display)"),
    COption("check", "c", "", OPT_SWITCH, "check for duplicates and other problems in the cache"),
    COption("data", "d", "", OPT_SWITCH, "in 'list' mode, render results as data (i.e export mode)"),
    COption("sort", "s", "", OPT_SWITCH, "sort the list of transactions and re-write (precludes other modes, other than --dedup)"),
    COption("fix", "f", "", OPT_SWITCH, "remove duplicates from the cache (if any)"),
    COption("list", "l", "", OPT_SWITCH, "list the contents of the cache (the default if no other option)"),
    COption("import", "i", "", OPT_SWITCH, "import transactions if import.txt file exists in current folder"),
    COption("remove", "r", "", OPT_SWITCH, "remove transactions if remove.txt file exists in current folder"),
    COption("truncate", "t", "<blknum>", OPT_FLAG, "truncate the cache at block :n (keeps block 'n' and before, implies --fix)"),
    COption("maxBlock", "k", "<blknum>", OPT_FLAG, "for testing, max block to visit"),
    COption("merge", "m", "", OPT_SWITCH, "merge two or more caches into a single cache"),
    COption("fmt", "x", "enum[none|json*|txt|csv|api]", OPT_FLAG, "export format (one of [json*|txt|csv])"),
    COption("skip", "k", "", OPT_HIDDEN | OPT_SWITCH, "skip value for testing"),
    COption("start", "s", "<blknum>", OPT_HIDDEN | OPT_FLAG, "un-used hidden value - do not remove"),
    COption("noHeader", "o", "", OPT_HIDDEN | OPT_SWITCH, "do not show header row of data"),
    COption("", "", "", OPT_DESCRIPTION, "Show the contents of an account cache and/or fix it by removing duplicate records.\n"),
// END_CODE_OPTIONS
};
static const size_t nParams = sizeof(params) / sizeof(COption);

//---------------------------------------------------------------------------------------------------
bool COptions::parseArguments(string_q& command) {

    ENTER("parseArguments");
    if (!standardOptions(command))
        EXIT_NOMSG(false);

    bool isMerge = false, isSort = false;
    Init();
    explode(arguments, command, ' ');
    for (auto arg : arguments) {
        if (arg == "-c" || arg == "--check") {
            mode = "check|" + mode;  // always do 'checks' first

        } else if (arg == "--fix") {
            if (!contains(mode, "fix"))
                mode += "fix|";
            replace(mode, "list|fix", "fix|list");  // always do 'fixes' first

        } else if (arg == "-l" || arg == "--list") {
            mode += "list|";  // do 'listing' in order found
            exportFmt = JSON1;

        } else if (startsWith(arg, "-t:") || startsWith(arg, "--truncate:")) {
            arg = substitute(substitute(arg, "-t:", ""), "--truncate:", "");
            if (!isNumeral(arg))
                EXIT_USAGE("You must supply a block number with the --truncate:n command.");
            trunc = str_2_Uint(arg);
            if (trunc > getLastBlock_client())
                EXIT_USAGE("You must supply a block number lower than the latest block.");
            if (!contains(mode, "fix"))
                mode += "fix|";
            replace(mode, "list|fix", "fix|list");  // do 'fixing' prior to 'listing'

        } else if (startsWith(arg, "--start:")) {
            // dummy value

        } else if (startsWith(arg, "-k:") || startsWith(arg, "--maxBlock:")) {
            arg = substitute(substitute(arg, "-k:", ""), "--maxBlock:", "");
            if (!isNumeral(arg))
                EXIT_USAGE("You must supply a block number with the --maxBlock:n command.");
            maxBlock = str_2_Uint(arg);
            if (maxBlock == 0)
                maxBlock = NOPOS;

        } else if (arg == "-m" || arg == "--merge") {
            isMerge = true;

        } else if (arg == "-i" || arg == "--import") {
            if (!fileExists("./import.txt"))
                EXIT_USAGE("File ./import.txt not found.");
            isImport = true;

        } else if (arg == "-r" || arg == "--remove") {
            if (!fileExists("./remove.txt"))
                EXIT_USAGE("File ./remove.txt not found.");
            cerr << cGreen << "Found removal file...\n";
            string_q contents;
            asciiFileToString("./remove.txt", contents);
            CStringArray lines;
            explode(lines, contents, '\n');
            for (auto line : lines) {
                CAppearance_base item(line);
                if (item.blk > 0) {
                    removals.push_back(item);
                    LOG_INFO(cYellow, "\tremoval instruction: ", cTeal, removals.size(), "-", item.blk, ".", item.txid, cOff, "\r");
                }
            }
            if (!isTestMode()) {
                copyFile("./remove.txt", "remove.bak");
                remove("./remove.txt");
            }
            sort(removals.begin(), removals.end());
            isRemove = (removals.size() > 0);

        } else if (arg == "-s" || arg == "--sort") {
            isSort = true;

        } else if (arg == "-d" || arg == "--data") {
            asData = true;
            colorsOff();

        } else if (arg == "-o" || arg == "--noHeader") {
            // do nothing

        } else if (startsWith(arg, "-k:") || startsWith(arg, "--skip:")) {
            string_q arg1 = substitute(substitute(arg, "-k:", ""), "--skip:", "");
            skip = str_2_Uint(arg1);
            if (!skip)
                skip = 1;

        } else if (startsWith(arg, '-')) {  // do not collapse
            if (!builtInCmd(arg)) {
                EXIT_USAGE("Invalid option: " + arg);
            }

        } else {
            address_t addr = toLower(substitute(arg, ".acct.bin", ""));
            if (isTestMode()) cerr << addr << endl;
            if (!isTestMode() && !isAddress(addr))
                EXIT_USAGE("Option '" + arg + "' does not appear to be an address.");

            // Command line and chifra send in straight addresses, some test cases may send a local file...
            string_q path = (fileExists(arg + ".acct.bin") ? (arg + ".acct.bin") : getMonitorPath(addr));
            if (isTestMode()) cerr << path << endl;
            if (!fileExists(path)) // Hack alert: some weird test cases send in 'merged' as the address.
                EXIT_USAGE("Monitor file for '" + arg + "' does not exist.");

            monitors.push_back(CAccountWatch(addr, path));
        }
    }

    if (monitors.size() == 0 && !isImport)
        EXIT_USAGE("You must provide at least one filename.");
    if (mode.empty())
        mode = "list|";
    if (isMerge && monitors.size() < 2)
        EXIT_USAGE("Merge command needs at least two filenames.");
    if ((isSort || isRemove) && monitors.size() != 1)
        EXIT_USAGE("Command requires a single filename.");

    if (isSort) {
        handleSort();
        EXIT_NOMSG(false);

    } else if (isMerge) {
        handleMerge();
        EXIT_NOMSG(false);

    } else if (isImport) {
        if (monitors.empty() || monitors.size() > 1)
            EXIT_USAGE("Please provide a single address for this import.");
        handleImport();
        EXIT_NOMSG(false);

    } else if (isRemove) {
        handleRemove();
        EXIT_NOMSG(false);

    }
    EXIT_NOMSG(true);
}

//---------------------------------------------------------------------------------------------------
void COptions::Init(void) {
    registerOptions(nParams, params);
    optionOn(OPT_PREFUND | OPT_OUTPUT);

    monitors.clear();
    mode = "";
    trunc = 0;
    asData = false;
    maxBlock = NOPOS;
    skip = 1;
    isRemove = false;
    isImport = false;
}

//---------------------------------------------------------------------------------------------------
COptions::COptions(void) {
    Init();
}

//--------------------------------------------------------------------------------
COptions::~COptions(void) {
}

//-----------------------------------------------------------------------
bool loadMonitorData(CAppearanceArray_base& items, const address_t& addr) {
    ENTER("loadMonitorData");
    string_q fn = getMonitorPath(addr);
    if (isTestMode()) {
        replace(fn, getMonitorPath(""), "./");
        if (!contains(fn, ".acct.bin"))
            fn += ".acct.bin";
    }
    size_t nRecords = (fileSize(fn) / sizeof(CAppearance_base));
    ASSERT(nRecords);
    CAppearance_base *buffer = new CAppearance_base[nRecords];
    if (buffer) {
        bzero(buffer, nRecords * sizeof(CAppearance_base));
        CArchive txCache(READING_ARCHIVE);
        if (txCache.Lock(fn, modeReadOnly, LOCK_NOWAIT)) {
            txCache.Read(buffer, sizeof(CAppearance_base), nRecords);
            txCache.Release();
        } else {
            EXIT_FAIL("Could not open cache file.");
        }
        // Add to the items which may be non-empty
        items.reserve(items.size() + nRecords);
        for (size_t i = 0 ; i < nRecords ; i++)
            items.push_back(buffer[i]);
        delete [] buffer;
    } else {
        EXIT_FAIL("Could not allocate memory for address " + addr);
    }
    EXIT_NOMSG(true);
}
