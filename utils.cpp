#include "utils.h"

void printVector2D(const int* vec, int rows, int row_size) {
    std::cout << "Model grid:" << std::endl;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < row_size; j++) {
            std::cout << vec[i * row_size + j] << " ";
        }
        std::cout << std::endl;
    }
}

int* generateSpins(int rows_per_proc, int row_size, int rank) {
    std::mt19937 gen( MPI_PROC_NULL + rank );
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    int* spins = new int[rows_per_proc * row_size] ;

    double probability = 0.5;

    for (int i = 0; i < rows_per_proc; i++) {
        for (int j = 0; j < row_size; j++) {
            double rand = dis(gen);
            if (rand > probability) {
                spins[i * row_size + j] = 1;
            } else {
                spins[i * row_size + j] = 0;
            }
        }
    }
    return spins;
}

double single_spin_energy(int index, const int* grid, int row_size, double J, double B) {

  int energyNeigh = -2; 
  // wartosć początowa -2 bo w tablicy są tylko 0 i 1 
  // a dzieki temu nie trzeba konwersji wartości na +- 1/2
  int x = index / row_size;
  int y = index % row_size;

  // periodyczne warunki brzegowe
  int left = (y == 0) ? x * row_size + row_size - 1 : x * row_size + y - 1;
  int right = (y == row_size - 1) ? x * row_size : x * row_size + y + 1;
  int up = (x == 0) ? (row_size - 1) * row_size + y : (x - 1) * row_size + y;
  int down = (x == row_size - 1) ? y : (x + 1) * row_size + y;

  energyNeigh = grid[left] + grid[right] + grid[up] + grid[down];

  return (  J * static_cast<double>(grid[index]) * static_cast<double>(energyNeigh)
            + B * 0.25 * (  grid[index] ? 1.0 : -1.0 ) ); 
            // zamist (+-1/2)^2 mnożmy B razy 0.25 i uwzgl. znak
}

double calculateEnergyChange(int* grid, int idx, int row_size, int rows_per_proc, int num_proc) {
    int energy_change = 0;
    int spin = grid[idx];
    int left_idx = idx - 1;
    int right_idx = idx + 1;
    int up_idx = idx - row_size;
    int down_idx = idx + row_size;

    // Sprawdź wpływ sąsiadującego spinu po lewej stronie
    if (idx % row_size != 0) {
        int left_spin = grid[left_idx];
        energy_change += 2 * spin * left_spin;
    } else {
        int left_spin = grid[idx + row_size - 1];  // Periodyczne warunki brzegowe
        energy_change += 2 * spin * left_spin;
    }

    // Sprawdź wpływ sąsiadującego spinu po prawej stronie
    if (idx % row_size != row_size - 1) {
        int right_spin = grid[right_idx];
        energy_change += 2 * spin * right_spin;
    } else {
        int right_spin = grid[idx - row_size + 1];  // Periodyczne warunki brzegowe
        energy_change += 2 * spin * right_spin;
    }

    // Sprawdź wpływ sąsiadującego spinu powyżej
    if (idx >= row_size) {
        int up_spin = grid[up_idx];
        energy_change += 2 * spin * up_spin;
    } else {
        int up_spin = grid[idx + (row_size * rows_per_proc * num_proc) - row_size];  // Periodyczne warunki brzegowe
        energy_change += 2 * spin * up_spin;
    }

    // Sprawdź wpływ sąsiadującego spinu poniżej
    if (idx < (row_size * rows_per_proc * num_proc) - row_size) {
        int down_spin = grid[down_idx];
        energy_change += 2 * spin * down_spin;
    } else {
        int down_spin = grid[idx - (row_size * rows_per_proc * num_proc) + row_size];  // Periodyczne warunki brzegowe
        energy_change += 2 * spin * down_spin;
    }

    return static_cast<double>(energy_change);
}

double energy(int* grid, double J, double B, int row_size){
    double sum = 0.0;
    for(int i=0; i<row_size*row_size; i++){
        sum += single_spin_energy(i, grid, row_size, J, B);
    }
    return sum;
}

