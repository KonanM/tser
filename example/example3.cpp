// Licensed under the Boost License <https://opensource.org/licenses/BSL-1.0>.
// SPDX-License-Identifier: BSL-1.0
#include <iostream>
#include <tser/tser.hpp>

int main()
{
    tser::BinaryArchive ba;
    static constexpr size_t N = 5;
    size_t grid5x5[N][N]{};
    //fill the grid with some numbers
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            grid5x5[i][j] = i * N + j;
        }
    }

    ba.save(grid5x5);
    std::cout << ba;
    //with compression (35 chars): AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGA
    //wihtout compression (268 chars): 
    //AAAAAAAAAAABAAAAAAAAAAIAAAAAAAAAAwAAAAAAAAAEAAAAAAAAAAUAAAAAAAAABgAAAAAAAAAHAAAAAAAAAAg
    //AAAAAAAAACQAAAAAAAAAKAAAAAAAAAAsAAAAAAAAADAAAAAAAAAANAAAAAAAAAA4AAAAAAAAADwAAAAAAAAAQAA
    //AAAAAAABEAAAAAAAAAEgAAAAAAAAATAAAAAAAAABQAAAAAAAAAFQAAAAAAAAAWAAAAAAAAABcAAAAAAAAAGAAAAAAAAAA
}
