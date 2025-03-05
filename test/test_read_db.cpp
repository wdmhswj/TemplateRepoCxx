#include "Database.h"
#include <iostream>
int main(int argc, char** argv) {
    if (argc <= 1)
    {
        std::cerr << "need db file name\n";
        return 1;
    }
    
    Database db(argv[1]);
    db.fetchAllPages();
    return 0;
}