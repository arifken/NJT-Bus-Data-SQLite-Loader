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


agencies
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


int BusDataLoader::create_database(const char *path) {
    printf("\ncreating database at %s", path);
    sqlite3 *db = NULL;
    int ok = 0;

    ok = sqlite3_open(path, &db);
    if (ok == SQLITE_OK) {
        sqlite3_stmt *stmt = NULL;
        const char *pzTail;

        int numTables = 7;
        char const *sql[] = {
                "CREATE TABLE agencies (id INTEGER PRIMARY KEY, agency_id INTEGER, agency_name TEXT, agency_url TEXT, agency_timezone TEXT, agency_lang TEXT, agency_phone TEXT)",

                "CREATE TABLE calendar_dates (id INTEGER PRIMARY KEY, service_id INTEGER, date INTEGER, exception_type INTEGER)",

                "CREATE TABLE routes (id INTEGER PRIMARY KEY, route_id INTEGER, agency_id INTEGER, route_short_name TEXT, route_long_name TEXT, route_type INTEGER, route_url TEXT, route_color TEXT)",

                "CREATE TABLE stop_times (id INTEGER PRIMARY KEY, trip_id INTEGER, arrival_time TEXT, departure_time TEXT, stop_id INTEGER, stop_sequence INTEGER, pickup_type INTEGER, drop_off_type INTEGER, shape_dist_traveled REAL)",

                "CREATE TABLE stops (id INTEGER PRIMARY KEY, stop_id INTEGER, stop_code INTEGER, stop_name TEXT, stop_desc TEXT, stop_lat REAL, stop_lon REAL, zone_id INTEGER)",

                "CREATE TABLE trips (id INTEGER PRIMARY KEY, trip_id INTEGER, route_id INTEGER, service_id INTEGER, trip_headsign TEXT, direction_id INTEGER, block_id TEXT, shape_id INTEGER)",

                "CREATE TABLE shapes (id INTEGER PRIMARY KEY, shape_id INTEGER, shape_pt_lat REAL, shape_pt_lon REAL, shape_pt_sequence INTEGER, shape_dist_traveled REAL)",
        };

        printf("\nCreated %i tables", numTables);
        for (int i = 0; i < numTables; i++) {
            sqlite3_prepare_v2(db, sql[i], strlen(sql[i]), &stmt, &pzTail);
            sqlite3_step(stmt);
        }

    }

    sqlite3_close(db);

    return 0;
}

/*!
 * CSV parser function borrowed from http://www.zedwood.com/article/112/cpp-csv-parser
 */
