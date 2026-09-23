# Database Design

The system uses a local file based database stored on the computer. The database is created automatically inside the `storage` folder when the system first runs.

No online database provider is required.

## Storage

```text
Computer storage
    |
    +-- storage/
          |
          +-- parking.db
```

The database is a custom binary data store implemented in C++. It is read and updated by the C++ backend.

## Stored information

### Parking slots

Each slot stores:

- Floor
- Wing
- Slot number
- Status
- Vehicle registration when occupied
- Entry time when occupied

There are 200 parking slot records.

### Parking transactions

Each recorded visit stores:

- Transaction ID
- Vehicle registration
- Floor
- Wing
- Slot number
- Entry time
- Exit time
- Duration in minutes
- Amount
- Payment status
- Payment method
- Transaction status

A charged exit is stored as `Pending` until the operator confirms that payment was collected. The parking slot remains occupied during this stage.

### Blacklist

The database stores vehicle registration numbers that are not allowed to enter the facility.

### Pricing settings

The database stores the current parking limits and fees. This allows management to change the parking rates from the web interface without changing the source code.

The default settings are:

| Limit | Fee |
|---|---:|
| Up to 30 minutes | Free |
| Up to 2 hours | KSh 50 |
| Up to 4 hours | KSh 100 |
| Up to 6 hours | KSh 300 |
| Over 6 hours | KSh 500 |

## Reading the database

The management interface reads the local database through the C++ backend and displays:

- Live vehicles
- Parking spaces
- Parking transactions
- Payment information
- Blacklisted vehicle numbers
- Pricing settings
- Analytics

## Editing database records

Transaction records can be edited from the Parking Records page. Management can change the recorded amount and payment information while the system validates payment state changes.

Payment completion has its own confirmation action because it also releases the parking space and controls the exit barrier.

Pricing and blacklist records can be changed from the Management page.

## Payment flow

```text
Vehicle reaches exit
        |
        v
Fee calculated
        |
        v
Payment Pending
        |
        v
Operator collects payment
        |
        v
Operator confirms payment
        |
        +----> Payment marked Paid
        |
        +----> Parking space released
        |
        +----> Exit barrier opens automatically
                 or manually if automatic mode is unavailable
```

## Database updates

The database is saved after important changes such as:

- Vehicle entry
- Exit calculation
- Payment confirmation
- Adding a blacklist record
- Removing a blacklist record
- Changing parking rates
- Editing a transaction

The program writes through a temporary file before replacing the main database file. This reduces the chance of leaving a partially written database after a failed write.

## Database version

The current database format uses version `SPDB2`. The program can read the previous `SPDB1` format and load its parking records using the default pricing settings. The next save converts the data to the current format.
