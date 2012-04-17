#include "BusDataLoader.h"


void usage() {
    std::cout << "\nUsage: $ ./loader [path to bus data directory] [output sqlite directory]\n";
}

int main(int argc, const char *argv[]) {

    if (argc != 3) {
        usage();
        return 0;
    }

    const char *dir_path = argv[1];
    const char *db_path = argv[2];

    BusDataLoader *loader = new BusDataLoader();

    loader->clear_old_database(db_path);
    loader->create_database(db_path, NULL);
    loader->load_data(dir_path, db_path);

    delete loader;

    return 0;
}




