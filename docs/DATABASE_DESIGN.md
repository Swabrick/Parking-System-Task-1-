# Database Design

The system uses a local database file stored on the computer.

## Stored information

### Parking slots

Each slot stores:

- Floor
- Wing
- Slot number
- Status
- Vehicle registration when occupied
- Entry time when occupied

### Parking transactions

Each completed visit stores:

- Transaction ID
- Vehicle registration
- Floor
- Wing
- Slot number
- Entry time
- Exit time
- Duration in minutes
- Amount paid
- Transaction status

## Storage model

```text
Computer storage
    |
    +-- storage/
          |
          +-- parking.db
```

The database is created locally by the program. It does not depend on a cloud service or an internet connection.