void BusDataLoader::csvline_populate(vector<string> &record, const string& line, char delimiter) {
    int linepos = 0;
    int inquotes = false;
    char c;
    int i;
    int linemax = line.length();
    string curstring;
    record.clear();

    while (line[linepos] != 0 && linepos < linemax) {

        c = line[linepos];

        if (!inquotes && curstring.length() == 0 && c == '"') {
            //beginquotechar
            inquotes = true;
        }
        else if (inquotes && c == '"') {
            //quotechar
            if ((linepos + 1 < linemax) && (line[linepos + 1] == '"')) {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curstring.push_back(c);
                linepos++;
            }
            else {
                //endquotechar
                inquotes = false;
            }
        }
        else if (!inquotes && c == delimiter) {
            //end of field
            record.push_back(curstring);
            curstring = "";
        }
        else if (!inquotes && (c == '\r' || c == '\n')) {
            record.push_back(curstring);
            return;
        }
        else {
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

    sqlite3_stmt *stmt = NULL;
    ifstream file;
    string line;
    char cSql[1024];
    string sql;
    const char *pzTail;
    vector<string> comps;
    string colsArg;
    string valsArg;

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
        while (file.good()) {
            lineCtr++;
            getline(file, line);

            // the first line is a description of the fields, so start at line 2
            if (lineCtr > 1 && line.length() > 0) {
                //comps = split_line(line);
                csvline_populate(comps, line, ',');
                valsArg.clear();


                if (column_count == comps.size()) {
                    for (unsigned int i = 0; i < column_count; i++) {

                        comp = comps.at(i);
//                        valsArg.append(comp);

                        if (comp.length() == 0) {
                            valsArg.append("\"\"");
                        } else if (!is_number(comp) && comp.at(0) != '"') {
                            valsArg.append('"' + comp + '"');
                        } else {
                            valsArg.append(comp);
                        }
                        if (i < column_count - 1) {
                            valsArg.append(", ");
                        }
                    }
                    sprintf(cSql, "INSERT INTO %s (%s) VALUES(%s)", tableName.c_str(), colsArg.c_str(), valsArg.c_str());

                    sql = string(cSql);
//                if (tableName[0] == 's') {
//                    printf("%s\n", cSql);
//                }


                    printf("Loading %s...................................%i\r", tableName.c_str(), lineCtr);
                    fflush(stdout);

                    sqlite3_prepare_v2(db, sql.c_str(), sql.length(), &stmt, &pzTail);
                    status = sqlite3_step(stmt);

                    if (status != SQLITE_OK && status < 100) {
                        statusMsg = sqlite3_errmsg(db);
                        int ln = lineCtr;
                        sprintf(statusStr, "line %i: caught error %i: %s", ln, status, statusMsg);
                        warningLines.push_back(string(statusStr));
                    }
                } else {
                    sprintf(statusStr, "line %i: component count doesnt match column count", lineCtr);
                    warningLines.push_back(statusStr);
                }

            }

        }

        printf("Loading %s...................................done\n", tableName.c_str());

        for (int j = 0; j < warningLines.size(); j++) {
            printf("    WARN: %s\n", warningLines.at(j).c_str());
        }
        printf("\n");
    }

    if (free_cols) {
        delete column_names;
    }

    file.close();

    return 0;
}


int BusDataLoader::load_calendar_dates(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_calendarDates);

    vector<string> *cols = new vector<string>();
    cols->push_back("service_id");
    cols->push_back("date");
    cols->push_back("exception_type");

    insert_data(joined, db, "calendar_dates", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_routes(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_routes);

    vector<string> *cols = new vector<string>();
    cols->push_back("route_id");
    cols->push_back("agency_id");
    cols->push_back("route_short_name");
    cols->push_back("route_long_name");
    cols->push_back("route_type");
    cols->push_back("route_url");
    cols->push_back("route_color");

    insert_data(joined, db, "routes", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_stop_times(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_stopTimes);

    vector<string> *cols = new vector<string>();
    cols->push_back("trip_id");
    cols->push_back("arrival_time");
    cols->push_back("departure_time");
    cols->push_back("stop_id");
    cols->push_back("stop_sequence");
    cols->push_back("pickup_type");
    cols->push_back("drop_off_type");
    cols->push_back("shape_dist_traveled");

    insert_data(joined, db, "stop_times", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_stops(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_stops);

    vector<string> *cols = new vector<string>();
    cols->push_back("stop_id");
    cols->push_back("stop_code");
    cols->push_back("stop_name");
    cols->push_back("stop_desc");
    cols->push_back("stop_lat");
    cols->push_back("stop_lon");
    cols->push_back("zone_id");

    insert_data(joined, db, "stops", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_trips(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_trips);

    vector<string> *cols = new vector<string>();
    cols->push_back("trip_id");
    cols->push_back("route_id");
    cols->push_back("service_id");
    cols->push_back("trip_headsign");
    cols->push_back("direction_id");
    cols->push_back("block_id");
    cols->push_back("shape_id");

    insert_data(joined, db, "trips", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_agencies(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_agency);

    vector<string> *cols = new vector<string>();
    cols->push_back("agency_id");
    cols->push_back("agency_name");
    cols->push_back("agency_url");
    cols->push_back("agency_timezone");
    cols->push_back("agency_lang");
    cols->push_back("agency_phone");

    insert_data(joined, db, "agencies", cols);

    delete cols;

    return 0;
}

int BusDataLoader::load_shapes(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_shapes);

    vector<string> *cols = new vector<string>();
    cols->push_back("shape_id");
    cols->push_back("shape_pt_lat");
    cols->push_back("shape_pt_lon");
    cols->push_back("shape_pt_sequence");
    cols->push_back("shape_dist_traveled");

    insert_data(joined, db, "shapes", cols);

    delete cols;

    return 0;
}


int BusDataLoader::load_data(char const *dir_path, char const *db_path) {

    sqlite3 *db;
    sqlite3_open(db_path, &db);
    printf("\n\n");

    load_calendar_dates(dir_path, db);
    load_routes(dir_path, db);
    load_stops(dir_path, db);
    load_trips(dir_path, db);
    load_agencies(dir_path, db);
    load_shapes(dir_path, db);
    load_stop_times(dir_path, db);

    sqlite3_close(db);
    return 0;

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

