# CDBNPP - Conditions DataBase API library for NPP (C++)

## Intro
CDBNPP is a general-purpose Conditions Database API library for C++, addressing conditions/calibrations data access requirements of HEP and NP experiments.

It summarizes the CDB development and usage experience of STAR@RHIC experiment for over two decades, and accounts for CERN experiments use cases and best practices. "Conditions" here mean everything "non-event" data (or meta-data), like Detector Conditions, Calibrations, Geometry (alignment) - the non-DAQ data accompanying data taking process. This meta-data is primarily used in the detector simulation and data reconstruction processes.

#### CDBNPP allows time- and run-based, versioned data access in multiple ways:
- via direct db connection using SOCI DBAL (stands for Simple Open (Database) Call Interface)
- via HTTP-based REST-like API
- via flat file storage having data attributes embedded in file names
Intermediate results are cached using the provided in-memory cache.

Direct-db adapter of CDBNPP supports MySQL, Postres, Sqlite3 and, in theory, most if not all DB connectors supported by SOCI. HTTP adapter
supports MySQL and Postgres at the moment.

#### Packaging
This repository provides three major parts of CDBNPP:
- the CDBNPP library, under "lib" folder
- the Command-Line Interface executable, under "cli" folder
- the REST-like HTTP(S) service, under "rest" folder

## License
CDBNPP is distributed under the terms of [MIT License](https://en.wikipedia.org/wiki/MIT_License)

## Requirements

### Core:
- C++17 (gcc 8+, clang 5+)
- CMake 3.20+

### Libraries:
- DB Adapter: SOCI - DB Abstraction Layer library, https://github.com/SOCI/soci
- HTTP Adapter: libcurl
- nlohmann::json - header-only library, included in /contrib
- jwt-cpp - header-only library, included in /contrib
- valijson - header-only library, included in /contrib
- picosha2 - header-only library, included in /contrib
- xoshiro-cpp - header-only library, included in /contrib

## Documentation
See "documentation" and "examples" folders for details.

[MIT](https://en.wikipedia.org/wiki/MIT_License) © [Dmitry Arkhipkin](https://github.com/dmarkh)

