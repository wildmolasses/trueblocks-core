/*-------------------------------------------------------------------------------------------
 * qblocks - fast, easily-accessible, fully-decentralized data from blockchains
 * copyright (c) 2018 Great Hill Corporation (http://greathill.com)
 *
 * This program is free software: you may redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version. This program is
 * distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details. You should have received a copy of the GNU General
 * Public License along with this program. If not, see http://www.gnu.org/licenses/.
 *-------------------------------------------------------------------------------------------*/
#include "options.h"

//---------------------------------------------------------------------------------------------------
static const COption params[] = {
// BEG_CODE_OPTIONS
    COption("addr_list", "", "list<addr>", OPT_REQUIRED | OPT_POSITIONAL, "one or more addresses (0x...) from which to retrieve balances"),
    COption("block_list", "", "list<block>", OPT_POSITIONAL, "an optional list of one or more blocks at which to report balances, defaults to 'latest'"),
    COption("mode", "m", "enum[none|some*|all|balance|nonce|code|storage|deployed|accttype]", OPT_FLAG, "control which state to export. One of [none|some*|all|balance|nonce|code|storage|deployed|accttype]"),
    COption("nozero", "n", "", OPT_SWITCH, "suppress the display of zero balance accounts"),
    COption("changes", "c", "", OPT_SWITCH, "only report a balance when it changes from one block to the next"),
    COption("noHeader", "o", "", OPT_HIDDEN | OPT_SWITCH, "hide the header in txt and csv mode"),
    COption("fmt", "x", "enum[none|json*|txt|csv|api]", OPT_HIDDEN | OPT_FLAG, "export format (one of [none|json*|txt|csv|api])"),
    COption("", "", "", OPT_DESCRIPTION, "Retrieve the balance (in wei) for one or more addresses at the given block(s)."),
// END_CODE_OPTIONS
};
static const size_t nParams = sizeof(params) / sizeof(COption);

