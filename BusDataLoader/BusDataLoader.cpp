/*!
 * \file    BusDataLoader
 * \project 
 * \author  Andy Rifken 
 * \date    4/15/12.
 *
 */


/*
schema:

calendar_dates
--------------
service_id      integer (pk)
date            integer (unix timestamp)
exception_type  integer


routes
-------
route_id            integer (pk)
agency_id           integer (join agency table)
route_short_name    text
route_long_name     text
route_type          integer
route_url           text
route_color         text?

stop_times
----------
trip_id             integer (join trips)
arrival_time        text (HH:MM:SS)
departure_time      text (HH:MM:SS)
stop_id             integer (join stops table)
stop_sequence       integer
pickup_type         integer
drop_off_type       integer
shape_dist_traveled real

stops
------
stop_id             integer (pk)
stop_code           integer
stop_name           text
stop_desc           text
stop_lat            real
stop_lon            real
zone_id             integer

trips
------
trip_id             integer (pk)
route_id            integer
service_id          integer
trip_headsign       text
direction_id        integer
block_id            text
shape_id            integer


agency
-------
agency_id           integer (pk)
agency_name         text
agency_url          text
agency_timezone     text
agency_lang         text
agency_phone        text


shapes
------
shape_id                integer (pk)
shape_pt_lat            real
shape_pt_lon            real
shape_pt_sequence       integer
shape_dist_traveled     real


 */

#include "BusDataLoader.h"

const char *fn_calendarDates = "calendar_dates.txt";
const char *fn_routes = "routes.txt";
const char *fn_stopTimes = "stop_times.txt";
const char *fn_stops = "stops.txt";
const char *fn_trips = "trips.txt";
const char *fn_shapes = "shapes.txt";
const char *fn_agency = "agency.txt";

using namespace std;


int BusDataLoader::create_database(char const *path, const char **error_msg) {
    printf("\ncreating database at %s", path);
    sqlite3 *db = NULL;
    int status = 0;

    status = sqlite3_open(path, &db);
    if (status == SQLITE_OK) {
        sqlite3_stmt *stmt = NULL;
        const char *pzTail;

        int numTables = 7;
        char const *sql[] = {"CREATE TABLE agency (id INTEGER PRIMARY KEY, agency_id INTEGER, agency_name VARCHAR, agency_url VARCHAR, agency_timezone VARCHAR, agency_lang VARCHAR, agency_phone VARCHAR)",

                "CREATE TABLE calendar_date (id INTEGER PRIMARY KEY, service_id INTEGER, date VARCHAR, exception_type INTEGER)",

                "CREATE TABLE route (id INTEGER PRIMARY KEY, route_id INTEGER, agency_id INTEGER, route_short_name VARCHAR, route_long_name VARCHAR, route_type INTEGER, route_url VARCHAR, route_color VARCHAR)",

                "CREATE TABLE stop_time (id INTEGER PRIMARY KEY, trip_id INTEGER, arrival_time VARCHAR, departure_time VARCHAR, stop_id INTEGER, stop_sequence INTEGER, pickup_type INTEGER, drop_off_type INTEGER, shape_dist_traveled REAL)",

                "CREATE TABLE stop (id INTEGER PRIMARY KEY, stop_id INTEGER, stop_code INTEGER, stop_name VARCHAR, stop_desc TEXT, stop_lat REAL, stop_lon REAL, zone_id INTEGER)",

                "CREATE TABLE trip (id INTEGER PRIMARY KEY, route_id INTEGER, service_id INTEGER, trip_id INTEGER, trip_headsign VARCHAR, direction_id INTEGER, block_id VARCHAR, shape_id INTEGER)",

                "CREATE TABLE shape (id INTEGER PRIMARY KEY, shape_id INTEGER, shape_pt_lat REAL, shape_pt_lon REAL, shape_pt_sequence INTEGER, shape_dist_traveled REAL)",};

//        printf("\ncurr status = %i",status);
        printf("\nCreating %i tables", numTables);
        for (int i = 0; i < numTables; i++) {
            sqlite3_prepare_v2(db, sql[i], strlen(sql[i]), &stmt, &pzTail);
            status = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
//            printf("\nstatus at %i: %i",i,status);
            const char *error = sqlite3_errmsg(db);

            if (error_msg != NULL) {
                *error_msg = error;
            }
        }

    }

    sqlite3_close(db);

    return status;
}

void BusDataLoader::clear_old_database(char const *dbPath) {
    ifstream oldDb;
    bool exists = false;
    oldDb.open(dbPath);
    if (oldDb.good()) {
        exists = true;
    }
    oldDb.close();

    if (exists) {
        remove(dbPath);
    }
}


