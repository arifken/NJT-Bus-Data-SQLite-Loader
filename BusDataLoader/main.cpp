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

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sqlite3.h>
#include <dirent.h>
#include <stdlib.h>
#include <vector>
#include <iterator>


const char *fn_calendarDates = "calendar_dates.txt";
const char *fn_routes = "routes.txt";
const char *fn_stopTimes = "stop_times.txt";
const char *fn_stops = "stops.txt";
const char *fn_trips = "trips.txt";
const char *fn_shapes = "shapes.txt";
const char *fn_agency = "agency.txt";

using namespace std;

void usage() {
    std::cout << "\nUsage: $ ./loader [path to bus data directory] [output sqlite directory]\n";
}

int create_database(const char *path) {
    printf("\ncreating database at %s",path);
    sqlite3 *db = NULL;
    int ok = 0;

    ok = sqlite3_open(path, &db);
    if (ok == SQLITE_OK) {
        sqlite3_stmt *stmt = NULL;
        const char *pzTail;

        int numTables = 7;
        char const *sql[] = {
                "CREATE TABLE agencies (agency_id INTEGER PRIMARY KEY, agency_name TEXT, agency_url TEXT, agency_timezone TEXT, agency_lang TEXT, agency_phone TEXT)",

                "CREATE TABLE calendar_dates (id INTEGER PRIMARY KEY, service_id INTEGER, date INTEGER, exception_type INTEGER)",

                "CREATE TABLE routes (route_id INTEGER PRIMARY KEY, agency_id INTEGER, route_short_name TEXT, route_long_name TEXT, route_type INTEGER, route_url TEXT, route_color TEXT)",

                "CREATE TABLE stop_times (id INTEGER PRIMARY KEY, trip_id INTEGER, arrival_time TEXT, departure_time TEXT, stop_id INTEGER, stop_sequence INTEGER, pickup_type INTEGER, drop_off_type INTEGER, shape_dist_traveled REAL)",

                "CREATE TABLE stops (stop_id INTEGER PRIMARY KEY, stop_code INTEGER, stop_name TEXT, stop_desc TEXT, stop_lat REAL, stop_lon REAL, zone_id INTEGER)",

                "CREATE TABLE trips (trip_id INTEGER PRIMARY KEY, route_id INTEGER, service_id INTEGER, trip_headsign TEXT, direction_id INTEGER, block_id TEXT, shape_id INTEGER)",

                "CREATE TABLE shapes (shape_id INTEGER PRIMARY KEY, shape_pt_lat REAL, shape_pt_lon REAL, shape_pt_sequence INTEGER, shape_dist_traveled REAL)",
        };

        printf("\nCreated %i tables", numTables);
        for (int i = 0; i < numTables; i++) {
            sqlite3_prepare_v2(db, sql[i], strlen(sql[i]), &stmt, &pzTail);
            sqlite3_step(stmt);
        }

    }

    sqlite3_close(db);

}

vector<string> split_line(string line) {
    vector<string> ret;
    char delim = ',';
    stringstream ss(line);
    string item;
    while (std::getline(ss, item, delim)) {
        ret.push_back(item);
    }
    return ret;
}

void get_column_names(sqlite3 *db, string tableName, vector<string> *colNames, int *columnCount) {
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


int insert_data(string filePath, sqlite3 *db, string tableName, vector<string> *column_names) {

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
    int serviceId;
    long date;
    int exceptionType;
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
        while (file.good()) {
            lineCtr++;
            getline(file, line);

            // the first line is a description of the fields, so start at line 2
            if (lineCtr > 1 && line.length() > 0) {
                comps = split_line(line);

                valsArg.clear();


                if (column_count == comps.size()) {
                    for (unsigned int i = 0; i < column_count; i++) {
                        valsArg.append(comps.at(i));
                        if (i < column_count - 1) {
                            valsArg.append(", ");
                        }
                    }
                } else {
                    sprintf(statusStr, "line %i: component count doesnt match column count", lineCtr);
                    warningLines.push_back(statusStr);
                }

                sprintf(cSql, "INSERT INTO %s (%s) VALUES(%s)", tableName.c_str(), colsArg.c_str(), valsArg.c_str());

                sql = string(cSql);
//                printf("%s\n",cSql);
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
            }

        }


        printf("Loading %s...................................done\n",tableName.c_str());

        for (int j = 0; j < warningLines.size(); j++) {
            printf("    WARN: %s\n",warningLines.at(j).c_str());
        }
        printf("\n");
    }

    if (free_cols) {
        delete column_names;
    }

    file.close();

    return 0;
}


int load_calendar_dates(char const *dir_path, sqlite3 *db) {
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

int load_routes(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_routes);

    vector<string> *cols = new vector<string>();
    cols->push_back("route_id");
    cols->push_back("agency_id");
    cols->push_back("route_short_name");
    cols->push_back("route_long_name");
    cols->push_back("route_type");
    cols->push_back("route_url");

    insert_data(joined, db, "routes", cols);

    delete cols;

    return 0;
}

int load_stop_times(char const *dir_path, sqlite3 *db) {

}

int load_stops(char const *dir_path, sqlite3 *db) {

}

int load_trips(char const *dir_path, sqlite3 *db) {

}

int load_agencies(char const *dir_path, sqlite3 *db) {

}

int load_shapes(char const *dir_path, sqlite3 *db) {

}


int load_data(char const *dir_path, char const *db_path) {

    sqlite3 *db;
    sqlite3_open(db_path, &db);
    printf("\n\n");

    load_calendar_dates(dir_path, db);
    load_routes(dir_path, db);

    sqlite3_close(db);
    return 0;

}

void clear_old_database(char const *dbPath) {
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

int main(int argc, const char *argv[]) {

    if (argc != 3) {
        usage();
        return 0;
    }

    const char *dir_path = argv[1];
    const char *db_path = argv[2];

    clear_old_database(db_path);
    create_database(db_path);
    load_data(dir_path, db_path);

    return 0;
}




