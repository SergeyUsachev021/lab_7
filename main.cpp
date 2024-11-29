#include <iostream> // Подключаем библиотеку для ввода-вывода
#include <string> // Подключаем библиотеку для работы со строками
#include <vector> // Подключаем библиотеку для работы с векторами
#include <filesystem> // Подключаем библиотеку для работы с файловой системой
#include <boost/crc.hpp> // Подключаем библиотеку Boost для вычисления CRC32
#include <unordered_map> // Подключаем библиотеку для работы с хеш-таблицами
#include <algorithm> // Подключаем библиотеку для алгоритмов (например, сортировка)
#include <fstream> // Подключаем библиотеку для работы с файлами
#include <regex> // Подключаем библиотеку для работы с регулярными выражениями
// #include <boost/regex.hpp> // Закомментировано: регулярные выражения для обработки строк, не используется в коде
#include <set> // Подключаем библиотеку для работы с множествами (для хранения дубликатов)

namespace fs = std::filesystem; // Создаем псевдоним для пространства имен файловой системы

// Функция для вычисления CRC32
uint32_t calculate_crc32(const std::string& data) {
    boost::crc_32_type crc; // Создаем объект для вычисления CRC32
    crc.process_bytes(data.data(), data.size()); // Обрабатываем данные, переданные в функцию
    return crc.checksum(); // Возвращаем полученную контрольную сумму
}

// Функция для обработки файла и получения последовательности хэшей  
std::vector<uint32_t> file_processing(const fs::path& filePath, size_t blockSize) {
    std::vector<uint32_t> hashSequence; // Вектор для хранения хэшей блоков
    std::ifstream file(filePath, std::ios::binary); // Открываем файл в бинарном режиме

    if (!file) { // Проверяем, удалось ли открыть файл
        throw std::runtime_error("Cannot open file: " + filePath.string()); // Выбрасываем исключение, если файл не удалось открыть
    }

    std::string buffer(blockSize, '\0'); // Создаем буфер заданного размера, инициализируя его нулями

    while (file.read(&buffer[0], blockSize) || file.gcount() > 0) { // Чтение данных из файла блоками
        size_t bytesRead = static_cast<size_t>(file.gcount()); // Получаем количество прочитанных байтов
        buffer.resize(bytesRead); // Урезаем буфер до фактического размера прочитанных данных
        
        if (bytesRead < blockSize) { // Если прочитано меньше байтов, чем размер блока
            buffer.resize(blockSize, '\0'); // Дополняем до размера блока нулями
        }

        uint32_t hash = calculate_crc32(buffer); // Вычисляем хэш блока и сохраняем его
        hashSequence.push_back(hash); // Добавляем хэш в последовательность
    }

    return hashSequence; // Возвращаем вектор хэшей
}

// Функция для сравнения двух файлов по их хэшам
bool compare_hashes(const std::vector<uint32_t>& hashes1, const std::vector<uint32_t>& hashes2) { 
    if (hashes1.size() != hashes2.size()) return false; // Если размеры разные, файлы разные

    for (size_t i = 0; i < hashes1.size(); ++i) { 
        if (hashes1[i] != hashes2[i]) return false; // Если хотя бы один хэш не совпадает, файлы разные
    }

    return true; // Если все хэши совпадают, файлы идентичны
}

// Функция для обработки каждого файла в директории
void shouldProcessFile(const fs::directory_entry& entry, const std::vector<fs::path>& exclusions, size_t minSize, const std::regex& maskRegex, size_t blockSize, std::vector<std::pair<fs::path, std::vector<uint32_t>>>& hashVector) {
    if (entry.is_regular_file()) {  // Проверяем, является ли это обычным файлом
        
        if (std::find(exclusions.begin(), exclusions.end(), entry.path().parent_path()) != exclusions.end()) { 
            return; // Пропускаем файл, если его родительская директория в списке исключений
        }

        if (entry.file_size() < minSize) { 
            return; // Пропускаем файл, если его размер меньше минимального значения
        }

        if (!std::regex_match(entry.path().filename().string(), maskRegex)) { 
            return; // Пропускаем файл, если его имя не соответствует маске регулярного выражения
        }

        auto hashes = file_processing(entry.path(), blockSize); // Получаем хэши файла по блокам

        hashVector.emplace_back(entry.path(), hashes); // Сохраняем путь к файлу и его хэши в hashVector

        /*
           Закомментированный код ниже - альтернативное решение,
           которое пыталось сохранить пути к файлам в словаре на основе их хэшей.
           Однако это решение не удовлетворяет условиям лабораторной работы.
        */
        /*
        try {
            auto hashes = file_processing(entry.path(), blockSize);
            std::string hashKey(reinterpret_cast<const char*>(hashes.data()), hashes.size() * sizeof(uint32_t));
            hashMap[hashKey].push_back(entry.path());
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        */
    }
}

