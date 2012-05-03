/*!
 * \file    BusDataLoader
 * \project 
 * \author  Andy Rifken 
 * \date    4/15/12.
 *
 */




#ifndef __BusDataLoader_H_
#define __BusDataLoader_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sqlite3.h>
#include <vector>
#include <iterator>

class BusDataLoader {
    public:

    int load_data(char const *dir_path, char const *db_path);

    void clear_old_database(char const *dbPath);

    int create_database(char const *path, const char **error_msg);

    private:

    void csvline_populate(std::vector <std::string> &record, const std::string& line, char delimiter);

    void get_column_names(sqlite3 *db, std::string tableName, std::vector<std::string> *colNames, int *columnCount);

    bool is_number(const std::string& s);

    int insert_data(std::string filePath, sqlite3 *db, std::string tableName, std::vector<std::string> *column_names);

    int load_calendar_dates(char const *dir_path, sqlite3 *db);

    int load_routes(char const *dir_path, sqlite3 *db);

    int load_stop_times(char const *dir_path, sqlite3 *db);

    int load_stops(char const *dir_path, sqlite3 *db);

    int load_trips(char const *dir_path, sqlite3 *db);

    int load_agency(char const *dir_path, sqlite3 *db);

    int load_shapes(char const *dir_path, sqlite3 *db);

    const char* token_for_index(int i);

};

#endif //__BusDataLoader_H_