//---------------------------------------------------------------------------------------------------
bool COptions::parseArguments(string_q& command) {

    if (!standardOptions(command))
        return false;

    bool noHeader = false;

    Init();
    explode(arguments, command, ' ');
    for (auto arg : arguments) {
        if (arg == "-n" || arg == "--nozero") {
            exclude_zero = true;

        } else if (arg == "-c" || arg == "--changes") {
            changes = true;

        } else if (arg == "-o" || arg == "--noHeader") {
            noHeader = true;

        } else if (startsWith(arg, "-m:") || startsWith(arg, "--mode:")) {

            arg = substitute(substitute(arg, "-m:", ""), "--mode:", "");
            if (arg == "none") mode = ST_NONE;
            if (arg == "balance") mode = ethstate_t(mode|ST_BALANCE);
            if (arg == "nonce") mode = ethstate_t(mode|ST_NONCE);
            if (arg == "code") mode = ethstate_t(mode|ST_CODE);
            if (arg == "storage") mode = ethstate_t(mode|ST_STORAGE);
            if (arg == "deployed") mode = ethstate_t(mode|ST_DEPLOYED);
            if (arg == "accttype") mode = ethstate_t(mode|ST_ACCTTYPE);
            if (arg == "some") mode = ethstate_t(mode|ST_SOME);
            if (arg == "all") mode = ethstate_t(mode|ST_ALL);

        } else if (isHash(arg)) {

            string_q ret = blocks.parseBlockList(arg, newestBlock);
            if (endsWith(ret, "\n")) {
                cerr << "\n  " << ret << "\n";
                return false;
            } else if (!ret.empty()) {
                return usage(ret);
            }

        } else if (startsWith(arg, "0x")) {
            if (!isAddress(arg))
                return usage(arg + " does not appear to be a valid Ethereum address. Quitting...");
            addrs.push_back(toLower(arg));

        } else if (startsWith(arg, '-')) {  // do not collapse
            if (!builtInCmd(arg)) {
                return usage("Invalid option: " + arg);
            }

        } else {

            string_q ret = blocks.parseBlockList(arg, newestBlock);
            if (endsWith(ret, "\n")) {
                cerr << "\n  " << ret << "\n";
                return false;
            } else if (!ret.empty()) {
                return usage(ret);
            }
        }
    }

    // Data wrangling
    if (!blocks.hasBlocks())
        blocks.numList.push_back(newestBlock);  // use 'latest'

    if (!addrs.size())
        return usage("You must provide at least one Ethereum address.");

    deminimus = str_2_Wei(getGlobalConfig("getState")->getConfigStr("settings", "deminimus", "0"));

    UNHIDE_FIELD(CEthState, "address");
    string_q format = STR_DISPLAY_ETHSTATE;
    if (!(mode & ST_BALANCE))  { replace(format, "\t[{BALANCE}]",  ""); } else { UNHIDE_FIELD(CEthState, "balance"); UNHIDE_FIELD(CEthState, "ether"); }
    if (!(mode & ST_NONCE))    { replace(format, "\t[{NONCE}]",    ""); } else { UNHIDE_FIELD(CEthState, "nonce");    }
    if (!(mode & ST_CODE))     { replace(format, "\t[{CODE}]",     ""); } else { UNHIDE_FIELD(CEthState, "code");     }
    if (!(mode & ST_STORAGE))  { replace(format, "\t[{STORAGE}]",  ""); } else { UNHIDE_FIELD(CEthState, "storage");  }
    if (!(mode & ST_DEPLOYED)) { replace(format, "\t[{DEPLOYED}]", ""); } else { UNHIDE_FIELD(CEthState, "deployed"); }
    if (!(mode & ST_ACCTTYPE)) { replace(format, "\t[{ACCTTYPE}]", ""); } else { UNHIDE_FIELD(CEthState, "accttype"); }

    // Display formatting
    switch (exportFmt) {
        case NONE1:
        case TXT1:
        case CSV1:
            format = getGlobalConfig("getState")->getConfigStr("display", "format", format.empty() ? STR_DISPLAY_ETHSTATE : format);
            manageFields("CEthState:" + cleanFmt(format, exportFmt));
            break;
        case API1:
        case JSON1:
            format = "";
            break;
    }
    if (expContext().asEther)
        format = substitute(format, "{BALANCE}", "{ETHER}");
    if (expContext().asDollars)
        format = substitute(format, "{BALANCE}", "{DOLLARS}");
    expContext().fmtMap["format"] = expContext().fmtMap["header"] = cleanFmt(format, exportFmt);
    if (noHeader)
        expContext().fmtMap["header"] = "";

    if ((!isTestMode() && !hasHistory()) || nodeHasBalances())
        return true;

    // We need history, so try to get a different server. Fail silently. The user will be warned in the response
    string_q rpcProvider = getGlobalConfig()->getConfigStr("settings", "rpcProvider", "http://localhost:8545");
    string_q balanceProvider = getGlobalConfig()->getConfigStr("settings", "balanceProvider", rpcProvider);
    if (rpcProvider == balanceProvider || balanceProvider.empty())
        return true;

    // We release the curl context so we can set it to the new context.
    getCurlContext()->baseURL = balanceProvider;
    getCurlContext()->releaseCurl();
    getCurlContext()->getCurl();

    return true;
}

//---------------------------------------------------------------------------------------------------
void COptions::Init(void) {
    registerOptions(nParams, params);
    optionOn(OPT_RAW | OPT_OUTPUT);

    exclude_zero = false;
    changes = false;
    prevBal = 0;
    mode = ST_BALANCE;

    addrs.clear();
    current = "";
    blocks.Init();
    CHistoryOptions::Init();
    newestBlock = oldestBlock = getLastBlock_client();
}

//---------------------------------------------------------------------------------------------------
COptions::COptions(void) : CHistoryOptions() {
    sorts[0] = GETRUNTIME_CLASS(CBlock);
    sorts[1] = GETRUNTIME_CLASS(CTransaction);
    sorts[2] = GETRUNTIME_CLASS(CReceipt);
    Init();
    first = true;
}

//--------------------------------------------------------------------------------
COptions::~COptions(void) {
}

//--------------------------------------------------------------------------------
string_q COptions::postProcess(const string_q& which, const string_q& str) const {
    if (which == "options") {
        return substitute(str, "addr_list block_list", "<address> [address...] [block...]");

    } else if (which == "notes" && (verbose || COptions::isReadme)) {
        string_q ret;
        ret += "[{addresses}] must start with '0x' and be forty two characters long.\n";
        ret += "[{block_list}] may be a space-separated list of values, a start-end range, a [{special}], or any combination.\n";
        ret += "This tool retrieves information from the local node or rpcProvider if configured (see documentation).\n";
        ret += "If the queried node does not store historical state, the results are undefined.\n";
        ret += "[{special}] blocks are detailed under " + cTeal + "[{whenBlock --list}]" + cOff + ".\n";
        ret += "[{balance}] is the default mode. To select a single mode use [{none}] first, followed by that mode.\n";
        ret += "You may specify multiple modes on a single line.\n";
        return ret;
    }
    return str;
}
