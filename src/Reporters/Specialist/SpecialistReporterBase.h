/*
* SpecialistReporterBase.h
 *
 *  Extend the Reporter base class to provide some common functions and data
 * that specialist database reporters may need.
 */
#ifndef SPECIALISTREPORTERBASE_H
#define SPECIALISTREPORTERBASE_H

#include <sqlite3.h>
#include "Reporters/Reporter.h"

class SpecialistReporterBase : public Reporter {
private:
    const std::string SELECT_AGGREGATION =
        "SELECT aggregationlevel FROM sim_replicate WHERE ID = ? AND endtime IS NULL";

    const std::string SELECT_LOCATION =
        "SELECT l.id, l.index FROM sim_replicate r INNER JOIN sim_location l ON l.configurationid = "
        "r.configurationid WHERE r.id = ? ORDER BY INDEX";

protected:
    const std::string SELECT_MONTHLYDATAID =
        "SELECT id FROM sim_monthlydata WHERE replicateid = ? ORDER BY dayselapsed DESC LIMIT 1";

    // The aggregation level used in reporting
    std::string aggregation;

    // The lookup to use when mapping cell to district, will be one-to-one if
    // cellular aggregation is used
    std::vector<int> lookup;

    // The maximum value in the lookup table, may not be the same as the size if
    // we are operating at the district level
    int lookup_allocation;

    // Build the lookup table that maps from cell to district, or cell to cell for
    // reporting
    void build_lookup(sqlite3* db);
};

#endif