/*!
 * CSV parser function borrowed from http://www.zedwood.com/article/112/cpp-csv-parser
 */
void BusDataLoader::csvline_populate(vector<string> &record, const string& line, char delimiter) {
    int linepos = 0;
    int inquotes = false;
    char c;
    int linemax = line.length();
    string curstring;
    record.clear();

    while (line[linepos] != 0 && linepos < linemax) {

        c = line[linepos];

        if (!inquotes && curstring.length() == 0 && c == '"') {
            //beginquotechar
            inquotes = true;
        } else if (inquotes && c == '"') {
            //quotechar
            if ((linepos + 1 < linemax) && (line[linepos + 1] == '"')) {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curstring.push_back(c);
                linepos++;
            } else {
                //endquotechar
                inquotes = false;
            }
        } else if (!inquotes && c == delimiter) {
            //end of field
            record.push_back(curstring);
            curstring = "";
        } else if (!inquotes && (c == '\r' || c == '\n')) {
            record.push_back(curstring);
            return;
        } else {
            curstring.push_back(c);
        }
        linepos++;
    }
    record.push_back(curstring);
    return;
}

void BusDataLoader::get_column_names(sqlite3 *db, string tableName, vector<string> *colNames, int *columnCount) {
    char c_sql[1024];
    sprintf(c_sql, "select * from %s", tableName.c_str());
    string sql = string(c_sql);
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql.c_str(), sql.length(), &stmt, NULL);

    int ct = sqlite3_column_count(stmt);

    if (columnCount != NULL) {
        *columnCount = ct;
    }

    vector<string> names;
    for (int i = 0; i < ct; i++) {
        names.push_back(sqlite3_column_name(stmt, i));
    }

    if (colNames != NULL) {
        *colNames = names;
    }
}

bool BusDataLoader::is_number(const std::string& s) {
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}


int BusDataLoader::insert_data(string filePath, sqlite3 *db, string tableName, vector<string> *column_names) {
    bool free_cols = false;
    if (!column_names) {
        column_names = new vector<string>();
        get_column_names(db, tableName, column_names, NULL);
    }

    int retStatus = 0;
    sqlite3_stmt *stmt = NULL;
    bool stmtCreated = false;
    ifstream file;
    string line;
    char cSql[1024];
    string sql;
    const char *pzTail;
    vector<string> comps;
    string colsArg;
    string valsArg;
    char *transactionErrMsg;


    unsigned long column_count = column_names->size();
    for (unsigned int i = 0; i < column_count; i++) {
        colsArg.append(column_names->at(i));
        if (i < column_count - 1) {
            colsArg.append(", ");
        }
    }

    file.open(filePath.c_str());

    if (file.is_open()) {
        vector<string> warningLines;
        unsigned int lineCtr = 0;
        int status;
        char statusStr[1024];
        const char *statusMsg;
        string comp;
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &transactionErrMsg);
        while (file.good()) {
            lineCtr++;
            getline(file, line);

            // the first line is a description of the fields, so start at line 2
            if (lineCtr > 1 && line.length() > 0) {
                //comps = split_line(line);
                csvline_populate(comps, line, ',');

                printf("Loading %s...................................%i\r", tableName.c_str(), lineCtr);
                fflush(stdout);

                if (!stmtCreated) {
                    valsArg.clear();
                    for (unsigned int i = 0; i < comps.size(); i++) {
                        if (i > 0) {
                            valsArg.append(",");
                        }
                        valsArg.append("?");
                    }
                    sprintf(cSql, "INSERT INTO %s (%s) VALUES(%s)", tableName.c_str(), colsArg.c_str(), valsArg.c_str());
                    status = sqlite3_prepare_v2(db, cSql, strlen(cSql), &stmt, &pzTail);
                    stmtCreated = true;
                }

                string(tmp);


                for (unsigned int i = 0; i < comps.size(); i++) {
                    const char *sqlCmp = comps.at((size_t) i).c_str();
//                    printf("\nsql cmp: %s", sqlCmp);
                    sqlite3_bind_text(stmt, i + 1, sqlCmp, -1, SQLITE_TRANSIENT);
                }

                status = sqlite3_step(stmt);
                sqlite3_clear_bindings(stmt);
                sqlite3_reset(stmt);

                if (status != SQLITE_OK && status < 100) {
                    retStatus = 1;
                    statusMsg = sqlite3_errmsg(db);
                    int ln = lineCtr;
                    sprintf(statusStr, "line %i: caught error %i: %s", ln, status, statusMsg);
                    warningLines.push_back(string(statusStr));
                }
            }

        }

        sqlite3_finalize(stmt);
        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &transactionErrMsg);
        stmtCreated = false;

        printf("Loading %s...................................done\n", tableName.c_str());

        for (unsigned int j = 0; j < warningLines.size(); j++) {
            printf("    WARN: %s\n", warningLines.at(j).c_str());
        }
        printf("\n");
    }
    if (free_cols) {
        delete column_names;
    }

    file.close();

    return retStatus;
}


