/*!
 * \file    BusDataTests
 * \project 
 * \author  Andy Rifken 
 * \date    4/15/12.
 *
 */




#ifndef __BusDataTests_H_
#define __BusDataTests_H_

#include <iostream>
#include <sqlite3.h>
#include "gtest/gtest.h"

extern const char *RESOURCE_DIR_PATH;

class BusDataTests : public ::testing::Test {
    public:

    static int get_table_count(sqlite3 *db, char const *table, int *status);

};

#endif //__BusDataTests_H_