// Функция для поиска дубликатов
void find_duplicates(const std::vector<fs::path>& directories, const std::vector<fs::path>& exclusions, size_t blockSize, size_t minSize, std::regex& maskRegex, int scanLevel) {
    std::unordered_map<std::string, std::set<fs::path>> hashMap; // Словарь для хранения путей дубликатов
    std::vector<std::pair<fs::path, std::vector<uint32_t>>> hashVector; // Вектор для хранения всех обработанных файлов и их хэшей

    for (const auto& dir : directories) { // Проходим по всем указанным директориям
        if (!fs::exists(dir) || !fs::is_directory(dir)) { // Проверяем, существует ли директория и является ли она директорией
            std::cerr << "Директория не существует или не является директорией: " << dir << std::endl; 
            continue; // Переходим к следующей директории
        }

        // Выбор итератора в зависимости от уровня сканирования
        if (scanLevel == 0) { // Если уровень сканирования 0 (только указанная директория без вложенных)
            for (const auto& entry : fs::directory_iterator(dir)) { // Проходим по всем элементам в директории
                shouldProcessFile(entry, exclusions, minSize, maskRegex, blockSize, hashVector); // Обрабатываем файл
            }
        } else { // Если уровень сканирования 1 (рекурсивное сканирование)
            for (const auto& entry : fs::recursive_directory_iterator(dir)) { // Проходим по всем элементам рекурсивно
                shouldProcessFile(entry, exclusions, minSize, maskRegex, blockSize, hashVector); // Обрабатываем файл
            }
        }
    }

    // Сравнение хешей и добавление дубликатов в hashMap
    for (size_t i = 0; i < hashVector.size(); ++i) { // Проходим по всем обработанным файлам
        for (size_t j = i + 1; j < hashVector.size(); ++j) { // Сравниваем с остальными файлами
            if (compare_hashes(hashVector[i].second, hashVector[j].second)) {  // Если файлы идентичны по хэшам
                std::string hashKey(reinterpret_cast<const char*>(hashVector[i].second.data()), hashVector[i].second.size() * sizeof(uint32_t)); // Создаем ключ для хеш-таблицы из хэшей файла
                hashMap[hashKey].insert(hashVector[i].first); // Добавляем путь первого файла в словарь дубликатов
                hashMap[hashKey].insert(hashVector[j].first); // Добавляем путь второго файла в словарь дубликатов
            }
        }
    }

    // Вывод результатов
    for (const auto& pair : hashMap) { // Проходим по всем найденным дубликатам в словаре
        std::cout << "Дубликаты:\n"; // Выводим заголовок для группы дубликатов
        for (const auto& file : pair.second) { // Проходим по всем файлам в группе дубликатов
            std::cout << file << std::endl; // Выводим путь к файлу
        }
        std::cout << std::endl; // Разделяем группы дубликатов пустой строкой
    }

    /*
       Закомментированный код ниже - альтернативный способ вывода результатов,
       который проверяет наличие дубликатов и выводит только те группы,
       которые содержат более одного файла.
    */
    /*
    for (const auto& [hashKey, files] : hashMap) {
        if (files.size() > 1) { // Если есть дубликаты
            std::cout << "Дубликаты:\n"; 
            for (const auto& file : files) {
                std::cout << file << std::endl; 
            }
            std::cout << std::endl; // Разделяем группы дубликатов пустой строкой
        }
    }
    */
}

