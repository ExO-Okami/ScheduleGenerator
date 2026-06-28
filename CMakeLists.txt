cmake_minimum_required(VERSION 3.14)
project(TimetableGA LANGUAGES CXX)

# Требуем стандарт C++17 (нужен для auto [c1, c2] = crossover(...))
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исполняемый файл из нужных .cpp файлов
# (заголовки .h подключать НЕ нужно — они подтянутся автоматически через #include)
add_executable(timetable
    main.cpp
    fitness.cpp
    genetic_algorithm.cpp
)

# Включаем оптимизацию для Release-сборки
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Флаги компиляции
if(MSVC)
    # Для Visual Studio
    target_compile_options(timetable PRIVATE /W4 /O2)
else()
    # Для GCC / Clang
    target_compile_options(timetable PRIVATE -Wall -Wextra -O2)
endif()