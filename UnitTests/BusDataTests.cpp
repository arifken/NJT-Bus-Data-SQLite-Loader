/*!
 * \file    BusDataTests
 * \project 
 * \author  Andy Rifken 
 * \date    4/15/12.
 *
 */



#include "BusDataTests.h"
#include "BusDataLoader.h"
#include <string>

const char *RESOURCE_DIR_PATH = "";

int BusDataTests::get_table_count(sqlite3 *db, char const *table, int *status) {
    sqlite3_stmt *stmt;
    int code, rows;
    std::string sql = std::string("select count(*) from ").append(table);
    sqlite3_prepare_v2(db, sql.c_str(), sql.length(), &stmt, NULL);
    code = sqlite3_step(stmt);
    if (status != NULL) {
        *status = code;
    }
    rows = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return rows;
}
namespace {

    TEST_F(BusDataTests, MethodClearOldDatabase) {
        // create a file
        char const *path = "/tmp/deleteme.txt";
        std::ofstream os;
        std::ifstream is;
        os.open(path);
        os.write("test", 4);
        os.close();

        // assert exists
        is.open(path);
        ASSERT_TRUE(is.good());
        is.close();

        // delete it
        BusDataLoader *loader = new BusDataLoader();
        loader->clear_old_database(path);
        delete loader;

        // assert gone
        is.open(path);
        ASSERT_FALSE(is.good());
        is.close();
    }

    TEST_F(BusDataTests, MethodCreateDatabase) {
        const char *path = "/tmp/busdata_test.db";

        BusDataLoader *loader = new BusDataLoader();

        // delete database first
        loader->clear_old_database(path);

        const char *errmsg;
        int status = loader->create_database(path, &errmsg);

        // check status code
        printf("\nstatus code = %i", status);
        printf("\nerrmsg = %s", errmsg);

        ASSERT_FALSE((status > 0 && status < 100));

        // ensure that file exists
        std::ifstream fstream;
        fstream.open(path);
        ASSERT_TRUE(fstream.good());
        fstream.close();


        // check table column order
        sqlite3 *db;
        sqlite3_open(path, &db);

        sqlite3_close(db);


        delete loader;
    }