int main() { // Начало функции main, точки входа в программу
    std::vector<fs::path> directories; // Вектор для хранения путей директорий для сканирования
    std::vector<fs::path> exclusions; // Вектор для хранения путей директорий, которые нужно исключить из сканирования
    size_t blockSize = 4096; // Значение по умолчанию для размера блока чтения файла
    size_t minSize = 1; // Минимальный размер файла по умолчанию
    int scanLevel = 1; // Уровень сканирования по умолчанию (1 - рекурсивное)

    // Запрос директорий для сканирования
    int numDirs; // Переменная для хранения количества директорий
    std::cout << "Введите количество директорий для сканирования: "; 
    std::cin >> numDirs; // Чтение количества директорий

    for (int i = 0; i < numDirs; ++i) { // Цикл для ввода каждой директории
        fs::path dir; // Переменная для хранения пути к директории
        std::cout << "Введите путь к директории " << (i + 1) << ": "; 
        std::cin >> dir; // Чтение пути к директории
        directories.push_back(dir); // Добавление введенной директории в вектор
    }

    // Запрос директорий для исключения
    int numExclusions; // Переменная для хранения количества исключаемых директорий
    std::cout << "Введите количество директорий для исключения: "; 
    std::cin >> numExclusions; // Чтение количества исключаемых директорий

    for (int i = 0; i < numExclusions; ++i) { // Цикл для ввода каждой исключаемой директории
        fs::path exclDir; // Переменная для хранения пути к исключаемой директории
        std::cout << "Введите путь к директории для исключения " << (i + 1) << ": "; 
        std::cin >> exclDir; // Чтение пути к исключаемой директории
        exclusions.push_back(exclDir); // Добавление введенной директории в вектор исключений
    }

    // Запрос уровня сканирования
    std::cout << "Введите уровень сканирования (0 - только указанная директория без вложенных, 1 - рекурсивное): "; 
    std::cin >> scanLevel; // Чтение уровня сканирования

    // Запрос маски имен файлов
    std::string maskString; // Переменная для хранения маски имен файлов
    std::cout << "Введите маску имен файлов разрешенных для сравнения (например, *.txt или file?.txt): "; 
    std::cin >> maskString; // Чтение маски имен файлов

    // Преобразуем маску в регулярное выражение
    std::regex star_regex("\\*"); // Регулярное выражение для символа '*'
    std::regex question_regex("\\?"); // Регулярное выражение для символа '?'
    
    maskString = "^" + std::regex_replace(maskString, star_regex, ".*"); // Заменяем '*' на '.*' и добавляем начало строки
    maskString = std::regex_replace(maskString, question_regex, "."); // Заменяем '?' на '.'
    
    maskString += "$"; // Добавляем конец строки к регулярному выражению

    std::cout << "Регулярное выражение: " << maskString << std::endl; 
    
    // Запрос минимального размера файла
    std::cout << "Введите минимальный размер файла (по умолчанию 1): "; 
    size_t inputminSize; // Переменная для хранения введенного минимального размера файла
    
    if (std::cin >> inputminSize && inputminSize > 0) { // Если пользователь ввел корректный размер больше нуля
        minSize = inputminSize; // Устанавливаем минимальный размер файла на основе пользовательского ввода
    }

    // Запрос размера блока
    std::cout << "Введите размер блока (по умолчанию 4096): "; 
    size_t inputBlockSize; // Переменная для хранения введенного размера блока
    
    if (std::cin >> inputBlockSize && inputBlockSize > 0) { // Если пользователь ввел корректный размер больше нуля
        blockSize = inputBlockSize; // Устанавливаем размер блока на основе пользовательского ввода
    }

    try {
        std::regex maskRegex(maskString, std::regex_constants::icase); // Создаем регулярное выражение с игнорированием регистра

        find_duplicates(directories, exclusions, blockSize, minSize, maskRegex, scanLevel); // Вызываем функцию поиска дубликатов с заданными параметрами
        
    } catch (const std::regex_error& e) { 
        std::cerr << "Ошибка в регулярном выражении: " << e.what() << '\n'; 
        return 1; // Возвращаем код ошибки 1 при возникновении исключения
    }
    
    return 0; // Завершаем программу с кодом успешного завершения 0
}

