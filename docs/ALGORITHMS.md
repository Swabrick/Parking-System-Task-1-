# Algorithms

## 1. Initialise parking spaces

```text
START
Create an empty list of parking spaces
FOR floor 1 to 5
    FOR wing A to B
        FOR slot 1 to 20
            Create the parking space
            Mark it as available
            Add it to the list
        END FOR
    END FOR
END FOR
Save the spaces in the local database
END
```

## 2. Register vehicle entry

```text
START
Read vehicle registration
Normalize the registration number
Check whether the vehicle is already inside
IF vehicle is already inside
    Reject entry
END IF

Find the first available parking space
IF no space is available
    Reject entry
END IF

Mark the space as occupied
Store vehicle registration
Store entry time
Save changes to local database
Display assigned parking space
END
```

## 3. Register vehicle exit

```text
START
Read vehicle registration
Find the occupied space belonging to the vehicle
IF vehicle is not found
    Display error
END IF

Record exit time
Calculate parking duration
Calculate fee from the pricing tiers
Create a completed transaction
Mark the parking space as available
Save changes to local database
Display duration and fee
END
```

## 4. Fee calculation

```text
IF duration <= 30 minutes
    fee = 0
ELSE IF duration <= 120 minutes
    fee = 50
ELSE IF duration <= 240 minutes
    fee = 100
ELSE IF duration <= 360 minutes
    fee = 300
ELSE
    fee = 500
END IF
```

## 5. Search vehicle

```text
START
Read vehicle registration
Normalize registration
Search occupied parking spaces
IF registration is found
    Display parking space
ELSE
    Display not found
END IF
END
```