int BusDataLoader::load_calendar_dates(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_calendarDates);

    int status = 0;

    vector<string> *cols = new vector<string>();
    cols->push_back("service_id");
    cols->push_back("date");
    cols->push_back("exception_type");

    status = insert_data(joined, db, "calendar_date", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_routes(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_routes);

    int status = 0;

    vector<string> *cols = new vector<string>();
    cols->push_back("route_id");
    cols->push_back("agency_id");
    cols->push_back("route_short_name");
    cols->push_back("route_long_name");
    cols->push_back("route_type");
    cols->push_back("route_url");
    cols->push_back("route_color");

    status = insert_data(joined, db, "route", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_stop_times(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_stopTimes);

    int status = 0;

    vector<string> *cols = new vector<string>();
    cols->push_back("trip_id");
    cols->push_back("arrival_time");
    cols->push_back("departure_time");
    cols->push_back("stop_id");
    cols->push_back("stop_sequence");
    cols->push_back("pickup_type");
    cols->push_back("drop_off_type");
    cols->push_back("shape_dist_traveled");

    status = insert_data(joined, db, "stop_time", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_stops(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_stops);

    int status = 0;

    vector<string> *cols = new vector<string>();
    cols->push_back("stop_id");
    cols->push_back("stop_code");
    cols->push_back("stop_name");
    cols->push_back("stop_desc");
    cols->push_back("stop_lat");
    cols->push_back("stop_lon");
    cols->push_back("zone_id");

    status = insert_data(joined, db, "stop", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_trips(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_trips);

    int status = 0;

    vector<string> *cols = new vector<string>();
    cols->push_back("route_id");
    cols->push_back("service_id");
    cols->push_back("trip_id");
    cols->push_back("trip_headsign");
    cols->push_back("direction_id");
    cols->push_back("block_id");
    cols->push_back("shape_id");

    status = insert_data(joined, db, "trip", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_agency(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_agency);

    int status = 0;

    vector<string> *cols = new vector<string>();
    cols->push_back("agency_id");
    cols->push_back("agency_name");
    cols->push_back("agency_url");
    cols->push_back("agency_timezone");
    cols->push_back("agency_lang");
    cols->push_back("agency_phone");

    status = insert_data(joined, db, "agency", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_shapes(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_shapes);

    int status = 0;

    vector<string> *cols = new vector<string>();
    cols->push_back("shape_id");
    cols->push_back("shape_pt_lat");
    cols->push_back("shape_pt_lon");
    cols->push_back("shape_pt_sequence");
    cols->push_back("shape_dist_traveled");

    status = insert_data(joined, db, "shape", cols);

    delete cols;

    return status;
}

int BusDataLoader::create_indices(sqlite3 *db) {
    const char *sql;
    char errMsg[1024];

    sql = "CREATE INDEX idx_st on stop_time(stop_id)";
    if (sqlite3_exec(db, sql, NULL, NULL, (char **) &errMsg) != SQLITE_OK) {
        printf("\nError creating indices: %s", errMsg);
        return -1;
    }

    sql = "CREATE INDEX idx_trip on trip(trip_id);";
    if (sqlite3_exec(db, sql, NULL, NULL, (char **) &errMsg) != SQLITE_OK) {
        printf("\nError creating indices: %s", errMsg);
        return -1;
    }

    sql = "CREATE INDEX idx_cd on calendar_date(date, service_id)";
    if (sqlite3_exec(db, sql, NULL, NULL, (char **) &errMsg) != SQLITE_OK) {
        printf("\nError creating indices: %s", errMsg);
        return -1;
    }

    return 0;
}


int BusDataLoader::load_data(char const *dir_path, char const *db_path) {

    sqlite3 *db;
    sqlite3_open(db_path, &db);
    printf("\n\n");

    int status = 0;

    int failureCt = 0;

    failureCt += load_calendar_dates(dir_path, db);
    failureCt += load_routes(dir_path, db);
    failureCt += load_stops(dir_path, db);
    failureCt += load_trips(dir_path, db);
    failureCt += load_agency(dir_path, db);
    failureCt += load_shapes(dir_path, db);
    failureCt += load_stop_times(dir_path, db);


    if (failureCt != 0) {
        status = 1;
        printf("\nData load failed with %i errors.",failureCt);
    } else {
        printf("\ncreating indices............................");
        status = create_indices(db);
        printf("done\n\n");
    }

    sqlite3_close(db);


    return status;

}


