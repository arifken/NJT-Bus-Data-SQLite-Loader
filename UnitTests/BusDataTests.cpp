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

        delete loader;
    }

    TEST_F(BusDataTests, MethodLoadData) {

        const char *dirPath = RESOURCE_DIR_PATH;
        const char *dbPath = "/tmp/busdata_test.db";

        const int exp_calDates = 16;
        const int exp_agencies = 1;
        const int exp_routes = 3;
        const int exp_stopTimes = 7;
        const int exp_shapes = 5;
        const int exp_stops = 4;
        const int exp_trips = 2;

        BusDataLoader *loader = new BusDataLoader();
        int status = 0;
        loader->clear_old_database(dbPath);
        loader->create_database(dbPath, NULL);

        status = loader->load_data(dirPath, dbPath);

        ASSERT_EQ(status, 0);


        const char *error;
        int code;
        int rows;
        sqlite3 *db;


        sqlite3_open(dbPath, &db);

        // check the calendar_dates
        rows = get_table_count(db, "calendar_dates", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_calDates, rows);

        rows = get_table_count(db, "agencies", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_agencies, rows);

        rows = get_table_count(db, "routes", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_routes, rows);

        rows = get_table_count(db, "shapes", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_shapes, rows);

        rows = get_table_count(db, "stops", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_stops, rows);

        rows = get_table_count(db, "trips", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_trips, rows);

        rows = get_table_count(db, "stop_times", &code);
        ASSERT_TRUE((code == SQLITE_OK || code >= 100));
        ASSERT_EQ(exp_stopTimes, rows);

        sqlite3_close(db);

        delete loader;
    }


}
