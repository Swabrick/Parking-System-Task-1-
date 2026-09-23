# System Features

## Live parking information

The dashboard shows the current number of available and occupied spaces.

The parking layout shows all 200 spaces and their current state.

The live vehicle page shows vehicles currently inside the facility together with their spaces, entry times, duration, estimated fee and payment-pending state when an exit is waiting for payment confirmation.

## Database management

The management interface can read the local records and edit transaction information.

The database is stored locally in `storage/parking.db`.

## Analytics

The system calculates:

- Collected revenue
- Recorded sessions
- Confirmed paid sessions
- Free sessions
- Pending payments
- Average parking duration
- Peak entry hour
- Vehicle visits by floor

Pending payments are not counted as collected revenue.

## Printable receipt

After a free exit or confirmed paid exit, the system provides a printable parking receipt.

The receipt contains:

- Vehicle number
- Parking space
- Entry time
- Exit time
- Duration
- Amount
- Payment method when applicable

## Manual payment collection

Charged exits are not released immediately. The system records the amount as pending and keeps the parking space occupied.

The operator collects the payment manually and confirms the payment in the exit window. Cash is supported for this manual collection flow. Card and M-Pesa are listed as future electronic integrations and currently display a Coming soon message.

After confirmation:

1. The transaction is marked paid.
2. The parking space is released.
3. The exit barrier opens automatically when automatic mode is enabled.
4. The operator can open the barrier manually when automatic operation is unavailable.

## Parking rate management

Management can change the parking limits and fees from the web interface.

The new values are stored in the local database so the software does not need to be changed when rates change.

## Number plate blacklist

Management can add or remove vehicle registration numbers from the blacklist.

A blacklisted vehicle is rejected during entry.

## Barrier controls

Automatic barrier operation is the default path for successful vehicle movement. The system opens the relevant barrier automatically and closes the software barrier state after a short automatic-open period on the next system check.

Manual controls are available for both entrance and exit barriers. They can be used when automatic operation fails or when an operator needs to override it.

The current project provides software barrier controls. Physical barrier hardware can be connected later.
