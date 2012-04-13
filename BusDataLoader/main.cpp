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

int create_database(const char *path, sqlite3 **retDb) {
    printf("\ncreating database...");
    sqlite3 *db = NULL;
    int ok = 0;

    ok = sqlite3_open(path, &db);
    if (ok == SQLITE_OK) {
        sqlite3_stmt *stmt;
        const char *pzTail;

        int numTables = 7;
        char const *sql[] = {
                "CREATE TABLE agencies (agency_id INTEGER PRIMARY KEY, agency_name TEXT, agency_url TEXT, agency_timezone TEXT, agency_lang TEXT, agency_phone TEXT)",

                "CREATE TABLE calendar_dates (id INTEGER PRIMARY KEY, service_id INTEGER, date INTEGER, exception_type INTEGER)",

                "CREATE TABLE routes (route_id INTEGER PRIMARY KEY, agency_id INTEGER, rout_short_name TEXT, route_long_name TEXT, route_type INTEGER, route_url TEXT, route_color TEXT)",

                "CREATE TABLE stop_times (id INTEGER PRIMARY KEY, trip_id INTEGER, arrival_time TEXT, departure_time TEXT, stop_id INTEGER, stop_sequence INTEGER, pickup_type INTEGER, drop_off_type INTEGER, shape_dist_traveled REAL)",

                "CREATE TABLE stops (stop_id INTEGER PRIMARY KEY, stop_code INTEGER, stop_name TEXT, stop_desc TEXT, stop_lat REAL, stop_lon REAL, zone_id INTEGER)",

                "CREATE TABLE trips (trip_id INTEGER PRIMARY KEY, route_id INTEGER, service_id INTEGER, trip_headsign TEXT, direction_id INTEGER, block_id TEXT, shape_id INTEGER)",

                "CREATE TABLE shapes (shape_id INTEGER PRIMARY KEY, shape_pt_lat REAL, shape_pt_lon REAL, shape_pt_sequence INTEGER, shape_dist_traveled REAL)",
        };

        printf("\nnum statments: %i", numTables);
        for (int i = 0; i < numTables; i++) {
            sqlite3_prepare_v2(db, sql[i], strlen(sql[i]), &stmt, &pzTail);
            sqlite3_step(stmt);
            sqlite3_free(stmt), stmt = NULL;
        }

    }

    sqlite3_close(db);

    if (retDb != NULL) {
        *retDb = db;
    }
}

vector<string> split_line(string line) {
    vector<string> ret;
    char delim = ',';
    stringstream ss(line);
    string item;
    while(std::getline(ss, item, delim)) {
        ret.push_back(item);
    }
    return ret;
}

int load_calendar_dates(char const *dir_path, sqlite3 *db) {
    std::string joined = std::string(dir_path);
    joined.append("/").append(fn_calendarDates);

    printf("\n Loading calendar_dates: %s", joined.c_str());

    sqlite3_stmt *stmt;
    ifstream file;
    string line;
    string sql;
    int serviceId;
    long date;
    int exceptionType;
    vector<string> comps;
    file.open(joined.c_str());
    if (file.is_open()) {
        while (file.good()) {
            getline(file, line);
            comps = split_line(line);

            if (comps.size() == 3) {
                serviceId = atoi(comps.at(0).c_str());
                date = atoi(comps.at(1).c_str());
                exceptionType = atoi(comps.at(2).c_str());

                sql = "INSERT INTO calendar_dates (service_id, date, exception_type) VALUES(";
                sqlite3_prepare_v2(db, sql.c_str(), sql.length(), &stmt, NULL);
                sqlite3_prepare
                sqlite3_step(stmt);
                sqlite3_free(stmt), stmt = NULL;


                printf("\n  calender id %i, date %i, exception %i:",serviceId, date,exceptionType);
            }

        }
    }

    file.close();

    return 0;

}

int load_routes(const char *path) {

}

int load_stop_times(const char *path) {

}

int load_stops(const char *path) {

}

int load_trips(const char *path) {

}

int load_agencies(const char *path) {

}

int load_shapes(const char *path) {

}


int load_data(char const *dir_path, sqlite3 *db) {
    load_calendar_dates(dir_path, db);
    return 0;

}

int main(int argc, const char *argv[]) {

    if (argc != 3) {
        usage();
        return 0;
    }

    const char *dir_path = argv[1];
    const char *db_path = argv[2];

    sqlite3 *db;
    create_database(db_path, &db);
    load_data(dir_path, db);

    return 0;
}




