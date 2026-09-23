# Algorithms

## 1. Initialise the parking system

```text
START
Open the local database
Load saved parking spaces, transactions, rates and blacklist
IF the parking spaces have not been created
    Create 5 floors
    FOR each floor
        Create Wing A and Wing B
        FOR each wing
            Create 20 parking spaces
        END FOR
    END FOR
END IF
Save the current system data
END
```

## 2. Register vehicle entry

```text
START
Read vehicle registration
Normalize the registration number
Check the blacklist
IF vehicle is blacklisted
    Reject entry
END IF

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
Save changes to the local database

IF automatic barrier mode is enabled
    Open entrance barrier automatically
ELSE
    Tell the operator to use the manual barrier control
END IF

Display the assigned parking space
END
```

## 3. Register vehicle exit

A charged exit has two stages. The system first calculates the amount and creates a pending payment record. The parking space is kept occupied until the operator confirms that payment has been collected.

```text
START
Read vehicle registration
Find the occupied space belonging to the vehicle
IF vehicle is not found
    Display error
END IF

Check whether payment is already pending for the vehicle
IF payment is already pending
    Show the existing payment request
END IF

Record exit time
Calculate parking duration
Calculate the fee using the saved rates
Create a transaction

IF amount is zero
    Mark payment as paid
    Release the parking space
    IF automatic barrier mode is enabled
        Open exit barrier automatically
    ELSE
        Ask operator to open the barrier manually
    END IF
ELSE
    Mark payment as pending
    Keep the parking space occupied
    Ask operator to collect payment
END IF

Save changes
Display the receipt information
END
```

## 4. Manual payment collection

```text
START
Read the pending transaction ID
Operator collects the payment from the driver
Select the payment method
IF payment method is not selected
    Reject confirmation
END IF

Mark payment as paid
Store payment method
Release the parking space
Save changes

IF automatic barrier mode is enabled
    Open exit barrier automatically
ELSE
    Ask operator to open the exit barrier manually
END IF

Display confirmation and receipt
END
```

The current interface supports manual cash collection. Card and M-Pesa integrations are shown as future options and are not processed by the program yet.

## 5. Fee calculation

The rates are stored in the local database so management can change them without changing the program.

```text
IF duration <= free limit
    fee = 0
ELSE IF duration <= first limit
    fee = first fee
ELSE IF duration <= second limit
    fee = second fee
ELSE IF duration <= third limit
    fee = third fee
ELSE
    fee = maximum fee
END IF
```

The default limits are 30 minutes, 2 hours, 4 hours and 6 hours.

The default fees are Free, KSh 50, KSh 100, KSh 300 and KSh 500.

## 6. Live vehicle records

```text
START
Read all occupied parking spaces
FOR each occupied space
    Display vehicle registration
    Display parking space
    Display entry time
    Calculate time currently spent inside
    Calculate the current estimated fee
    Show payment pending when the vehicle has an unfinished charged exit
END FOR
END
```

## 7. Analytics

```text
START
Read stored parking transactions
Add only confirmed payments to collected revenue
Count confirmed paid sessions
Count free sessions
Count pending payments
Calculate average parking duration
Count vehicle visits for each floor
Count entries for each hour of the day
Find the hour with the highest number of entries
Display the results
END
```

## 8. Edit a parking record

```text
START
Select a transaction from the records page
Read the transaction ID
Change the amount or payment information
Validate the values
IF trying to complete a pending payment through normal record editing
    Ask the operator to use payment confirmation instead
END IF
Save the updated transaction to the local database
Display confirmation
END
```

## 9. Number plate blacklist

```text
START
Read vehicle registration
Normalize the registration
Check whether it is already on the blacklist
IF it is already listed
    Reject the request
ELSE
    Add the registration to the blacklist
    Save the database
END IF
```

During vehicle entry the blacklist is checked before a parking space is assigned.

## 10. Barrier control

Automatic operation is the normal path. Manual controls are available as a fallback.

```text
ENTRY OR CONFIRMED EXIT
    IF automatic mode is enabled
        Open the relevant barrier automatically
        Keep it open for the short automatic-open period
        Close it automatically on the next system check after the period
    ELSE
        Wait for the operator to open the barrier manually
    END IF

IF automatic operation fails or is unavailable
    Operator can open or close the entrance or exit barrier manually
END IF
```

The current project represents barrier state in software. Physical barrier hardware can be connected to the same control points later.
