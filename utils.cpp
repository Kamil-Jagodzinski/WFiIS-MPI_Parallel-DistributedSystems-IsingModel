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


int* generateSpins(int rows_per_proc, int row_size) {
    std::random_device rd;
    std::mt19937 gen(rd());
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

double energy(int* grid, double J, double B, int row_size){
    double sum = 0.0;
    for(int i=0; i<row_size*row_size; i++){
        sum += single_spin_energy(i, grid, row_size, J, B);
    }
    return sum;
}

double single_spin_energy(int index, const int* grid, int row_size, double J, double B) {

  double energyNeigh = -2; 
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

  return (  J * grid[index] * energyNeigh 
            + B * 0.25 * (  grid[index] ? 1.0 : -1.0 ) ); 
            // zamist (+-1/2)^2 mnożmy B razy 0.25 i uwzgl. znak
}

void saveGrid(int* grid, int row_size, int iteration, std::string folderName) {
    char filename[256];
    const char *cstr = folderName.c_str();
    sprintf(filename, "%s/spins_%d.bin", cstr, iteration);
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("Error: could not open file for writing.\n");
        return;
    }
    fwrite(grid, sizeof(int), row_size*row_size, fp);
    fclose(fp);
}


std::string createFolderWithTimestampName()
{
    // Uzyskaj aktualny czas
    auto currentTime = std::chrono::system_clock::now();
    std::time_t currentTime_t = std::chrono::system_clock::to_time_t(currentTime);

    // Sformatuj czas jako string
    char timestamp[128];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&currentTime_t));
    std::string timestampStr(timestamp);

    // Stwórz ścieżkę do nowego folderu
    std::string folderName =  "./" + timestampStr;
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
    return timestampStr;
}
