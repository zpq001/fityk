// Implementation of class BruckerV1RawDataSet for reading meta-data and 
// xy-data from Siemens/Bruker Diffrac-AT Raw Format version 1 format files
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_brucker_raw_v1.cpp $

/*

FORMAT DESCRIPTION:
====================

Siemens/Bruker Diffrac-AT Raw Format version 1, data format used in 
Siemens/Brucker X-ray diffractors.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   ds_brucker_raw_v1
    * Extension name:   raw
    * Binary/Text:      binary
    * Multi-ranged:     Y
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
direct access, binary raw format without any record deliminators. 
All data are stored in records, and each record has a fixed length. Note that 
in one record, the data is stored as little-endian (see "endian" in wikipedia
www.wikipedia.org for details), so on the platform other than little-endian
(e.g. PDP and Sun SPARC), the byte-order needs be exchanged.

It may contain multiple groups/ranges in one file.
There is a common header in the head of the file, with fixed length fields 
indicating file-scope meta-info whose actrual meaning can be found in the 
format specification.Each range has its onw range-scope meta-info at the 
beginning, followed by the X-Y data.

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: 
mainly based on the file format specification: some scanned pages from a book, 
sent to me by Marcin Wojdyr (wojdyr@gmail.com). 
The title of these pages are "Appendix B: DIFFRAC-AT Raw Data File Format". 
In these pages, there are some tables listing every field of the DIFFRAC-AT v1 
and v2/v3 formats; and at the end, a sample FORTRAN program is given.

*/

#include "ds_brucker_raw_v1.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo BruckerV1RawDataSet::fmt_info(
    FT_BR_RAW1,
    "diffracat_v1_raw",
    "Siemens/Bruker Diffrac-AT Raw Format v1",
    vector<string>(1, "raw"),
    true,                       // whether binary
    true                        // whether multi-ranged
);


bool BruckerV1RawDataSet::check(istream &f)
{
    // the first 3 letters must be "RAW"
    f.clear();

    string head;
    try {
        head = read_string(f, 4);
    } catch (...) {
        return false;
    }
    
    if(f.rdstate() & ios::failbit) {
        return false;
    }

    f.seekg(0);     // reset the istream, as if no lines have been read
    return ("RAW" == head.substr(0, 3) && head[3] != '2');
}


void BruckerV1RawDataSet::load_data(std::istream &f) 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }
    clear();

    unsigned following_range;
    
    do {
        Range* p_rg = new Range;
        my_assert(p_rg != NULL, "no memory to allocate");
    
        f.ignore(4);
        unsigned cur_range_steps = read_uint32_le(f);  
        p_rg->add_meta("MEASUREMENT_TIME_PER_STEP", S(read_flt_le(f)));
        float x_step = read_flt_le(f); 
        p_rg->add_meta("SCAN_MODE", S(read_uint32_le(f)));
        f.ignore(4); 
        float x_start = read_flt_le(f);

        StepColumn *p_xcol = new StepColumn(x_start, x_step);
        my_assert(p_xcol != NULL, "no memory to allocate");
        
        float t = read_flt_le(f);
        if (-1e6 != t)
            p_rg->add_meta("THETA_START", S(t));
            
        t = read_flt_le(f);
        if (-1e6 != t)
            p_rg->add_meta("KHI_START", S(t));
            
        t = read_flt_le(f);
        if (-1e6 != t)
            p_rg->add_meta("PHI_START", S(t));

        p_rg->add_meta("SAMPLE_NAME", read_string(f, 32));
        p_rg->add_meta("K_ALPHA1", S(read_flt_le(f)));
        p_rg->add_meta("K_ALPHA2", S(read_flt_le(f)));

        f.ignore(72);   // unused fields
        following_range = read_uint32_le(f);
        
        VecColumn *p_ycol = new VecColumn();
        my_assert(p_ycol != NULL, "no memory to allocate");
        for(unsigned i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f);
            p_ycol->add_val(y);
        }
        p_rg->add_column(p_xcol, Range::CT_X);
        p_rg->add_column(p_ycol, Range::CT_Y);
        
        ranges.push_back(p_rg);
    } while (following_range > 0);
}

} // end of namespace xylib

