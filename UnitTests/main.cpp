//
//  main.cpp
//  UnitTests
//
//  Created by  on 4/15/12.
//  Copyright (c) 2012 Home. All rights reserved.
//

#include <iostream>
#include "gtest/gtest.h"
#include "BusDataTests.h"


int main(int argc, const char *argv[]) {

    std::string exec_path = std::string(argv[0]);
    std::string dir_path = exec_path.substr(0, exec_path.find_last_of('/')).append("/TestResources");
    RESOURCE_DIR_PATH = dir_path.c_str();

    printf("\n Using test resources in directory: %s\n",RESOURCE_DIR_PATH);

    ::testing::InitGoogleTest(&argc, (char **) argv);
    return RUN_ALL_TESTS();
}