void saveGrid(int* grid, int row_size, std::string folderName) {
    char filename[256];
    const char *cstr = folderName.c_str();
    sprintf(filename, "%s/spins.txt", cstr);
    FILE* fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("Error: could not open file for writing.\n");
        return;
    }
    for (int i = 0; i < row_size; i++) {
        for (int j = 0; j < row_size; j++) {
            fprintf(fp, "%d ", grid[i*row_size+j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void saveMag(double mg, std::string folderName) {
    char filename[256];
    const char *cstr = folderName.c_str();
    sprintf(filename, "%s/avgMagnetism.txt", cstr);
    FILE* fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("Error: could not open file for writing.\n");
        return;
    }
    fprintf(fp, "%f\n", mg);
    fclose(fp);
}

void saveEnergy(double energy, std::string folderName) {
    char filename[256];
    const char *cstr = folderName.c_str();
    sprintf(filename, "%s/energy.txt", cstr);
    FILE* fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("Error: could not open file for writing.\n");
        return;
    }
    fprintf(fp, "%f\n", energy);
    fclose(fp);
}

std::string createFolderWithTimestampName(int rep){
    // Uzyskaj aktualny czas
    auto currentTime = std::chrono::system_clock::now();
    std::time_t currentTime_t = std::chrono::system_clock::to_time_t(currentTime);

    // Sformatuj czas jako string
    char timestamp[128];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&currentTime_t));
    std::string timestampStr(timestamp);

    // Stwórz ścieżkę do nowego folderu
    std::string folderName =  "result/" + timestampStr + "_" + std::to_string(rep);
    std::filesystem::path folderPath(folderName);

    // Sprawdź czy folder już istnieje
    if (std::filesystem::exists(folderPath))
    {
        std::cerr << "Folder o nazwie " << folderName << " już istnieje.\n";
        return folderName;
    }

    // Stwórz nowy folder
    if (!std::filesystem::create_directory(folderPath))
    {
        std::cerr << "Nie udało się utworzyć folderu " << folderName << "\n";
        return "ERROR";
    }

    std::cout << "Utworzono folder " << folderName << "\n";
    return folderName;
}

void saveParametersToFile(int netSize, double J, double B, long long iters, long long repeat) {
    std::ofstream file("parameters.txt");
    if (!file) {
        std::cout << "Failed to open file for writing." << std::endl;
        return;
    }

    file << "Net Size: " << netSize << std::endl;
    file << "J: " << J << std::endl;
    file << "B: " << B << std::endl;
    file << "Number of iterations: " << iters << std::endl;
    file << "Number repeats: " << repeat << std::endl;

    file.close();
    std::cout << "Parameters saved to file successfully." << std::endl;
}

void readParametersFromFile(int& netSize, double& J, double& B, long long& iters, long long& repeat) {
    std::ifstream file("parameters.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("Net Size:") != std::string::npos) {
                netSize = std::stoi(line.substr(line.find(":")+1));
            } else if (line.find("J:") != std::string::npos) {
                J = std::stod(line.substr(line.find(":")+1));
            } else if (line.find("B:") != std::string::npos) {
                B = std::stod(line.substr(line.find(":")+1));
            } else if (line.find("Number of iterations:") != std::string::npos) {
                iters = std::stoll(line.substr(line.find(":")+1));
            } else if (line.find("Number repeats:") != std::string::npos) {
                repeat = std::stoll(line.substr(line.find(":")+1));
            }
        }
        file.close();
    } else {
        std::cerr << "Error: could not open file for reading." << std::endl;
    }
}

int* flipSpin(int* grid, int idx, int row_size, int rows_per_proc, int num_proc) {
    int* new_grid = new int[row_size * rows_per_proc * num_proc];
    std::copy(grid, grid + row_size * rows_per_proc * num_proc, new_grid);
    new_grid[idx] = (new_grid[idx] == 0) ? 1 : 0;
    return new_grid;
}

double avgMagnetism(int* spinArray, int spinArraySize) {
    double sum = 0;
    for(int i=0; i<spinArraySize; i++) {
        sum += spinArray[i];
    }
    return sum / spinArraySize;
}