    TEST_F(BusDataTests, MethodLoadData) {

        const char *dirPath = RESOURCE_DIR_PATH;
        const char *dbPath = "/tmp/busdata_test.db";
        const char *error;
        int code;
        int rows;
        sqlite3 *db;
        sqlite3_stmt *stmt;
        const char *sql;
        int intCol;
        double realCol;
        const char *textCol;
        BusDataLoader *loader;

        const int exp_calDates = 16;
        const int exp_agencies = 1;
        const int exp_routes = 3;
        const int exp_stopTimes = 7;
        const int exp_shapes = 5;
        const int exp_stops = 4;
        const int exp_trips = 2;

        // ------------ setup --------------------------------

        loader = new BusDataLoader();
        int status = 0;
        loader->clear_old_database(dbPath);
        loader->create_database(dbPath, NULL);

        status = loader->load_data(dirPath, dbPath);

        ASSERT_EQ(status, 0);


        sqlite3_open(dbPath, &db);

        //------------------- check counts -------------------------


        rows = get_table_count(db, "calendar_date", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_calDates, rows);

        rows = get_table_count(db, "agency", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_agencies, rows);

        rows = get_table_count(db, "route", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_routes, rows);

        rows = get_table_count(db, "shape", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_shapes, rows);

        rows = get_table_count(db, "stop", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_stops, rows);

        rows = get_table_count(db, "trip", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_trips, rows);

        rows = get_table_count(db, "stop_time", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_stopTimes, rows);


        //-------- check data inserted into correct columns -----------------


        // calendar_dates
        sql = "select * from calendar_date";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        sqlite3_step(stmt);

        intCol = sqlite3_column_int(stmt, 1); // service_id
        ASSERT_EQ(1, intCol);

        textCol = (char const *) sqlite3_column_text(stmt, 2); // date
        ASSERT_STREQ("20120406", textCol);

        intCol = sqlite3_column_int(stmt, 3); // exception_type
        ASSERT_EQ(1, intCol);

        sqlite3_finalize(stmt);


        // agency
        sql = "select * from agency";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        sqlite3_step(stmt);

        intCol = sqlite3_column_int(stmt, 1); // agency_id
        ASSERT_EQ(1, intCol);

        textCol = (char const *) sqlite3_column_text(stmt, 2); // agency_name
        ASSERT_STREQ("NJ TRANSIT BUS", textCol);

        textCol = (char const *) sqlite3_column_text(stmt, 3); // agency_url
        ASSERT_STREQ("http://www.njtransit.com/", textCol);

        textCol = (char const *) sqlite3_column_text(stmt, 4); // agency_timezone
        ASSERT_STREQ("America/New_York", textCol);

        textCol = (char const *) sqlite3_column_text(stmt, 5); // agency_lang
        ASSERT_STREQ("en", textCol);

        sqlite3_finalize(stmt);


        // routes
        sql = "select * from route";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        sqlite3_step(stmt);

        intCol = sqlite3_column_int(stmt, 1); // route_id
        ASSERT_EQ(1, intCol);

        intCol = sqlite3_column_int(stmt, 2); // agency_id
        ASSERT_EQ(1, intCol);

        textCol = (char const *) sqlite3_column_text(stmt, 3); // route_short_name
        ASSERT_STREQ("1", textCol);

        textCol = (char const *) sqlite3_column_text(stmt, 4); // route_long_name
        ASSERT_STREQ("", textCol);

        intCol = sqlite3_column_int(stmt, 5); // route_type
        ASSERT_EQ(3, intCol);

        textCol = (char const *) sqlite3_column_text(stmt, 6); // route_url
        ASSERT_STREQ("", textCol);

        sqlite3_finalize(stmt);


        // stops
        sql = "select * from stop";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        sqlite3_step(stmt);

        intCol = sqlite3_column_int(stmt, 1); // stop_id
        ASSERT_EQ(7, intCol);

        intCol = sqlite3_column_int(stmt, 2); // stop_code
        ASSERT_EQ(21263, intCol);

        textCol = (char const *) sqlite3_column_text(stmt, 3); // stop_name
        ASSERT_STREQ("ELM ST AT MIDLAND AVE#", textCol);

        textCol = (char const *) sqlite3_column_text(stmt, 4); // stop_desc
        ASSERT_STREQ("", textCol);

        realCol = sqlite3_column_double(stmt, 5); // stop_lat
        ASSERT_EQ(40.769019, realCol);

        realCol = sqlite3_column_double(stmt, 6); // stop_lat
        ASSERT_EQ(-74.141421, realCol);

        intCol = sqlite3_column_int(stmt, 7); // zone_id
        ASSERT_EQ(589, intCol);

        sqlite3_finalize(stmt);



        // trips (route_id,service_id,trip_id,trip_headsign,direction_id,block_id,shape_id)
        sql = "select * from trip";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        sqlite3_step(stmt);

        intCol = sqlite3_column_int(stmt, 1); // route_id
        ASSERT_EQ(48, intCol);

        intCol = sqlite3_column_int(stmt, 2); // service_id
        ASSERT_EQ(1, intCol);

        intCol = sqlite3_column_int(stmt, 3); // trip_id
        ASSERT_EQ(12141, intCol);

        textCol = (char const *) sqlite3_column_text(stmt, 4); // trip_headsign
        ASSERT_STREQ("163P NEW YORK PARKWAY EXP", textCol);

        intCol = sqlite3_column_int(stmt, 5); // direction_id
        ASSERT_EQ(0, intCol);

        textCol = (char const *) sqlite3_column_text(stmt, 6); // block_id
        ASSERT_STREQ("164OD014", textCol);

        intCol = sqlite3_column_int(stmt, 7); // shape_id
        ASSERT_EQ(1333, intCol);

        sqlite3_finalize(stmt);




        // stop_times
        sql = "select * from stop_time";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        sqlite3_step(stmt);

        intCol = sqlite3_column_int(stmt, 1); // trip_id
        ASSERT_EQ(1, intCol);

        textCol = (char const *) sqlite3_column_text(stmt, 2); // arrival_time
        ASSERT_STREQ("06:08:00", textCol);

        textCol = (char const *) sqlite3_column_text(stmt, 3); // departure_time
        ASSERT_STREQ("06:08:00", textCol);

        intCol = sqlite3_column_int(stmt, 4); // stop_id
        ASSERT_EQ(2204, intCol);

        intCol = sqlite3_column_int(stmt, 5); // stop_sequence
        ASSERT_EQ(1, intCol);

        intCol = sqlite3_column_int(stmt, 6); // pickup_type
        ASSERT_EQ(0, intCol);

        intCol = sqlite3_column_int(stmt, 7); // drop_off_type
        ASSERT_EQ(0, intCol);

        realCol = sqlite3_column_double(stmt, 8); // shape_dist_traveled
        ASSERT_EQ(0.7345, realCol);

        sqlite3_finalize(stmt);


        // shapes
        sql = "select * from shape";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        sqlite3_step(stmt);

        intCol = sqlite3_column_int(stmt, 1); // shape_id
        ASSERT_EQ(6493, intCol);

        realCol = sqlite3_column_double(stmt, 2); // shape_pt_lat
        ASSERT_EQ(40.750758, realCol);

        realCol = sqlite3_column_double(stmt, 3); // shape_pt_lon
        ASSERT_EQ(-74.179943, realCol);

        intCol = sqlite3_column_int(stmt, 4); // shape_pt_sequence
        ASSERT_EQ(15, intCol);

        realCol = sqlite3_column_double(stmt, 5); // shape_dist_traveled
        ASSERT_EQ(7.1193, realCol);

        sqlite3_finalize(stmt);


        // ----------- cleanup -------------------

        sqlite3_close(db);
        delete loader;
    }


}