// //./lab7 -p C:/Users/Sopha/Downloads/lab7/dir1 -p C:/Users/Sopha/Downloads/lab7/dir2 -e C:/Users/Sopha/Downloads/lab7/dir1/dirindir1 -b 5 -m "*.txt" -s 1 -min 1
// int main(int argc, char* argv[]) {
//     std::vector<fs::path> directories;
//     std::vector<fs::path> exclusions;
//     size_t blockSize = 4096;
//     size_t minSize = 1;
//     int scanLevel = 1;
//     std::string maskString = "*.txt"; // Значение по умолчанию


//     // Разбор аргументов командной строки
//     for (int i = 1; i < argc; ++i) {
//         std::string arg = argv[i];
//         if (arg == "-p" || arg == "--path") { 
//             if (++i < argc) {
//                 directories.push_back(argv[i]);
//             } else {
//                 std::cerr << "Ошибка: параметр -p/--path требует аргумент.\n";
//                 return 1;
//             }
//         } else if (arg == "-e" || arg == "--exclude") {
//             if (++i < argc) {
//                 exclusions.push_back(argv[i]);
//             } else {
//                 std::cerr << "Ошибка: параметр -e/--exclude требует аргумент.\n";
//                 return 1;
//             }
//         } else if (arg == "-b" || arg == "--block-size") {
//             if (++i < argc) {
//                 try {
//                     blockSize = std::stoul(argv[i]);
//                 } catch (const std::invalid_argument& e) {
//                     std::cerr << "Ошибка: некорректное значение для параметра -b/--block-size.\n";
//                     return 1;
//                 } catch (const std::out_of_range& e) {
//                     std::cerr << "Ошибка: значение для параметра -b/--block-size слишком большое.\n";
//                     return 1;
//                 }
//             } else {
//                 std::cerr << "Ошибка: параметр -b/--block-size требует аргумент.\n";
//                 return 1;
//             }
//         } else if (arg == "-m" || arg == "--mask") {
//             if (++i < argc) {
//                 maskString = argv[i];
//             } else {
//                 std::cerr << "Ошибка: параметр -m/--mask требует аргумент.\n";
//                 return 1;
//             }
//         } else if (arg == "-s" || arg == "--scan-level") {
//             if (++i < argc) {
//                 try {
//                     scanLevel = std::stoi(argv[i]);
//                     if (scanLevel < 0 || scanLevel > 1) {
//                         std::cerr << "Ошибка: значение параметра -s/--scan-level должно быть 0 или 1.\n";
//                         return 1;
//                     }
//                 } catch (const std::invalid_argument& e) {
//                     std::cerr << "Ошибка: некорректное значение для параметра -s/--scan-level.\n";
//                     return 1;
//                 } catch (const std::out_of_range& e) {
//                     std::cerr << "Ошибка: значение для параметра -s/--scan-level слишком большое.\n";
//                     return 1;
//                 }
//             } else {
//                 std::cerr << "Ошибка: параметр -s/--scan-level требует аргумент.\n";
//                 return 1;
//             }
//         } else if (arg == "-min" || arg == "--min-size") {
//             if (++i < argc) {
//                 try {
//                     minSize = std::stoul(argv[i]);
//                 } catch (const std::invalid_argument& e) {
//                     std::cerr << "Ошибка: некорректное значение для параметра -min/--min-size.\n";
//                     return 1;
//                 } catch (const std::out_of_range& e) {
//                     std::cerr << "Ошибка: значение для параметра -min/--min-size слишком большое.\n";
//                     return 1;
//                 }
//             } else {
//                 std::cerr << "Ошибка: параметр -min/--min-size требует аргумент.\n";
//                 return 1;
//             }
//         }
//         else {
//             std::cerr << "Неизвестный параметр: " << arg << "\n";
//             return 1;
//         }
//     }

//     // Проверка на обязательный параметр --path
//     if (directories.empty()) {
//         std::cerr << "Ошибка: Необходимо указать хотя бы один путь с помощью параметра -p/--path.\n";
//         return 1;
//     }


//     try {
//         std::regex maskRegex(maskString, std::regex_constants::icase);
//         find_duplicates(directories, exclusions, blockSize, minSize, maskRegex, scanLevel);
//     } catch (const std::regex_error& e) {
//         std::cerr << "Ошибка в регулярном выражении: " << e.what() << '\n';
//         return 1;
//     }
//     return 0;
// }