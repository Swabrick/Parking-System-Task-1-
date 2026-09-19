# Smart Parking Management System

A local web-based parking management system developed in C++.

## System overview

The system manages 200 parking spaces in a five-floor facility. Each floor contains Wing A and Wing B. Each wing contains 20 spaces.

The system provides:

- Live parking availability
- Automatic parking space allocation
- Vehicle entry registration
- Vehicle exit registration
- Parking duration calculation
- Automatic fee calculation
- Vehicle search
- Parking history
- Local data storage
- Offline web operation through localhost

## Parking layout

- 5 floors
- 2 wings per floor
- 20 spaces per wing
- 40 spaces per floor
- 200 spaces in total

A space is identified using a code such as `1A05` or `5B20`.

## Parking fees

| Duration | Fee |
|---|---:|
| Up to 30 minutes | Free |
| Up to 2 hours | KSh 50 |
| Up to 4 hours | KSh 100 |
| Up to 6 hours | KSh 300 |
| Over 6 hours | KSh 500 |

The system rounds seconds up to the next whole minute when calculating the final parking duration.

## Technology

- C++
- HTML
- CSS
- JavaScript
- Local C++ HTTP server
- Local file-based database

No online database or web hosting service is required.

## Local database

The program creates `storage/parking.db` automatically. The file is stored on the local computer and contains the parking spaces and completed parking records.

The database is a small binary data store implemented in C++. It is designed specifically for this system and does not require a separate database server.

## Folder structure

```text
smart-parking-system-cpp/
├── include/
│   ├── config.h
│   ├── database.h
│   ├── http_server.h
│   ├── models.h
│   ├── parking.h
│   └── utils.h
├── src/
│   ├── database.cpp
│   ├── http_server.cpp
│   ├── main.cpp
│   ├── parking.cpp
│   └── utils.cpp
├── web/
│   ├── index.html
│   ├── script.js
│   └── style.css
├── storage/
│   └── parking.db
├── docs/
├── .gitignore
├── Makefile
└── README.md
```

## How the modules work together

`main.cpp` starts the application and connects the database to the parking system and local web server.

`parking.cpp` contains the main parking operations including entry assignment exit processing search and fee calculation.

`database.cpp` handles local storage. The program creates the database file when it does not exist and updates it after changes.

`http_server.cpp` provides the local HTTP server and connects browser requests to the parking system.

The files in `web/` form the user interface. JavaScript requests current data from the C++ server and updates the page without requiring an internet connection.

## Running the system

A C++17 compiler is required.

### Windows using MinGW g++

From the project folder run:

```text
g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp src/database.cpp src/parking.cpp src/utils.cpp src/http_server.cpp -o smart_parking.exe -lws2_32
```

Then run:

```text
smart_parking.exe
```

Open the following address in a browser:

```text
http://localhost:8080
```

### Linux

```text
g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp src/database.cpp src/parking.cpp src/utils.cpp src/http_server.cpp -o smart_parking
./smart_parking
```

Then open:

```text
http://localhost:8080
```

## Notes

The application binds to localhost so the web interface is intended to run on the local computer. Internet access is not required for normal operation.
