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
    COption("data", "d", "", OPT_SWITCH, "Export prices as JSON data"),
    COption("freshen", "f", "", OPT_SWITCH, "Freshen database (append new data)"),
    COption("period", "p", "enum[5|15|30|120*|240|1440]", OPT_FLAG, "Display prices in this increment. One of [5|15|30|120*|240|1440]"),
    COption("pair", "p", "<pair>", OPT_FLAG, "Which price pair to freshen or list (see Poloniex)"),
    COption("fmt", "x", "enum[none|json*|txt|csv|api]", OPT_HIDDEN | OPT_FLAG, "export format (one of [none|json*|txt|csv|api])"),
    COption("", "", "", OPT_DESCRIPTION, "Freshen and/or display Ethereum price data and other purposes."),
// END_CODE_OPTIONS
};
static const size_t nParams = sizeof(params) / sizeof(COption);

//---------------------------------------------------------------------------------------------------
bool COptions::parseArguments(string_q& command) {

    if (!standardOptions(command))
        return false;

    bool noHeader = false;
    string_q format;
    Init();
    explode(arguments, command, ' ');
    for (auto arg : arguments) {
        string_q orig = arg;

        if (arg == "-f" || arg == "--freshen") {
            freshen = true;

        } else if (arg == "-d" || arg == "--data") {
            // we don't have to do anything, simply handling the option
            // enables the behavour. Don't remove.

        } else if (startsWith(arg, "-p:") || startsWith(arg, "--period:")) {
            arg = substitute(substitute(orig, "-p:", ""), "--period:", "");
            freq = str_2_Uint(arg);
            if (!isUnsigned(arg) || freq % 5)
                return usage("Positive multiple of 5 expected: " + orig);

        } else if (startsWith(arg, "--pair:")) {
            arg = substitute(orig, "--pair:", "");
            source.pair = arg;

        } else {
            if (!builtInCmd(arg)) {
                return usage("Invalid option: " + arg);
            }
        }
    }

    // Data wrangling
    // None

    // Display formatting
    switch (exportFmt) {
        case NONE1:
        case TXT1:
        case CSV1:
            format = getGlobalConfig("ethQuote")->getConfigStr("display", "format", format.empty() ? STR_DISPLAY_PRICEQUOTE : format);
            manageFields("CAccountName:" + cleanFmt(format, exportFmt));
            break;
        case API1:
        case JSON1:
            format = "";
            break;
    }
    expContext().fmtMap["format"] = expContext().fmtMap["header"] = cleanFmt(format, exportFmt);
    if (noHeader)
        expContext().fmtMap["header"] = "";

    string_q unused;
    if (!fileExists(source.getDatabasePath(unused))) {
        if (verbose)
            cerr << "Establishing price database cache.\n";
        establishFolder(source.getDatabasePath(unused));
    }

    return true;
}

//---------------------------------------------------------------------------------------------------
void COptions::Init(void) {
    registerOptions(nParams, params);

    freshen = false;
    freq = 120;
    first = true;
}

//---------------------------------------------------------------------------------------------------
COptions::COptions(void) {
    needsOption = true;
    Init();
}

//--------------------------------------------------------------------------------
COptions::~COptions(void) {
}

//--------------------------------------------------------------------------------
string_q COptions::postProcess(const string_q& which, const string_q& str) const {
    if (which == "options") {

    } else if (which == "notes" && (verbose || COptions::isReadme)) {
        string_q ret;
        ret += "Valid pairs include any pair from the public Poloniex's API here:|"
                "https://poloniex.com/public?command=returnCurrencies.\n";
        ret += "[{Note}]: Due to restrictions from Poloniex, this tool retrieves only 30 days of data|"
                "at a time. You must repeatedly run this command until the data is up-to-date.\n";
        return ret;
    }
    return str;
}
