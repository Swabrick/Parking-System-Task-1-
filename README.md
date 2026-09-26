# Saint Hotel Parking System

A local web based parking management system developed in C++.

## System overview

The system manages 200 parking spaces in a five floor facility. Each floor contains Wing A and Wing B. Each wing contains 20 spaces.

The system provides:

- Live parking availability
- Automatic parking space allocation
- Vehicle entry registration
- Vehicle exit registration
- Parking duration calculation
- Automatic fee calculation
- Vehicle search
- Live vehicle records
- Parking history
- Database record editing
- Parking analytics
- Printable parking receipts
- Manual payment collection and confirmation
- Payment status tracking
- Parking rate management
- Number plate blacklisting
- Automatic barrier operation
- Manual barrier override
- Local offline data storage

## Parking layout

- 5 floors
- 2 wings per floor
- 20 spaces per wing
- 40 spaces per floor
- 200 spaces in total

A space is identified using a code such as `1A05` or `5B20`.

## Parking fees

The default rates are:

| Duration | Fee |
|---|---:|
| Up to 30 minutes | Free |
| Up to 2 hours | KSh 50 |
| Up to 4 hours | KSh 100 |
| Up to 6 hours | KSh 300 |
| Over 6 hours | KSh 500 |

The rates can be changed from the Management page and are stored in the local database.

## Technology

- C++17
- HTML
- CSS
- JavaScript
- Local C++ HTTP server
- Local file based database

No online database provider or web hosting service is required.

## Local database

The program creates `storage/parking.db` automatically.

The database stores:

- Parking spaces
- Parking transactions
- Number plate blacklist
- Parking rates
- Payment information

The current database format is `SPDB2`. Older `SPDB1` parking data can be loaded and converted when the system saves it.

## Web interface

The system runs locally at:

```text
http://localhost:8080
```

The interface contains:

- Dashboard
- Parking Layout
- Vehicle Operations
- Live Vehicles
- Parking Records
- Analytics
- Management

## Entry and exit

During entry the system checks the registration number and blacklist, then assigns the first available parking space. If automatic barrier mode is enabled, the entrance barrier opens automatically. Manual controls remain available as a fallback.

During exit the system calculates the duration and fee. Free exits are completed immediately. For a charged exit, the transaction is marked as pending and the parking space remains occupied until the operator confirms payment.

The current manual payment flow supports cash collection. Card and M-Pesa are displayed as future electronic integrations and currently show a Coming soon message.

After payment confirmation, the parking space is released and the exit barrier opens automatically when automatic mode is enabled. If automatic operation is unavailable, the operator can open and close the barrier manually from Management.

A printable receipt can be generated after a free exit or a confirmed payment.

## Management

The Management page allows the operator to:

- Change parking rates
- Add a vehicle to the blacklist
- Remove a vehicle from the blacklist
- Open or close the entrance barrier manually
- Open or close the exit barrier manually
- Switch between automatic and manual barrier mode

## Analytics

The system calculates collected revenue, average parking duration, confirmed paid sessions, free sessions, pending payments, peak entry hour and visits by floor. Pending payments are not counted as collected revenue.

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
│   └── .gitkeep
├── docs/
│   ├── ALGORITHMS.md
│   ├── DATABASE_DESIGN.md
│   ├── DATA_STRUCTURES.md
│   └── FEATURES.md
├── .gitignore
├── Makefile
└── README.md
```

## How the modules work together

`main.cpp` starts the database, parking system and local web server.

`parking.cpp` contains the main vehicle entry and exit rules, fee calculation, payment confirmation, blacklist management, pricing management and barrier control.

`database.cpp` handles the local file based database.

`http_server.cpp` provides the local HTTP server and API endpoints used by the web interface.

The files in `web/` form the user interface. JavaScript requests current data from the C++ server and updates the page without requiring an internet connection.

## Running the system on Windows

A C++17 compiler such as MinGW g++ is required.

From the project folder run:

```text
g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp src/database.cpp src/parking.cpp src/utils.cpp src/http_server.cpp -o smart_parking.exe -lws2_32
```

Then run:

```text
smart_parking.exe
```

Open:

```text
http://localhost:8080
```

## Running on Linux

```text
g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp src/database.cpp src/parking.cpp src/utils.cpp src/http_server.cpp -o smart_parking
./smart_parking
```

Then open:

```text
http://localhost:8080
```

## Notes

The application binds to localhost so the web interface is intended to run on the local computer.

Internet access is not required for normal operation.

The barrier layer currently stores the barrier state in software. The automatic and manual control points are kept separate so physical barrier hardware can be connected later without changing the parking rules.
